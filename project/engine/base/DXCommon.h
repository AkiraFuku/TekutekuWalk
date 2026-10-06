#pragma once

#include "WinApp.h"
#include <array> //
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <wrl.h>

#pragma comment(lib, "dxcompiler.lib")
#include "DirectXTex.h"
#include "Vector4.h"
#include "d3dx12.h"
#include <chrono>
#include <memory>

class DXCommon {
public:
    // シングルトンインスタンス取得
    static DXCommon* GetInstance();

    // コピー禁止
    DXCommon(const DXCommon& obj) = delete;
    DXCommon& operator=(const DXCommon& obj) = delete;
    friend struct std::default_delete<DXCommon>;

    void Initialize();
    void Finalize();

    // 描画開始前処理
    void PreDraw();
    // 描画終了後処理
    void PostDraw();
    void SwapChainDraw();

    // getter
    Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() const {
        return device_.Get();
    }
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() const {
        return commandList_.Get();
    }
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() const {
        return commandQueue_.Get();
    }
    // コンパイルシェーダー
    Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath,
        const wchar_t* profile);
    // クリエイトバッファ
    Microsoft::WRL::ComPtr<ID3D12Resource>
        CreateBufferResource(size_t sizeInBytes);
    // クリエイトテクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource>
        CreateTextureResourse(const DirectX::TexMetadata& metadata);
    // アップロードテクスチャ
    Microsoft::WRL::ComPtr<ID3D12Resource>
        UploadTextureData(const Microsoft::WRL::ComPtr<ID3D12Resource> textur,
            const DirectX::ScratchImage& mipImages);

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>
        CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heepType, UINT numDescriptors,
            bool shaderVisible);

    uint32_t GetSwapChainBufferCount() const {
        return swapChainDesc_.BufferCount;
    }

    WinApp* GetWinApp() const {
        return WinApp::GetInstance();
    }
    static const float kDeltaTime;


    struct RenderTexture {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint32_t srvIndex;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
        Vector4 clearColor;
    };

    void CreateRenderTexture(DXGI_FORMAT format, const Vector4& ClearColor);
    void CreateDepthTexture(DXGI_FORMAT format);


    const RenderTexture& GetRenderTexture(size_t index) const {
        return renderTextures_.at(index);
    }
    uint32_t GetRenderTextureSrvIndex(size_t index = 0) const {
        return renderTextures_.at(index).srvIndex;
    }  uint32_t GetDepthTextureSrvIndex() const {
        return depthTexture_.srvIndex;
    }


     Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVBufferResource(size_t sizeInBytes, uint32_t& uavIndex);

private:
    // コンストラクタ・デストラクタをprivateにして外部生成を禁止
    DXCommon() = default;
    ~DXCommon() = default;

    void RenderTextureDraw(size_t index = 0);
    void DepthTextureDraw();

    static std::unique_ptr<DXCommon> instance;

    // FPS固定
    void InitializeFixFPS();
    // FPS更新
    void UpdateFixFPS();
    // 記録用時間
    std::chrono::steady_clock::time_point reference_;

private:
    HRESULT hr_;

    void CreateDevice();
    // D3D12デバイス
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    // DXGIファクトリー
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;

    // コマンドキュー
    void CreateCommand();
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    // コマンドアロケーター
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
    // コマンドリスト
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    // レンダーテクスチャ
public:
    // レンダーテクスチャの枚数（ピンポンバッファなら2枚）
    static constexpr size_t kRenderTextureCount = 2;



private:
    std::array<RenderTexture, kRenderTextureCount> renderTextures_;
    Microsoft::WRL::ComPtr<ID3D12Resource>
        CreateRenderTextureResource(DXGI_FORMAT format, const Vector4& ClearColor);
    // レンダーテクスチャリソースデスクリプション
    D3D12_RESOURCE_DESC renderTextureResourceDesc_{};
    D3D12_HEAP_PROPERTIES renderTextureHeapProperties_{};
    // レンダーテクスチャのRTV
    void CreateRenderTextureRTV();

    struct DepthTexture {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint32_t srvIndex;
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
    };
    DepthTexture depthTexture_;
    Microsoft::WRL::ComPtr<ID3D12Resource>
        CreateDepthTextureResource(DXGI_FORMAT format);
    void CreateDepthTextureDSV();
    D3D12_RESOURCE_BARRIER depthBarrier_{};
    // スワップチェーン
    void CreateSwapChain();
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc_{};



    // 各種ディスクプリプターヒープ
    void CreateDescriptorHeaps();
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
    uint32_t descriptorSizeRTV_;
    uint32_t descriptorSizeDSV_;

    // レンダーターゲットビュー
    void CreateRenderTargetView();
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources_;
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc_{};
    // ディスクリプタ２つ用意
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap,
        uint32_t descriptorSize, uint32_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
        const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap,
        uint32_t descriptorSize, uint32_t index);
    // フェンス
    void CreateFence();
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_ = nullptr;
    HANDLE fenceEvent_ = nullptr;
    // ビューポート矩形
    void CreateViewport();
    D3D12_VIEWPORT viewport_{};
    // シザー矩形
    D3D12_RECT scissorRect_{};
    void CreateScissorRect();
    // DXCコンパイラ
    void CreateDXCompiler();
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils = nullptr;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler = nullptr;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler = nullptr;
    // IMGUI初期化

    // バリア
    D3D12_RESOURCE_BARRIER barrier_{};
    // フェンス値
    uint64_t fenceValue_ = 0;
};
