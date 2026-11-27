#include "PbdCommon.h"
#include "PSOManager.h"
#include "DXCommon.h"
#include "Logger.h"

void PbdCommon::Initialize() {
    PsoConfig config{};

    PsoConfig::ShaderPath vsPath{ ShaderType::VS, L"resources/shaders/PBD/PBD.VS.hlsl", "main", L"vs_6_0" };
    PsoConfig::ShaderPath psPath{ ShaderType::PS, L"resources/shaders/PBD/PBD.PS.hlsl", "main", L"ps_6_0" };

    config.shaderPaths.push_back(vsPath);
    config.shaderPaths.push_back(psPath);

    config.rootSignatureGenerator = []() {
        return RootSignatureBuilder()
            // 0. WVP (CBV b0, Vertex)
            .AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX)
            // 1. Material (CBV b0, Pixel)
            .AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL)
            // 2. Texture (Table t0, Pixel)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)
            // 3. Camera (CBV b1, Pixel)
            .AddCBV(1, D3D12_SHADER_VISIBILITY_PIXEL)
            // スタティックサンプラー
            .AddStaticSampler(PSOManager::GetInstance()->StaticSamplers())
            // ビルド
            .Build(DXCommon::GetInstance()->GetDevice().Get());
    };

    config.inputLayoutGenerator = []() {
        InputLayout inputLayout = {};
        inputLayout.inputElement = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        inputLayout.inputLayout.pInputElementDescs = inputLayout.inputElement.data();
        inputLayout.inputLayout.NumElements = static_cast<UINT>(inputLayout.inputElement.size());
        return inputLayout;
    };

    // 深度設定
    config.depthEnable = true;
    config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    config.cullMode = D3D12_CULL_MODE_NONE; // 布の裏面も描画

    // PSOManager に登録
    PSOManager::GetInstance()->RegisterPsoGenerator("PBD", config);

    // PSOとRootSignatureを取得してキャッシュ
    auto psoSet = PSOManager::GetInstance()->GetPso("PBD");
    rootSignature_ = psoSet.rootSignature;
    graphicsPipelineState_ = psoSet.pipelineState;
}

void PbdCommon::PbdCommonDraw() {
    auto commandList = DXCommon::GetInstance()->GetCommandList();
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetPipelineState(graphicsPipelineState_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
