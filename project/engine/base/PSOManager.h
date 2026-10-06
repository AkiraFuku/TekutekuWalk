#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <d3dx12.h> // CD3DX12 構造体のために推奨
#include <dxcapi.h> // IDxcBlobのために推奨


// 追加：シェーダーの種類を定義
enum class ShaderType {
    VS, PS, GS, HS, DS, CS
};

struct ShaderSet {
    // vectorで管理する場合。インデックス = ShaderType とすると扱いやすいです。
    // もしくは、個別に保持せず map<ShaderType, ComPtr<IDxcBlob>> にするのも手です。
    std::unordered_map<ShaderType, Microsoft::WRL::ComPtr<IDxcBlob>> blobs;
};

struct InputLayout
{
    D3D12_INPUT_LAYOUT_DESC inputLayout{};
    std::vector<D3D12_INPUT_ELEMENT_DESC>inputElement{};
};
struct PsoConfig {
    /// <summary>
    /// 
    /// </summary>
    struct ShaderPath {
        ShaderType type;
        std::wstring path;
        std::string entryPoint = "main"; // 必要に応じて
        std::wstring profile;           // L"vs_6_0" など
    };
    std::vector<ShaderPath> shaderPaths;

    using RootSignatureGenerator = std::function<Microsoft::WRL::ComPtr<ID3D12RootSignature>()>;
    RootSignatureGenerator rootSignatureGenerator;

    using InputLayoutGenerator = std::function<InputLayout()>;
    InputLayoutGenerator inputLayoutGenerator;

    D3D12_DEPTH_STENCIL_DESC depth = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_NONE;
    bool depthEnable = true;
    D3D12_DEPTH_WRITE_MASK depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

};
enum class Toporogy {
    PointList, LineList, TriangleList,
};
enum class BlendMode {
    None, Normal, Add, Subtract, Multiply, Screen
};
enum class FillMode {
    kSolid, kWireFrame
};



struct PsoSet {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
};

class PSOManager {
public:
    static PSOManager* GetInstance();
    friend struct std::default_delete<PSOManager>;
    void Initialize();
    void Finalize();

    void RegisterPsoGenerator(const std::string& name, const PsoConfig& psoConfig);
    const PsoSet& GetPso(const std::string& name, BlendMode blendMode = BlendMode::None, FillMode fillMode = FillMode::kSolid, Toporogy type = Toporogy::TriangleList);
    const PsoSet& GetComputePso(const std::string& name);

    D3D12_STATIC_SAMPLER_DESC StaticSamplers();

private:
    PSOManager() = default;
    ~PSOManager() = default;

    void CreatePso(const std::string& name, BlendMode blend, FillMode fill, Toporogy type);
    void CreateComputePso(const std::string& name);
    D3D12_BLEND_DESC CreateBlendDesc(BlendMode mode);
    void EnsureShaders(const std::string& name, ShaderSet& outSet);
    D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(Toporogy type);

    struct CacheKey {
        std::string name;
        BlendMode blend;
        FillMode fill;
        Toporogy type;
        bool operator==(const CacheKey& o) const {
            return name == o.name && blend == o.blend && fill == o.fill && type == o.type;
        }
    };

    struct KeyHasher {
        std::size_t operator()(const CacheKey& k) const {
            size_t seed = std::hash<std::string>()(k.name);

            // 一般的なハッシュ結合アルゴリズム
            auto hash_combine = [&seed](size_t value) {
                seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                };

            hash_combine(std::hash<int>()((int)k.blend));
            hash_combine(std::hash<int>()((int)k.fill));
            hash_combine(std::hash<int>()((int)k.type));
            return seed;
        }
    };

    static std::unique_ptr<PSOManager> instance_;
    std::unordered_map<std::string, PsoConfig> psoConfigs_;
    std::unordered_map<CacheKey, PsoSet, KeyHasher> psoCache_;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12RootSignature>> rootSigCache_;
    std::unordered_map<std::string, ShaderSet> shaderCache_;
};

class RootSignatureBuilder {
public:
    RootSignatureBuilder() = default;
    ~RootSignatureBuilder() = default;

    // --- ルートパラメータ追加用メソッド (メソッドチェーン対応) ---

    /// CBV (定数バッファ) を追加
    RootSignatureBuilder& AddCBV(
        UINT shaderRegister,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL,
        UINT registerSpace = 0);

    /// SRV (シェーダーリソース) を追加
    RootSignatureBuilder& AddSRV(
        UINT shaderRegister,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL,
        UINT registerSpace = 0);

    /// UAV (アンオーダードアクセス) を追加
    RootSignatureBuilder& AddUAV(
        UINT shaderRegister,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL,
        UINT registerSpace = 0);

    /// ディスクリプタテーブルを追加 (単一レンジ)
    RootSignatureBuilder& AddDescriptorTable(
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
        UINT numDescriptors,
        UINT baseShaderRegister,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL,
        UINT registerSpace = 0);

    /// ディスクリプタテーブルを追加 (複数レンジ指定)
    RootSignatureBuilder& AddDescriptorTable(
        const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges,
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

    // --- サンプラー追加用メソッド ---

    /// スタティックサンプラーを追加
    RootSignatureBuilder& AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& sampler);

    /// フラグ設定
    RootSignatureBuilder& SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags);

    // --- ビルド処理 ---

    /// RootSignature をビルドして ComPtr として返却
    Microsoft::WRL::ComPtr<ID3D12RootSignature> Build(ID3D12Device* device);

private:
    std::vector<D3D12_ROOT_PARAMETER> rootParameters_;
    // ディスクリプタテーブル参照用にポインタが破棄されないよう実体を保持
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> descriptorRangesList_;
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers_;
    D3D12_ROOT_SIGNATURE_FLAGS flags_ = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
};