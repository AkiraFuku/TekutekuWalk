#pragma once
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <stack>

class GameScene;

class PauseMenu
{
public:
    // メニュー階層ID（スライドに準拠）
    enum class MenuId {
        PauseMain,  // メインポーズメニュー
        Options,    // 設定メニュー
        Sound       // サウンド設定
    };

    // 画面状態（ID + カーソル位置）
    struct MenuState {
        MenuId menuId;
        int selectedIndex = 0;
    };

    // トップメニュー項目
    enum class MainAction {
        Resume = 0,
        Options,
        Retry,
        Title,
        Count
    };

    // 設定メニュー項目
    enum class OptionAction {
        Sound = 0,
        Back,
        Count
    };

    // サウンド設定項目
    enum class SoundAction {
        BgmVolume = 0,
        SeVolume,
        Back,
        Count
    };

    void Initialize(GameScene* gameScene);
    void Update();
    void Draw();

    bool IsPaused() const { return isPaused_; }
    void SetPaused(bool paused);

private:
    GameScene* gameScene_ = nullptr;
    bool isPaused_ = false;

    // 履歴管理用スタック
    std::stack<MenuState> menuStack_;

    // 各階層のラベル
    const std::vector<std::string> mainLabels_ = {
        "RESUME",
        "OPTIONS",
        "RETRY",
        "TITLE"
    };

    const std::vector<std::string> optionLabels_ = {
        "SOUND SETTINGS",
        "BACK"
    };

    const std::vector<std::string> soundLabels_ = {
        "BGM VOLUME",
        "SE VOLUME",
        "BACK"
    };

    // 音量パラメータ (0.0f ~ 1.0f)
    float bgmVolume_ = 0.8f;
    float seVolume_ = 1.0f;

    uint32_t selectSE_ = 0;
    uint32_t decideSE_ = 0;
    uint32_t cancelSE_ = 0;

    // スタック操作ヘルパー
    void PopMenu();
    void ClearMenu();

    // 階層別更新
    void UpdateMainMenu();
    void UpdateOptionsMenu();
    void UpdateSoundMenu();

    // 階層別描画
    void DrawMenuItems(const std::string& title, const std::vector<std::string>& labels, int selectedIndex);
};
