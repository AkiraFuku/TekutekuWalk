#include "SpriteCommon.h"
#include "Logger.h"
#include <cassert>
#include "DXCommon.h"
#include "PSOManager.h"

void SpriteCommon::Initialize()
{

    PsoConfig config{};
    PsoConfig::ShaderPath vsPath{ ShaderType::VS, L"resources/shaders/Sprite/Sprite.vs.hlsl", "main", L"vs_6_0" };
    PsoConfig::ShaderPath psPath{ ShaderType::PS, L"resources/shaders/Sprite/Sprite.ps.hlsl", "main", L"ps_6_0" };
    config.shaderPaths.push_back(vsPath);
    config.shaderPaths.push_back(psPath);


    config.rootSignatureGenerator = []() {
     return RootSignatureBuilder()
    // 0. kMaterial (CBV b0, ALL)
    .AddCBV(0, D3D12_SHADER_VISIBILITY_ALL)

    // 1. kTransform (CBV b1, Vertex)
    .AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX)

    // 2. kTexture (DescriptorTable t0, Pixel)
    .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)

    // スタティックサンプラー追加
    .AddStaticSampler(PSOManager::GetInstance()->StaticSamplers())

    // ビルド＆生成
    .Build(DXCommon::GetInstance()->GetDevice().Get());
        };
    config.inputLayoutGenerator = []() {
        InputLayout inputLayout = {};

        inputLayout.inputElement ={
           { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };  
        
      
        inputLayout.inputLayout.pInputElementDescs = inputLayout.inputElement.data();
        inputLayout.inputLayout.NumElements = static_cast<UINT>(inputLayout.inputElement.size());
        return inputLayout;
        };
    // 深度設定
    config.depthEnable = true;
    config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

    // PSOManagerに名前を付けて登録
    PSOManager::GetInstance()->RegisterPsoGenerator("Sprite", config);
    auto psoSet = PSOManager::GetInstance()->GetPso("Sprite");
    rootSignature_ = psoSet.rootSignature;
    graphicsPipelineState_ = psoSet.pipelineState;
    //CreatePSO();

}

void SpriteCommon::SpriteCommonDraw()
{
    // RootSignatureの設定
    DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootSignature(rootSignature_.Get());
    //  //PSOの設定
    DXCommon::GetInstance()->GetCommandList()->SetPipelineState(graphicsPipelineState_.Get());

    //プリミティブトポロジーのセット
    DXCommon::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

}

