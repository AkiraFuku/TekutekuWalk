#include "SkyBox.h"
#include "Logger.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "DXCommon.h"
#include "MathFunction.h"
#include "Object3dCommon.h"
#include "EditorManager.h"
#ifdef USE_IMGUI
#include "imgui.h"
#endif

SkyBox::~SkyBox()
{
    EditorManager::SafeUnregister(this);
}

void SkyBox::OnInspectorGUI()
{
#ifdef USE_IMGUI
    if (ImGui::CollapsingHeader("SkyBox", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (materialData_) {
            ImGui::ColorEdit4("Color", &materialData_->color.x);
        }
        ImGui::TextDisabled("Texture: %s", textureFilePath_.c_str());
    }
#endif
}

void SkyBox::Initialize()
{

    PsoConfig PSO{};
    PsoConfig::ShaderPath vsPath{ ShaderType::VS, L"resources/shaders/Skybox/Skybox.vs.hlsl", "main", L"vs_6_0" };
    PsoConfig::ShaderPath psPath{ ShaderType::PS, L"resources/shaders/Skybox/Skybox.ps.hlsl", "main", L"ps_6_0" };
    PSO.shaderPaths.push_back(vsPath);
    PSO.shaderPaths.push_back(psPath);

    PSO.rootSignatureGenerator = []() {
       return RootSignatureBuilder()
    // [Param 0] Material (CBV b0, Pixel)
    .AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL)

    // [Param 1] Transform (CBV b1, Vertex)
    .AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX)

    // [Param 2] Texture (DescriptorTable t0, Pixel)
    .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)

    // スタティックサンプラー追加
    .AddStaticSampler(PSOManager::GetInstance()->StaticSamplers())

    // ビルド＆生成
    .Build(DXCommon::GetInstance()->GetDevice().Get());
        };

    PSO.inputLayoutGenerator = []() {
        InputLayout inputLayout = {};
        inputLayout.inputElement = {
           { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        inputLayout.inputLayout.pInputElementDescs = inputLayout.inputElement.data();
        inputLayout.inputLayout.NumElements = static_cast<UINT>(inputLayout.inputElement.size());
        return inputLayout;

        };
    PSO.cullMode = D3D12_CULL_MODE_NONE;
    PSO.depthEnable = true;
    PSO.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    PSO.depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    PSOManager::GetInstance()->RegisterPsoGenerator("SkyBox", PSO);
    vertexRecourse_ =
        DXCommon::GetInstance()->
        CreateBufferResource(sizeof(VertexData) * 24);
    vertexBufferView_.BufferLocation = vertexRecourse_.Get()->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 24;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);


    vertexRecourse_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));

    //右 描画インデックス[0,1,2],[2,1,3]
    vertexData_[0].position = { 1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[1].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[2].position = { 1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[3].position = { 1.0f, -1.0f, -1.0f, 1.0f };
    //左　描画インデックス[4,5,6],[6,5,7]
    vertexData_[4].position = { -1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[5].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[6].position = { -1.0f, -1.0f, -1.0f, 1.0f };
    vertexData_[7].position = { -1.0f, -1.0f, 1.0f, 1.0f };
    //前 描画インデックス[8,9,10],[10,9,11]
    vertexData_[8].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[9].position = { 1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[10].position = { -1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[11].position = { 1.0f, -1.0f, 1.0f, 1.0f };
    //後ろ 描画インデックス[12,13,14],[14,13,15]
    vertexData_[12].position = { -1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[13].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[14].position = { -1.0f, -1.0f, -1.0f, 1.0f };
    vertexData_[15].position = { 1.0f, -1.0f, -1.0f, 1.0f };
    //上 描画インデックス[16,17,18],[18,17,19]
    vertexData_[16].position = { -1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[17].position = { 1.0f, 1.0f, -1.0f, 1.0f };
    vertexData_[18].position = { -1.0f, 1.0f, 1.0f, 1.0f };
    vertexData_[19].position = { 1.0f, 1.0f, 1.0f, 1.0f };
    //下 描画インデックス[20,21,22],[22,21,23]
    vertexData_[20].position = { -1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[21].position = { 1.0f, -1.0f, 1.0f, 1.0f };
    vertexData_[22].position = { -1.0f, -1.0f, -1.0f, 1.0f };
    vertexData_[23].position = { 1.0f, -1.0f, -1.0f, 1.0f };
    indexResource_ =
        DXCommon::GetInstance()->
        CreateBufferResource(sizeof(uint32_t) * 36);
    indexBufferView_.BufferLocation = indexResource_.Get()->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * 36;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexResource_.Get()->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
    uint32_t indices[] = {
    0,1,2,2,1,3,
    4,5,6,6,5,7,
    8,9,10,10,9,11,
    12,13,14,14,13,15,
    16,17,18,18,17,19,
    20,21,22,22,21,23
    };
    std::memcpy(indexData_, indices, sizeof(indices));








    materialResource_ =
        DXCommon::GetInstance()->
        CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    materialData_->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

    //座標変換（0: Game, 1: Scene）
    for (int i = 0; i < 2; ++i) {
        transformationMatrixResources_[i] =
            DXCommon::GetInstance()->
            CreateBufferResource(sizeof(TransformationMatrix));
        transformationMatrixResources_[i].Get()->
            Map(0, nullptr, reinterpret_cast<void**>(&wvpResources_[i]));
        wvpResources_[i]->WVP = Makeidentity4x4();
        wvpResources_[i]->World = Makeidentity4x4();
        wvpResources_[i]->WorldInverseTranspose = Inverse(wvpResources_[i]->World);
    }


    worldTransform_.scale = Vector3(1.0f, 1.0f, 1.0f);
    worldTransform_.rotate = Vector3(0.0f, 0.0f, 0.0f);
    worldTransform_.translate = Vector3(0.0f, 0.0f, 0.0f);


    for (int i = 0; i < 24; ++i) {
        vertexData_[i].texcord = { vertexData_[i].position.x, vertexData_[i].position.y, vertexData_[i].position.z };
    }
    EditorManager::GetInstance()->RegisterObject(this);
}

void SkyBox::Finalize()
{
}

void SkyBox::Update()
{
    auto writePass = [&](int idx, Camera* cam) {
        if (!wvpResources_[idx]) return;
        Vector3 pos = cam ? cam->GetTranslate() : worldTransform_.translate;
        Matrix4x4 worldMatrix = MakeAffineMatrix(worldTransform_.scale, worldTransform_.rotate, pos);
        Matrix4x4 worldViewProjectionMatrix = cam
            ? Multiply(worldMatrix, cam->GetViewProtectionMatrix())
            : worldMatrix;
        wvpResources_[idx]->WVP = worldViewProjectionMatrix;
        wvpResources_[idx]->World = worldMatrix;
        wvpResources_[idx]->WorldInverseTranspose = Transpose(Inverse(worldMatrix));
    };

    writePass(0, camera_);
    Camera* editorCam = EditorManager::GetInstance()->GetEditorCamera();
    writePass(1, editorCam ? editorCam : camera_);
}

void SkyBox::Draw()
{
    if (!isVisible_) {
        return;
    }
    auto psoSet = PSOManager::GetInstance()->GetPso("SkyBox", BlendMode::Normal, FillMode::kSolid);
    // PSOをセット
    DXCommon::GetInstance()->GetCommandList()->SetPipelineState(psoSet.pipelineState.Get());
    //パイプラインステートとルートシグネチャの設定

    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootSignature(psoSet.rootSignature.Get());
    DXCommon::GetInstance()->
        GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView_);
    // ★ 2. プリミティブトポロジをセット（これが抜けているはずです）
    DXCommon::GetInstance()->
        GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //インデックスバッファの設定
    //マテリアルの設定
    DXCommon::GetInstance()->
        GetCommandList()->
        SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    // SRVのディスクプリプターテーブルの先頭を設定
    DXCommon::GetInstance()->
        GetCommandList()->
        SetGraphicsRootDescriptorTable(2,
            TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex_));
    //座標変換行列の設定
    int pass = Object3dCommon::GetInstance()->GetRenderPassIndex();
    DXCommon::GetInstance()->
        GetCommandList()->
        SetGraphicsRootConstantBufferView(1, transformationMatrixResources_[pass]->GetGPUVirtualAddress());


    //  DXCommon::GetInstance()->GetCommandList()->DrawInstanced(24, 1, 0, 0);

    DXCommon::GetInstance()->
        GetCommandList()->IASetIndexBuffer(&indexBufferView_); // 追加
    DXCommon::GetInstance()->
        GetCommandList()->DrawIndexedInstanced(36, 1, 0, 0, 0); // インデックスドローに変更
}

void SkyBox::SetTextureByFilePath(const std::string& textureFilePath)
{
    textureIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);

}
