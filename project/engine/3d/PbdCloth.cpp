#include "PbdCloth.h"
#include "PbdCommon.h"
#include "DXCommon.h"
#include "TextureManager.h"
#include "MathFunction.h"
#include <cstring>
#include <cassert>

void PbdCloth::Initialize(const Vector3& startPos, const Vector3& endPos, int width, int height,
                          float k, float dt, float kDamping, const Vector3& gravity) {
    // 物理シミュレータ初期化
    solver_.InitializeGrid(startPos, endPos, width, height, k, dt, kDamping, gravity);

    camera_ = PbdCommon::GetInstance()->GetDefaultCamera();

    // GPU バッファの作成
    CreateBuffers();
    CreateConstantBuffers();

    // デフォルトテクスチャの読み込み
    SetTexture("resources/white.png");

    // 初期頂点データと法線の設定
    UpdateNormalsAndVertices();
}

void PbdCloth::CreateBuffers() {
    int width  = solver_.GetWidth();
    int height = solver_.GetHeight();
    if (width < 2 || height < 2) return;

    vertexCount_ = static_cast<UINT>(width * height);
    indexCount_  = static_cast<UINT>((width - 1) * (height - 1) * 6);

    // 1. 頂点バッファの作成 (UPLOAD)
    vertexResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(VertexData) * vertexCount_);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataPtr_));

    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = static_cast<UINT>(sizeof(VertexData) * vertexCount_);
    vbv_.StrideInBytes = sizeof(VertexData);

    // 2. インデックスバッファの作成 (UPLOAD/DEFAULT)
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

    indexResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(uint32_t) * indexCount_);
    uint32_t* indexDataPtr = nullptr;
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexDataPtr));
    std::memcpy(indexDataPtr, indices.data(), sizeof(uint32_t) * indexCount_);
    indexResource_->Unmap(0, nullptr);

    ibv_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibv_.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * indexCount_);
    ibv_.Format = DXGI_FORMAT_R32_UINT;
}

void PbdCloth::CreateConstantBuffers() {
    auto dxCommon = DXCommon::GetInstance();

    // WVP
    transformResource_ = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));
    transformResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformData_));
    transformData_->WVP = Makeidentity4x4();
    transformData_->World = Makeidentity4x4();
    transformData_->WorldInverseTranspose = Makeidentity4x4();

    // Material
    materialResource_ = dxCommon->CreateBufferResource(sizeof(MaterialCB));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = 1;

    // Camera (256バイト境界整列)
    size_t cameraBufferSize = (sizeof(CameraForGPU) + 0xff) & ~0xff;
    cameraResource_ = dxCommon->CreateBufferResource(cameraBufferSize);
    cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
    cameraData_->worldPosition = { 0.0f, 0.0f, -10.0f };
    cameraData_->farClip = 1000.0f;
    cameraData_->cameraForward = { 0.0f, 0.0f, 1.0f };
}

void PbdCloth::UpdateNormalsAndVertices() {
    if (!vertexDataPtr_) return;

    int width  = solver_.GetWidth();
    int height = solver_.GetHeight();
    if (width < 2 || height < 2) return;

    const auto& points = solver_.GetPoints();

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            uint32_t idx = static_cast<uint32_t>(i * height + j);
            const auto& p = points[i][j];

            vertexDataPtr_[idx].position = { p.position.x, p.position.y, p.position.z, 1.0f };
            vertexDataPtr_[idx].texcoord = {
                static_cast<float>(i) / (width - 1),
                static_cast<float>(j) / (height - 1)
            };

            // 周囲の質点から法線を計算
            Vector3 right = (i < width - 1) ? (points[i + 1][j].position - p.position)
                                            : (p.position - points[i - 1][j].position);
            Vector3 down  = (j < height - 1) ? (points[i][j + 1].position - p.position)
                                            : (p.position - points[i][j - 1].position);

            Vector3 normal = Cross(right, down);
            float len = Length(normal);
            if (len > 0.0001f) {
                normal = normal / len;
            } else {
                normal = { 0.0f, 0.0f, 1.0f };
            }

            vertexDataPtr_[idx].normal = normal;
        }
    }
}

void PbdCloth::Update() {
    // 物理シミュレーションを1ステップ更新
    solver_.Update();

    // 頂点バッファと法線を同期
    UpdateNormalsAndVertices();
}

void PbdCloth::Draw() {
    if (indexCount_ == 0 || !vertexResource_ || !indexResource_) return;

    // カメラの自動同期
    if (!camera_) {
        camera_ = PbdCommon::GetInstance()->GetDefaultCamera();
    }

    Matrix4x4 worldMatrix = MakeAffineMatrix(scale_, rotate_, translate_);
    Matrix4x4 wvpMatrix = worldMatrix;

    if (camera_) {
        wvpMatrix = Multiply(worldMatrix, camera_->GetViewProtectionMatrix());
        cameraData_->worldPosition = camera_->GetTranslate();
        cameraData_->farClip = 1000.0f;

        const Matrix4x4& mat = camera_->GetWorldMatrix();
        Vector3 forward = { mat.m[2][0], mat.m[2][1], mat.m[2][2] };
        cameraData_->cameraForward = Normalize(forward);
    }

    transformData_->WVP = wvpMatrix;
    transformData_->World = worldMatrix;
    transformData_->WorldInverseTranspose = Transpose(Inverse(worldMatrix));

    auto commandList = DXCommon::GetInstance()->GetCommandList();

    // PBD共通パイプラインステートのバインド
    PbdCommon::GetInstance()->PbdCommonDraw();

    // ルートパラメータの設定
    // 0: WVP (Vertex)
    commandList->SetGraphicsRootConstantBufferView(0, transformResource_->GetGPUVirtualAddress());
    // 1: Material (Pixel)
    commandList->SetGraphicsRootConstantBufferView(1, materialResource_->GetGPUVirtualAddress());
    // 2: Texture (Pixel Table)
    commandList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex_));
    // 3: Camera (Pixel)
    commandList->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());

    // 頂点バッファ & インデックスバッファのセット
    commandList->IASetVertexBuffers(0, 1, &vbv_);
    commandList->IASetIndexBuffer(&ibv_);

    // 描画実行
    commandList->DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
}

void PbdCloth::SetTexture(const std::string& filePath) {
    textureFilePath_ = filePath;
    TextureManager::GetInstance()->LoadTexture(filePath);
    textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(filePath);
}

void PbdCloth::SetColor(const Vector4& color) {
    if (materialData_) {
        materialData_->color = color;
    }
}

void PbdCloth::SetEnableLighting(bool enable) {
    if (materialData_) {
        materialData_->enableLighting = enable ? 1 : 0;
    }
}
