#include "PSOManager.h" // ファイル名変更に合わせてインクルードも変更
#include "DXCommon.h"
#include "Logger.h"
#include <cassert>
#include <d3d12.h>
#include <dxcapi.h> // IDxcBlobのため


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxcompiler.lib")

using namespace Microsoft::WRL;

std::unique_ptr<PSOManager> PSOManager::instance_ = nullptr;

PSOManager* PSOManager::GetInstance() {
    if (instance_ == nullptr) {
        // privateコンストラクタを呼び出せるヘルパー構造体
        struct Helper : public PSOManager {
            Helper() : PSOManager() {}
        };
        instance_ = std::make_unique<Helper>();
    }
    return instance_.get();
}



void PSOManager::Initialize() {
    psoCache_.clear();
    rootSigCache_.clear();
    psoConfigs_.clear();
}

void PSOManager::Finalize() {
    psoCache_.clear();
    rootSigCache_.clear();
    psoConfigs_.clear();
}

void PSOManager::RegisterPsoGenerator(const std::string& name, const PsoConfig& psoConfig) {
    psoConfigs_[name] = psoConfig;
}

D3D12_STATIC_SAMPLER_DESC PSOManager::StaticSamplers()
{

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    return sampler;
}

const PsoSet& PSOManager::GetPso(const std::string& name, BlendMode blend, FillMode fill, Toporogy type) {
    CacheKey key{ name, blend, fill, type };
    if (psoCache_.contains(key)) {
        return psoCache_[key];
    }

    // 設定に CS が含まれている場合は Compute PSO を取得
    if (psoConfigs_.contains(name)) {
        const auto& config = psoConfigs_.at(name);
        for (const auto& sp : config.shaderPaths) {
            if (sp.type == ShaderType::CS) {
                return GetComputePso(name);
            }
        }
    }

    CreatePso(name, blend, fill, type);
    return psoCache_.at(key);
}

const PsoSet& PSOManager::GetComputePso(const std::string& name) {
    CacheKey key{ name, BlendMode::None, FillMode::kSolid, Toporogy::TriangleList };
    if (psoCache_.contains(key)) {
        return psoCache_[key];
    }
    CreateComputePso(name);
    return psoCache_.at(key);
}

void PSOManager::CreateComputePso(const std::string& name) {
    auto device = DXCommon::GetInstance()->GetDevice();
    const auto& config = psoConfigs_.at(name);

    // 1. RootSignature のキャッシュ確認と生成
    if (!rootSigCache_.contains(name)) {
        assert(config.rootSignatureGenerator && "RootSignatureGenerator is null");
        rootSigCache_[name] = config.rootSignatureGenerator();
    }
    auto rootSignature = rootSigCache_[name];

    // 2. CS シェーダーの取得
    ShaderSet shaders;
    EnsureShaders(name, shaders);
    assert(shaders.blobs.contains(ShaderType::CS) && "Compute Shader Blob not found");

    // 3. Compute PSO 構築
    D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc{};
    computeDesc.pRootSignature = rootSignature.Get();
    auto& csBlob = shaders.blobs[ShaderType::CS];
    computeDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };

    PsoSet psoSet;
    psoSet.rootSignature = rootSignature;
    HRESULT hr = device->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&psoSet.pipelineState));
    assert(SUCCEEDED(hr) && "Failed to create Compute Pipeline State");

    CacheKey key{ name, BlendMode::None, FillMode::kSolid, Toporogy::TriangleList };
    psoCache_[key] = psoSet;
}


// -------------------------------------------------------------------------
// シェーダー管理（重複コンパイル防止）
// -------------------------------------------------------------------------
// 修正内容: contains()にはキー（name）を渡す必要がある
void PSOManager::EnsureShaders(const std::string& name, ShaderSet& outSet) {
    if (shaderCache_.contains(name)) {
        outSet = shaderCache_[name];
        return;
    }

    auto dxCommon = DXCommon::GetInstance();
    const auto& config = psoConfigs_[name];
    ShaderSet newSet;

    for (const auto& shaderInfo : config.shaderPaths) {
        auto blob = dxCommon->CompileShader(shaderInfo.path.c_str(), shaderInfo.profile.c_str());
        assert(blob && "Shader Compilation Failed");
        newSet.blobs[shaderInfo.type] = blob;
    }

    shaderCache_[name] = newSet;
    outSet = newSet;


}

D3D12_PRIMITIVE_TOPOLOGY_TYPE PSOManager::GetPrimitiveTopologyType(Toporogy type)
{
    switch (type) {
    case Toporogy::PointList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    case Toporogy::LineList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    case Toporogy::TriangleList:
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    default:
        assert(false && "Unknown topology type");
        return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // デフォルトは三角形
    }
}


// -------------------------------------------------------------------------
// PSO 生成
// -------------------------------------------------------------------------
void PSOManager::CreatePso(const std::string& name, BlendMode blend, FillMode fill, Toporogy type) {
    auto device = DXCommon::GetInstance()->GetDevice();
    const auto& config = psoConfigs_.at(name);

    // 1. RootSignature のキャッシュ確認と生成
    if (!rootSigCache_.contains(name)) {
        assert(config.rootSignatureGenerator && "RootSignatureGenerator is null");
        rootSigCache_[name] = config.rootSignatureGenerator();
    }
    auto rootSignature = rootSigCache_[name];

    // 3. Shader の取得
    ShaderSet shaders;
    EnsureShaders(name, shaders);

    // 4. PSO構築
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignature.Get();

    // InputLayout


    InputLayout layoutHold;

    if (config.inputLayoutGenerator) {
        // 2. 構造体ごと受け取り、この関数が終わるまで保持する
        layoutHold = config.inputLayoutGenerator();

        // 3. 保持している実体のポインタを改めてセットする
        psoDesc.InputLayout = layoutHold.inputLayout;
        psoDesc.InputLayout.pInputElementDescs = layoutHold.inputElement.data();
    }
    /*{ inputElements.data(), static_cast<UINT>(inputElements.size()) };*/

   // Mapから各シェーダーを割り当て
    if (shaders.blobs.count(ShaderType::VS)) {
        auto& b = shaders.blobs[ShaderType::VS];
        psoDesc.VS = { b->GetBufferPointer(), b->GetBufferSize() };
    }
    if (shaders.blobs.count(ShaderType::PS)) {
        auto& b = shaders.blobs[ShaderType::PS];
        psoDesc.PS = { b->GetBufferPointer(), b->GetBufferSize() };
    }
    if (shaders.blobs.count(ShaderType::GS)) {
        auto& b = shaders.blobs[ShaderType::GS];
        psoDesc.GS = { b->GetBufferPointer(), b->GetBufferSize() };
    }
    if (shaders.blobs.count(ShaderType::HS)) {
        auto& b = shaders.blobs[ShaderType::HS];
        psoDesc.HS = { b->GetBufferPointer(), b->GetBufferSize() };
    }
    if (shaders.blobs.count(ShaderType::DS)) {
        auto& b = shaders.blobs[ShaderType::DS];
        psoDesc.DS = { b->GetBufferPointer(), b->GetBufferSize() };
    }
    if (shaders.blobs.count(ShaderType::CS)) {
        auto& b = shaders.blobs[ShaderType::CS];
    }



    // Blend State
    psoDesc.BlendState = CreateBlendDesc(blend);

    // Rasterizer State
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FillMode = (fill == FillMode::kWireFrame) ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = config.cullMode;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;

    // DepthStencil State (Configからの値を優先)
    psoDesc.DepthStencilState = config.depth;
    psoDesc.DepthStencilState.DepthEnable = config.depthEnable;
    psoDesc.DepthStencilState.DepthWriteMask = config.depthWriteMask;
    // 深度比較関数が設定されていない場合のデフォルト
    psoDesc.DepthStencilState.DepthFunc = config.depthFunc;
    if (psoDesc.DepthStencilState.DepthFunc == 0) {
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    }

    // フォーマット設定 (環境に合わせて適宜変更)
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.NumRenderTargets = 1;
    psoDesc.PrimitiveTopologyType = GetPrimitiveTopologyType(type);
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    PsoSet psoSet;
    psoSet.rootSignature = rootSignature;
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&psoSet.pipelineState));
    assert(SUCCEEDED(hr) && "Failed to create Pipeline State");

    psoCache_[{name, blend, fill, type}] = psoSet;
}

D3D12_BLEND_DESC PSOManager::CreateBlendDesc(BlendMode mode) {
    D3D12_BLEND_DESC blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;

    // 共通初期値
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    switch (mode) {
    case BlendMode::None:
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        break;
    case BlendMode::Normal:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        break;
    case BlendMode::Add:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        break;
    case BlendMode::Subtract:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
        break;
    case BlendMode::Multiply:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        break;
    case BlendMode::Screen:
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        break;
    }
    return blendDesc;
}

RootSignatureBuilder& RootSignatureBuilder::AddCBV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace) {
    D3D12_ROOT_PARAMETER param{};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;
    rootParameters_.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddSRV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace) {
    D3D12_ROOT_PARAMETER param{};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;
    rootParameters_.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddUAV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace) {
    D3D12_ROOT_PARAMETER param{};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    param.ShaderVisibility = visibility;
    param.Descriptor.ShaderRegister = shaderRegister;
    param.Descriptor.RegisterSpace = registerSpace;
    rootParameters_.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddDescriptorTable(
    D3D12_DESCRIPTOR_RANGE_TYPE rangeType, UINT numDescriptors, UINT baseShaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT registerSpace) 
{
    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = rangeType;
    range.NumDescriptors = numDescriptors;
    range.BaseShaderRegister = baseShaderRegister;
    range.RegisterSpace = registerSpace;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    return AddDescriptorTable({ range }, visibility);
}

RootSignatureBuilder& RootSignatureBuilder::AddDescriptorTable(
    const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges, D3D12_SHADER_VISIBILITY visibility) 
{
    // レンジの実体を保持（生成中のメモリ破棄防止）
    descriptorRangesList_.push_back(ranges);
    const auto& storedRanges = descriptorRangesList_.back();

    D3D12_ROOT_PARAMETER param{};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.ShaderVisibility = visibility;
    param.DescriptorTable.pDescriptorRanges = storedRanges.data();
    param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(storedRanges.size());

    rootParameters_.push_back(param);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& sampler) {
    staticSamplers_.push_back(sampler);
    return *this;
}

RootSignatureBuilder& RootSignatureBuilder::SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags) {
    flags_ = flags;
    return *this;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignatureBuilder::Build(ID3D12Device* device) {
    assert(device != nullptr);

    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.Flags = flags_;
    desc.pParameters = rootParameters_.data();
    desc.NumParameters = static_cast<UINT>(rootParameters_.size());
    desc.pStaticSamplers = staticSamplers_.data();
    desc.NumStaticSamplers = static_cast<UINT>(staticSamplers_.size());

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        }
        assert(false && "Failed to serialize RootSignature.");
        return nullptr;
    }

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));

    return rootSignature;
}