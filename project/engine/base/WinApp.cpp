#include "WinApp.h"
#include "assert.h"
#ifdef USE_IMGUI
#include "ImGuiManager.h"
#include"imgui_impl_win32.h"
#include"imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // USE_IMGUI

bool WinApp::ShouldHideCursor()
{
#if defined(NDEBUG)
#ifdef USE_IMGUI
    return !ImGuiManager::GetInstance()->IsShow();
#else
    return true;
#endif
#else
    return false;
#endif
}

void WinApp::RefreshCursor()
{
    if (!hwnd) {
        return;
    }
    POINT pt{};
    if (!GetCursorPos(&pt)) {
        return;
    }
    if (WindowFromPoint(pt) != hwnd) {
        return;
    }
    ScreenToClient(hwnd, &pt);
    RECT rect{};
    GetClientRect(hwnd, &rect);
    if (PtInRect(&rect, pt)) {
        SetCursor(ShouldHideCursor() ? nullptr : LoadCursor(nullptr, IDC_ARROW));
    }
}

LRESULT WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_SETCURSOR && LOWORD(lparam) == HTCLIENT && ShouldHideCursor()) {
        SetCursor(nullptr);
        return TRUE;
    }
#ifdef USE_IMGUI


    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
    {
        return true;
    }
#endif // USE_IMGUI
    switch (msg) {
        //
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void WinApp::Initialize() {
    HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);


    //ウィンドウプロシージャ
    wc.lpfnWndProc = WindowProc;
    //ウィンドウクラスの名前
    wc.lpszClassName =windowName_;
    //インスタンスハンドル
    wc.hInstance = GetModuleHandle(nullptr);
    //カーソル
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    //ウィンドウクラスの登録
    RegisterClass(&wc);
    // Output


    //OutputDebugStringA("Hello,DirectX!\n");


    RECT wrc = { 0, 0, kClientWidth, kClientHeight };

    //  
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, FALSE);

    //ウィンドウの作成
    hwnd = CreateWindow(
        wc.lpszClassName,//クラス名
        windowName_,

        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );


    //ウィンドウを表示
    ShowWindow(hwnd, SW_SHOW);
    //

    timeBeginPeriod(1);



};
void WinApp::Update() {}
void WinApp::Finalize()
{
    CoUninitialize();
    //デバッグレイヤーの解放

    CloseWindow(hwnd);

    ///デバッグレイヤーのライブオブジェクトのレポート
    Singleton<WinApp>::Finalize();
}
bool WinApp::ProcessMessage()
{
    MSG msg{};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return true;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return false;
}
;