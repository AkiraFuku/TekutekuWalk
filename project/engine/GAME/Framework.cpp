#include "Framework.h"
#include "ParticleManager.h"//フレームワークに移植
#include "SceneManager.h"
#include "WinApp.h"//フレームワークに移植
#include "ImGuiManager.h"//フレームワークに移植
#include"Audio.h"//フレームワークに移植
#include"Input.h"//フレームワークに移植
#include"StringUtility.h"//フレームワークに移植
#include"Logger.h"//フレームワークに移植
#include "SpriteCommon.h"//フレームワークに移植
#include "TextureManager.h"//フレームワークに移植
#include "Object3DCommon.h"//フレームワークに移植
#include "PbdCommon.h"
#include "ModelManager.h"//フレームワークに移植
#include "SrvManager.h"//フレームワークに移植
#include <fstream>
#include <iostream> 
#include "PSOManager.h"
#include "LightManager.h"
#include "OffScreen.h"
#include "JsonManager.h"
#include "PrimitiveDrawer.h"
static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
    //ダンプファイルの作成
    SYSTEMTIME time;
    GetLocalTime(&time);
    wchar_t filePath[MAX_PATH] = { 0 };
    CreateDirectory(L"./Dumps", nullptr);
    StringCchPrintfW(filePath, MAX_PATH,
        L"./Dumps/%04d-%02d%02d-%02d%02d.dmp",
        time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute);
    HANDLE dumpFileHandle = CreateFile(
        filePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        0, CREATE_ALWAYS, 0, 0
    );
    //プロセスIDとクラッシュが発生したスレッドIDを取得
    DWORD procesessId = GetCurrentProcessId();
    DWORD threadId = GetCurrentThreadId();
    //設定情報入力
    MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
    minidumpInformation.ThreadId = threadId;
    minidumpInformation.ExceptionPointers = exception;
    minidumpInformation.ClientPointers = TRUE;
    //ダンプの出力
    MiniDumpWriteDump(
        GetCurrentProcess(),
        procesessId,
        dumpFileHandle,
        MiniDumpNormal,
        &minidumpInformation,
        nullptr,
        nullptr
    );
    return EXCEPTION_EXECUTE_HANDLER;
}

std::wstring wstr = L"Hello,DirectX!";
void Framework::Initialize()
{
    //D3D12の初期化
    CoInitializeEx(0, COINIT_MULTITHREADED);

    SetUnhandledExceptionFilter(ExportDump);
    //ログ出力用のディレクトリを作成
    std::filesystem::create_directory("logs");
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
        nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    std::chrono::zoned_time localTime{
        std::chrono::current_zone(),
        nowSeconds
    };
    std::string dataString = std::format(
        "{:%Y%m%d_%H%M%S}",
        localTime
    );
    std::string logFilePath = std::string("logs/") + dataString + ".log";
    //ファイルへの書き込み
    std::ofstream logStream(logFilePath);


    WinApp::GetInstance()->Initialize();


    // 引数には生のポインタが必要なので .get() を使用
    DXCommon::GetInstance()->Initialize();
    PSOManager::GetInstance()->Initialize();
    OffScreen::GetInstance()->Initialize();

    ImGuiManager::GetInstance()->Initialize();
    JsonManager::GetInstance()->Initialize();
    TextureManager::GetInstance()->Initialize();
    ModelManager::GetInstance()->Initialize();
    ParticleManager::GetInstance()->Initialize();
    Logger::Log(StringUtility::ConvertString(std::format(L"WSTRING{}\n", wstr)));
    LightManager::GetInstance()->Initialize();
    // 外部入力
    Input::GetInstance()->Initialize();

    SpriteCommon::GetInstance()->Initialize();
    Object3dCommon::GetInstance()->Initialize();
    PbdCommon::GetInstance()->Initialize();
    Audio::GetInstance()->Initialize();


}

void Framework::Finalize()
{
    JsonManager::GetInstance()->Finalize();
    LightManager::GetInstance()->Finalize();
    SrvManager::GetInstance()->Finalize();
    SceneManager::GetInstance()->Finalize();
    DXCommon::GetInstance()->Finalize();
    PSOManager::GetInstance()->Finalize();

    // すべてのD3D12リソース破棄後に、DXGIDebugチェックを実行
    LeakCheck.reset();  // この時点で全リソースが破棄済み

    Audio::GetInstance()->Finalize();
    Input::GetInstance()->Finalize();
    PbdCommon::GetInstance()->Finalize();
    Object3dCommon::GetInstance()->Finalize();

    ImGuiManager::GetInstance()->Finalize();

    SpriteCommon::GetInstance()->Finalize();
    TextureManager::GetInstance()->Finalize();
    ModelManager::GetInstance()->Finalize();
    ParticleManager::GetInstance()->Finalize();

    WinApp::GetInstance()->Finalize();
}
void Framework::Update()
{
    //メッセージがある限りGetMessageを呼び出す
    if (WinApp::GetInstance()->ProcessMessage()) {
        endRequest_ = true;


    }
#ifdef USE_IMGUI
    ImGuiManager::GetInstance()->Begin();
#endif
    Input::GetInstance()->Update();
#ifdef USE_IMGUI
    if (Input::GetInstance()->TriggerKeyDown(DIK_F3)) {
        ImGuiManager::GetInstance()->ToggleShow();
    }
    if (Input::GetInstance()->TriggerKeyDown(DIK_F4)) {
        PrimitiveDrawer::GetInstance()->ToggleEnable();
    }
#endif
    Audio::GetInstance()->Update();
    LightManager::GetInstance()->Update();

     OffScreen::GetInstance()->Update();
}

void Framework::PreDraw()
{
    DXCommon::GetInstance()->PreDraw();
    SrvManager::GetInstance()->PreDraw();
}

void Framework::Draw()
{
    DXCommon::GetInstance()->SwapChainDraw();
    OffScreen::GetInstance()->Draw();

#ifdef USE_IMGUI
    ImGuiManager::GetInstance()->End();

    ImGuiManager::GetInstance()->Draw();
#endif
    DXCommon::GetInstance()->PostDraw();
    TextureManager::GetInstance()->ReleaseIntermediateResources();

    // 基底クラスの描画処理（純粋仮想関数として宣言されているため、必ずオーバーライドする必要があります）
}
void Framework::Run()
{
    Initialize();
    while (true) {
        Update();
        if (IsEnd()) {
            break;
        }
        PreDraw();
        Draw();
    }
    Finalize();
}
