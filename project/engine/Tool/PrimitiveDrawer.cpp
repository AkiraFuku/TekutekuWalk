#include "PrimitiveDrawer.h"

#include "Camera.h"
#include "DXCommon.h"
#include "Logger.h"
#include "MathFunction.h"
#include "PSOManager.h"
#include "RotateFunction.h"
#include <cassert>
#include <numbers>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

std::unique_ptr<PrimitiveDrawer> PrimitiveDrawer::instance_ = nullptr;

PrimitiveDrawer* PrimitiveDrawer::GetInstance() {
    if (instance_ == nullptr) {
        // privateコンストラクタを呼び出せるヘルパー構造体
        struct Helper : public PrimitiveDrawer {
            Helper() : PrimitiveDrawer() {}
        };
        instance_ = std::make_unique<Helper>();
    }
    return instance_.get();
}

void PrimitiveDrawer::Finalize() {}

void PrimitiveDrawer::AddPSO() {
#ifdef USE_LINE
    PsoConfig config{};
    PsoConfig::ShaderPath vsPath{ ShaderType::VS, L"resources/shaders/Primitive/Primitive.VS.hlsl", "main", L"vs_6_0" };
    PsoConfig::ShaderPath psPath{ ShaderType::PS, L"resources/shaders/Primitive/Primitive.PS.hlsl", "main", L"ps_6_0" };
    config.shaderPaths.push_back(vsPath);
    config.shaderPaths.push_back(psPath);

    // ルートシグネチャの生成（b0: WVP定数バッファ）
    config.rootSignatureGenerator = []() {
        return RootSignatureBuilder()
            .AddCBV(0, D3D12_SHADER_VISIBILITY_ALL)
            .Build(DXCommon::GetInstance()->GetDevice().Get());
    };

    // 入力レイアウトの生成（POSITION: float4, COLOR: float4）
    config.inputLayoutGenerator = []() {
        InputLayout inputLayout = {};
        inputLayout.inputElement = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        inputLayout.inputLayout.pInputElementDescs = inputLayout.inputElement.data();
        inputLayout.inputLayout.NumElements = static_cast<UINT>(inputLayout.inputElement.size());
        return inputLayout;
    };

    // 深度・カリング設定
    config.cullMode = D3D12_CULL_MODE_NONE;
    config.depthEnable = true;
    config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    config.depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    // PSOManagerに登録
    PSOManager::GetInstance()->RegisterPsoGenerator("Primitive", config);
#endif // USE_LINE
}

void PrimitiveDrawer::WVPResourceCreate() {
#ifdef USE_LINE
    WVPResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(WVPMatrix));
    WVPResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
    wvpData_->WVP = Makeidentity4x4();
#endif // USE_LINE
}

void PrimitiveDrawer::Initialize() {
#ifdef USE_LINE
    AddPSO();
    WVPResourceCreate();

    // 1. 頂点バッファ作成
    vertexResource_ = DXCommon::GetInstance()->CreateBufferResource(
        sizeof(VertexData) * kMaxVertices);
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataPtr_));

    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = sizeof(VertexData) * kMaxVertices;
    vbv_.StrideInBytes = sizeof(VertexData);

    // 2. インデックスバッファ作成
    indexResource_ = DXCommon::GetInstance()->CreateBufferResource(
        sizeof(uint32_t) * kMaxIndices);
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexDataPtr_));

    ibv_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    ibv_.SizeInBytes = sizeof(uint32_t) * kMaxIndices;
    ibv_.Format = DXGI_FORMAT_R32_UINT;

    vertices_.reserve(kMaxVertices);
    indices_.reserve(kMaxIndices);
#endif // USE_LINE
}

void PrimitiveDrawer::Draw() {
#ifdef USE_LINE
    if (commands_.empty())
        return;

    // 非表示設定時はGPU描画を発行せずクリア
    if (!isEnabled_) {
        vertices_.clear();
        indices_.clear();
        commands_.clear();
        return;
    }

    auto commandList = DXCommon::GetInstance()->GetCommandList();
    auto psoManager = PSOManager::GetInstance();

    // 1. WVP更新
    if (camera_) {
        wvpData_->WVP = camera_->GetViewProtectionMatrix();
    } else {
        wvpData_->WVP = Makeidentity4x4();
    }

    // 2. 頂点データをGPUバッファへコピー
    std::memcpy(vertexDataPtr_, vertices_.data(),
        sizeof(VertexData) * vertices_.size());
    std::memcpy(indexDataPtr_, indices_.data(),
        sizeof(uint32_t) * indices_.size());

    // 3. ルートシグネチャ・バッファのセット
    PsoSet firstPso =
        psoManager->GetPso("Primitive", commands_[0].blendMode,
            commands_[0].fillMode, commands_[0].psoTopology);
    commandList->SetGraphicsRootSignature(firstPso.rootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(
        0, WVPResource_->GetGPUVirtualAddress());

    commandList->IASetVertexBuffers(0, 1, &vbv_);
    commandList->IASetIndexBuffer(&ibv_);

    // 4. コマンド実行
    for (const auto& cmd : commands_) {
        PsoSet psoSet = psoManager->GetPso("Primitive", cmd.blendMode, cmd.fillMode,
            cmd.psoTopology);
        commandList->SetPipelineState(psoSet.pipelineState.Get());
        commandList->IASetPrimitiveTopology(cmd.topology);

        commandList->DrawIndexedInstanced(cmd.indexCount, 1, cmd.indexStart,
            cmd.baseVertex, 0);
    }

    // 5. フレーム終わりのクリア
    vertices_.clear();
    indices_.clear();
    commands_.clear();
#endif // USE_LINE
}

void PrimitiveDrawer::DrawLine(const Vector3& p1, const Vector3& p2,
    const Vector4& color) {
#ifdef USE_LINE
    if (!isEnabled_ || !isTypeEnabled_[static_cast<size_t>(PrimtiveType::kLine)]) return;

    if (vertices_.size() + 2 > kMaxVertices || indices_.size() + 2 > kMaxIndices)
        return;

    DrawCommand cmd;
    cmd.indexStart = static_cast<uint32_t>(indices_.size());
    cmd.indexCount = 2;
    cmd.baseVertex = static_cast<uint32_t>(vertices_.size());
    cmd.topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    cmd.psoTopology = Toporogy::LineList;
    cmd.fillMode = FillMode::kWireFrame;
    cmd.blendMode = BlendMode::Normal;

    // 2頂点を追加
    vertices_.push_back({ {p1.x, p1.y, p1.z, 1.0f}, color });
    vertices_.push_back({ {p2.x, p2.y, p2.z, 1.0f}, color });

    // インデックス（ローカル相対: 0, 1）を追加
    indices_.push_back(0);
    indices_.push_back(1);

    commands_.push_back(cmd);
#endif // USE_LINE
}

void PrimitiveDrawer::DrawTriangle(const Vector3& p1, const Vector3& p2,
    const Vector3& p3, const Vector4& color,
    FillMode fillMode, BlendMode blendMode) {
#ifdef USE_LINE
    if (!isEnabled_ || !isTypeEnabled_[static_cast<size_t>(PrimtiveType::kTriangle)]) return;

    if (vertices_.size() + 3 > kMaxVertices || indices_.size() + 3 > kMaxIndices) {
        return;
    }

    DrawCommand cmd;
    cmd.indexStart = static_cast<uint32_t>(indices_.size());
    cmd.indexCount = 3;
    cmd.baseVertex = static_cast<uint32_t>(vertices_.size());
    cmd.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    cmd.psoTopology = Toporogy::TriangleList;
    cmd.fillMode = fillMode;
    cmd.blendMode = blendMode;

    vertices_.push_back({ {p1.x, p1.y, p1.z, 1.0f}, color });
    vertices_.push_back({ {p2.x, p2.y, p2.z, 1.0f}, color });
    vertices_.push_back({ {p3.x, p3.y, p3.z, 1.0f}, color });

    indices_.push_back(0);
    indices_.push_back(1);
    indices_.push_back(2);

    commands_.push_back(cmd);
#endif // USE_LINE
}

void PrimitiveDrawer::DrawSphere(const Sphere& sphere, const Vector4& color) {
#ifdef USE_LINE
    if (!isEnabled_ || !isTypeEnabled_[static_cast<size_t>(PrimtiveType::kSpher)]) return;

    const uint32_t kSubdivision = 16;
    const uint32_t kNumVertices = (kSubdivision + 1) * (kSubdivision + 1);
    const uint32_t kNumIndices = kSubdivision * kSubdivision * 6;

    if (vertices_.size() + kNumVertices > kMaxVertices ||
        indices_.size() + kNumIndices > kMaxIndices)
        return;

    DrawCommand cmd;
    cmd.indexStart = static_cast<uint32_t>(indices_.size());
    cmd.indexCount = kNumIndices;
    cmd.baseVertex = static_cast<uint32_t>(vertices_.size());
    cmd.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    cmd.psoTopology = Toporogy::TriangleList;
    cmd.fillMode = FillMode::kWireFrame;
    cmd.blendMode = BlendMode::Normal;

    for (uint32_t lat = 0; lat <= kSubdivision; ++lat) {
        float phi = -std::numbers::pi_v<float> / 2.0f +
            std::numbers::pi_v<float> * static_cast<float>(lat) / kSubdivision;

        for (uint32_t lon = 0; lon <= kSubdivision; ++lon) {
            float theta = 2.0f * std::numbers::pi_v<float> * static_cast<float>(lon) /
                kSubdivision;

            Vector3 point = { sphere.radius * std::cos(phi) * std::cos(theta),
                             sphere.radius * std::sin(phi),
                             sphere.radius * std::cos(phi) * std::sin(theta) };

            point = RotateVector(point, sphere.rotate);
            point = Add(point, sphere.center);

            vertices_.push_back({ {point.x, point.y, point.z, 1.0f}, color });
        }
    }

    for (uint32_t lat = 0; lat < kSubdivision; ++lat) {
        for (uint32_t lon = 0; lon < kSubdivision; ++lon) {
            uint32_t start = lat * (kSubdivision + 1) + lon;

            indices_.push_back(start);
            indices_.push_back(start + 1);
            indices_.push_back(start + (kSubdivision + 1));

            indices_.push_back(start + 1);
            indices_.push_back(start + (kSubdivision + 1) + 1);
            indices_.push_back(start + (kSubdivision + 1));
        }
    }

    commands_.push_back(cmd);
#endif // USE_LINE
}

void PrimitiveDrawer::DrawGrid() {
#ifdef USE_LINE
    if (!isEnabled_ || !isTypeEnabled_[static_cast<size_t>(PrimtiveType::kGrid)]) return;

    const float kGridHalfWidth = 2.0f;
    const uint32_t kSubdivision = 10;
    const float kGridEvery =
        (kGridHalfWidth * 2.0f) / static_cast<float>(kSubdivision);

    for (uint32_t i = 0; i <= kSubdivision; ++i) {
        float pos = -kGridHalfWidth + static_cast<float>(i) * kGridEvery;
        Vector4 color = (i == kSubdivision / 2) ? Vector4{ 0, 0, 0, 1 }
        : Vector4{ 0.7f, 0.7f, 0.7f, 1 };
    }
#endif // USE_LINE
}

void PrimitiveDrawer::DrawAABB(const AABB& aabb, const Vector4& color) {
#ifdef USE_LINE
    if (!isEnabled_ || !isTypeEnabled_[static_cast<size_t>(PrimtiveType::kAABB)]) return;

    if (vertices_.size() + 8 > kMaxVertices || indices_.size() + 24 > kMaxIndices)
        return;

    DrawCommand cmd;
    cmd.indexStart = static_cast<uint32_t>(indices_.size());
    cmd.indexCount = 24;
    cmd.baseVertex = static_cast<uint32_t>(vertices_.size());
    cmd.topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    cmd.psoTopology = Toporogy::LineList;
    cmd.fillMode = FillMode::kWireFrame;
    cmd.blendMode = BlendMode::Normal;

    Vector3 min = aabb.min;
    Vector3 max = aabb.max;

    vertices_.push_back({ {min.x, min.y, min.z, 1.0f}, color }); // 0
    vertices_.push_back({ {max.x, min.y, min.z, 1.0f}, color }); // 1
    vertices_.push_back({ {max.x, max.y, min.z, 1.0f}, color }); // 2
    vertices_.push_back({ {min.x, max.y, min.z, 1.0f}, color }); // 3
    vertices_.push_back({ {min.x, min.y, max.z, 1.0f}, color }); // 4
    vertices_.push_back({ {max.x, min.y, max.z, 1.0f}, color }); // 5
    vertices_.push_back({ {max.x, max.y, max.z, 1.0f}, color }); // 6
    vertices_.push_back({ {min.x, max.y, max.z, 1.0f}, color }); // 7

    uint32_t boxIndices[24] = {
        0, 1,  1, 2,  2, 3,  3, 0,
        4, 5,  5, 6,  6, 7,  7, 4,
        0, 4,  1, 5,  2, 6,  3, 7
    };

    indices_.insert(indices_.end(), std::begin(boxIndices), std::end(boxIndices));

    commands_.push_back(cmd);
#endif // USE_LINE
}

void PrimitiveDrawer::DrawSegment(const Segment& segment,
    const Vector4& color) {
#ifdef USE_LINE
    if (!isEnabled_ || !isTypeEnabled_[static_cast<size_t>(PrimtiveType::kSegment)]) return;

    DrawLine(segment.origin, Add(segment.origin, segment.diff), color);
#endif // USE_LINE
}

void PrimitiveDrawer::DrawPlane(const Plane& plane, const Vector4& color) {
#ifdef USE_LINE
    if (!isEnabled_ || !isTypeEnabled_[static_cast<size_t>(PrimtiveType::Plane)]) return;
    // 必要に応じて平面描画処理を追加可能
#endif // USE_LINE
}

void PrimitiveDrawer::DrawImGui() {
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader("Primitive Drawer (Debug Draw)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enable All Primitives (F4)", &isEnabled_);
        if (isEnabled_) {
            ImGui::Indent();
            ImGui::Checkbox("Lines (線・レール)", &isTypeEnabled_[static_cast<size_t>(PrimtiveType::kLine)]);
            ImGui::Checkbox("Triangles (三角形・レイキャスト)", &isTypeEnabled_[static_cast<size_t>(PrimtiveType::kTriangle)]);
            ImGui::Checkbox("Spheres (球コライダー)", &isTypeEnabled_[static_cast<size_t>(PrimtiveType::kSpher)]);
            ImGui::Checkbox("AABB (バウンディングボックス)", &isTypeEnabled_[static_cast<size_t>(PrimtiveType::kAABB)]);
            ImGui::Checkbox("Grid (グリッド)", &isTypeEnabled_[static_cast<size_t>(PrimtiveType::kGrid)]);
            ImGui::Checkbox("Segments (線分)", &isTypeEnabled_[static_cast<size_t>(PrimtiveType::kSegment)]);
            ImGui::Checkbox("Planes (平面)", &isTypeEnabled_[static_cast<size_t>(PrimtiveType::Plane)]);
            ImGui::Unindent();
        }
    }
#endif // USE_IMGUI
}
