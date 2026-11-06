#include "ImGuiManager.h"
#include "SrvManager.h"
#include "WinApp.h"
#include "DXCommon.h"
#ifdef USE_IMGUI
#include"imgui.h"
#include"imgui_impl_dx12.h"
#include"imgui_impl_win32.h"
#endif // USE_IMGUI

// 静的メンバ変数の実体
std::unique_ptr<ImGuiManager> ImGuiManager::instance = nullptr;

ImGuiManager* ImGuiManager::GetInstance() {
    if (instance == nullptr) {
        // privateコンストラクタを呼び出せるヘルパー構造体
        struct Helper : public ImGuiManager {
            Helper() : ImGuiManager() {}
        };
        instance = std::make_unique<Helper>();
    }
    return instance.get();
}

void ImGuiManager::Initialize() {
    #ifdef USE_IMGUI

    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = "externals/imgui/my_imgui_settings.ini";
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(WinApp::GetInstance()->GetHwnd());

    // InitInfo をスタック変数で初期化（newによるメモリリークを修正）
    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = DXCommon::GetInstance()->GetDevice().Get();
    initInfo.CommandQueue = DXCommon::GetInstance()->GetCommandQueue().Get(); // 必須: テクスチャアップロード用
    initInfo.NumFramesInFlight =
        static_cast<int>(DXCommon::GetInstance()->GetSwapChainBufferCount());
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    initInfo.SrvDescriptorHeap = SrvManager::GetInstance()->GetDescriptorHeap().Get();

    // SRVアロケーターコールバックを SrvManager と連携して設定
    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu) {
        uint32_t index = SrvManager::GetInstance()->AllocateSRV();
        *out_cpu = SrvManager::GetInstance()->GetCPUDescriptorHandle(index);
        *out_gpu = SrvManager::GetInstance()->GetGPUDescriptorHandle(index);
    };
    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE) {
        // 現在の SrvManager は解放機能未実装のためノーオップ
    };

    ImGui_ImplDX12_Init(&initInfo);

#endif // USE_IMGUI

}
void ImGuiManager::Finalize() {
    #ifdef USE_IMGUI

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif // USE_IMGUI

}
void ImGuiManager::Begin() {
    #ifdef USE_IMGUI

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
#endif // USE_IMGUI

}
void ImGuiManager::End() {
    #ifdef USE_IMGUI

    ImGui::Render();
#endif // USE_IMGUI

}
void ImGuiManager::Draw() {
    #ifdef USE_IMGUI

    // 非表示設定時はGPU描画をスキップ
    if (!isShow_) {
        return;
    }

    ID3D12GraphicsCommandList* commandList = DXCommon::GetInstance()->GetCommandList().Get();

    ID3D12DescriptorHeap* ppHeaps[] = { SrvManager::GetInstance()->GetDescriptorHeap().Get() };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
#endif // USE_IMGUI

}
