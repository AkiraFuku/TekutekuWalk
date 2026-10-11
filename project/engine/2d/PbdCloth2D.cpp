#include "PbdCloth2D.h"
#include "SpriteCommon.h"
#include "DXCommon.h"
#include "TextureManager.h"
#include "WinApp.h"
#include "MathFunction.h"
#include <cstring>
#include <cmath>

void PbdCloth2D::Initialize(const Vector2& position, const Vector2& size, int gridX, int gridY,
                            float k, int pinMode) {
    position_ = position;
    size_ = size;
    gridX_ = gridX;
    gridY_ = gridY;
    stiffness_ = k;
    pinMode_ = pinMode;

    Reset();

    CreateBuffers();
    CreateConstantBuffers();

    SetTexture("resources/white.png");
    UpdateVertices();
}

void PbdCloth2D::Reset() {
    float localLeft   = -size_.x * anchorPoint_.x;
    float localRight  =  size_.x * (1.0f - anchorPoint_.x);
    float localTop    = -size_.y * anchorPoint_.y;
    float localBottom =  size_.y * (1.0f - anchorPoint_.y);

    Vector3 startPos = { localLeft,  localTop,    0.0f };
    Vector3 endPos   = { localRight, localBottom, 0.0f };

    solver_.InitializeGrid(startPos, endPos, gridX_, gridY_, stiffness_, 1.0f / 60.0f, 0.03f, { 0.0f, baseGravity_.y, 0.0f });
    solver_.SetSolverIterations(10);

    SetPinMode(pinMode_);
}

void PbdCloth2D::SetPinMode(int pinMode) {
    pinMode_ = pinMode;
    int w = solver_.GetWidth();
    int h = solver_.GetHeight();
    if (w < 2 || h < 2) return;

    // 一旦全点の固定フラグをリセット
    for (int i = 0; i < w; ++i) {
        for (int j = 0; j < h; ++j) {
            solver_.SetFixed(i, j, false);
        }
    }

    if (pinMode_ == 0) {
        // 上辺の左右2点のみ固定（幕・旗の2点吊り）
        solver_.SetFixed(0, 0, true);
        solver_.SetFixed(w - 1, 0, true);
    } else if (pinMode_ == 1) {
        // 上辺全体を固定（のれん・カーテン・バー固定）
        for (int i = 0; i < w; ++i) {
            solver_.SetFixed(i, 0, true);
        }
    } else if (pinMode_ == 2) {
        // 四隅のみ固定（画面端4点ピン留め）
        solver_.SetFixed(0, 0, true);
        solver_.SetFixed(w - 1, 0, true);
        solver_.SetFixed(0, h - 1, true);
        solver_.SetFixed(w - 1, h - 1, true);
    }
}

void PbdCloth2D::CreateBuffers() {
    int width  = solver_.GetWidth();
    int height = solver_.GetHeight();
    if (width < 2 || height < 2) return;

    vertexCount_ = static_cast<UINT>(width * height);
    indexCount_  = static_cast<UINT>((width - 1) * (height - 1) * 6);

    auto dxCommon = DXCommon::GetInstance();

    // 1. 頂点バッファ
    vertexResource_ = dxCommon->CreateBufferResource(sizeof(VertexData) * vertexCount_);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataPtr_));

    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertexCount_);
    vbv_.StrideInBytes = sizeof(VertexData);

    // 2. インデックスバッファ
    std::vector<uint32_t> indices;
    indices.reserve(indexCount_);

    for (int i = 0; i < width - 1; ++i) {
        for (int j = 0; j < height - 1; ++j) {
            uint32_t tl = static_cast<uint32_t>(i * height + j);
            uint32_t tr = static_cast<uint32_t>((i + 1) * height + j);
            uint32_t bl = static_cast<uint32_t>(i * height + (j + 1));
            uint32_t br = static_cast<uint32_t>((i + 1) * height + (j + 1));

            // 三角形 1: tl -> tr -> br
            indices.push_back(tl);
            indices.push_back(tr);
            indices.push_back(br);

            // 三角形 2: tl -> br -> bl
            indices.push_back(tl);
            indices.push_back(br);
            indices.push_back(bl);
        }
    }

    indexResource_ = dxCommon->CreateBufferResource(sizeof(uint32_t) * indexCount_);
    uint32_t* indexDataPtr = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexDataPtr));
    std::memcpy(indexDataPtr, indices.data(), sizeof(uint32_t) * indexCount_);
    indexResource_->Unmap(0, nullptr);

    ibv_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibv_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indexCount_);
    ibv_.Format = DXGI_FORMAT_R32_UINT;
}

void PbdCloth2D::CreateConstantBuffers() {
    auto dxCommon = DXCommon::GetInstance();

    // Material (b0)
    materialResource_ = dxCommon->CreateBufferResource(sizeof(MaterialCB));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = 0;
    materialData_->uvTransform = Makeidentity4x4();

    // TransformationMatrix (b1)
    transformationMatrixResource_ = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationData_));
    transformationData_->WVP = Makeidentity4x4();
    transformationData_->World = Makeidentity4x4();
}

void PbdCloth2D::UpdateVertices() {
    if (!vertexDataPtr_) return;

    int width  = solver_.GetWidth();
    int height = solver_.GetHeight();
    if (width < 2 || height < 2) return;

    const auto& points = solver_.GetPoints();

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            uint32_t idx = static_cast<uint32_t>(i * height + j);
            const auto& p = points[i][j];

            vertexDataPtr_[idx].position = { p.position.x, p.position.y, 0.0f, 1.0f };
            vertexDataPtr_[idx].texcoord = {
                static_cast<float>(i) / (width - 1),
                static_cast<float>(j) / (height - 1)
            };
            vertexDataPtr_[idx].normal = { 0.0f, 0.0f, -1.0f };
        }
    }
}

void PbdCloth2D::Update() {
    windTimer_ += 1.0f / 60.0f;

    // 風の適用（サイン波の重なりで自然な揺らぎ）
    if (windEnabled_) {
        float windX = (std::sin(windTimer_ * 2.5f) + std::sin(windTimer_ * 4.7f) * 0.4f) * windStrength_;
        float windY = std::cos(windTimer_ * 2.0f) * (windStrength_ * 0.15f);
        solver_.SetGravity({ windX, baseGravity_.y + windY, 0.0f });
    } else {
        solver_.SetGravity({ 0.0f, baseGravity_.y, 0.0f });
    }

    // 物理更新
    solver_.Update();

    // 頂点バッファ同期
    UpdateVertices();
}

void PbdCloth2D::Draw() {
    if (indexCount_ == 0 || !vertexResource_ || !indexResource_) return;

    // 行列の更新
    Vector3 scaleVec = { 1.0f, 1.0f, 1.0f };
    Vector3 rotateVec = { 0.0f, 0.0f, rotation_ };
    Vector3 translateVec = { position_.x, position_.y, 0.0f };
    Matrix4x4 worldMatrix = MakeAffineMatrix(scaleVec, rotateVec, translateVec);
    Matrix4x4 viewMatrix = Makeidentity4x4();
    Matrix4x4 projMatrix = MakeOrthographicMatrix(0.0f, 0.0f, static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight), 0.0f, 100.0f);

    transformationData_->WVP = Multiply(worldMatrix, Multiply(viewMatrix, projMatrix));
    transformationData_->World = worldMatrix;

    // Sprite共通描画準備
    SpriteCommon::GetInstance()->SpriteCommonDraw();

    auto psoSet = PSOManager::GetInstance()->GetPso("Sprite", blendMode_, FillMode::kSolid);
    auto commandList = DXCommon::GetInstance()->GetCommandList();

    commandList->SetPipelineState(psoSet.pipelineState.Get());
    commandList->IASetVertexBuffers(0, 1, &vbv_);
    commandList->IASetIndexBuffer(&ibv_);

    // ルートパラメータのバインド
    // 0: Material
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    // 1: TransformationMatrix
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());
    // 2: Texture SRV
    commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex_));

    commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}

void PbdCloth2D::ApplyGust(float forceX) {
    int w = solver_.GetWidth();
    int h = solver_.GetHeight();
    for (int i = 0; i < w; ++i) {
        for (int j = 1; j < h; ++j) {
            solver_.GetPoint(i, j).velocity.x += forceX;
        }
    }
}

void PbdCloth2D::SetTexture(const std::string& filePath) {
    textureFilePath_ = filePath;
    TextureManager::GetInstance()->LoadTexture(filePath);
    textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(filePath);
}

void PbdCloth2D::SetColor(const Vector4& color) {
    if (materialData_) {
        materialData_->color = color;
    }
}
