#pragma once
#include <Windows.h>
#include<cstdint>
#pragma comment(lib,"winmm.lib")
#include <memory>
#include "Vector2.h"
#include "Singleton.h"
class WinApp : public Singleton<WinApp>
{

public:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    friend class Singleton<WinApp>;
    friend struct std::default_delete<WinApp>;
    //初期化
    void Initialize();
    //更新
    void Update();
    //終了
    void Finalize();

    // //ウィンドウのサイズ
    static const int32_t kClientWidth = 1280;
    static const int32_t kClientHeight = 720;

    //ウィンドウの中央を取得
    Vector2 GetWindowCenter()const {
        return { kClientWidth / 2.0f, kClientHeight / 2.0f };
    }




    HWND GetHwnd()const {
        return hwnd;
    }
    HINSTANCE GetInstanceHandle()const {
        return wc.hInstance;
    }
    bool ProcessMessage();
    static bool ShouldHideCursor();
    void RefreshCursor();
protected:
    WinApp() = default;
    ~WinApp() override = default;

private: // メンバ変数
    HWND hwnd = nullptr;
    WNDCLASS wc{};
    //　ウィンドウ名
    const wchar_t* windowName_ = L"てくてくウォーク";


};

