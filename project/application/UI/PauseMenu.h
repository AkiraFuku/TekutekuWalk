#pragma once
#include "Vector2.h"
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <stack>

class GameScene;
class Sprite;

class PauseMenu
{
public:
    // メニュー階層ID
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

    // アニメーションタイマー
    float animTimer_ = 0.0f;

    // 音量パラメータ (0.0f ~ 1.0f)
    float bgmVolume_ = 0.8f;
    float seVolume_ = 1.0f;

    uint32_t selectSE_ = 0;
    uint32_t decideSE_ = 0;
    uint32_t cancelSE_ = 0;

    // ─── スプライト ───
    std::unique_ptr<Sprite> backdropSprite_;    // 全画面暗幕
    std::unique_ptr<Sprite> boardBackSprite_;   // 外枠ボード（木目調）
    std::unique_ptr<Sprite> boardInnerSprite_;  // 内側パネル
    std::unique_ptr<Sprite> headerPlateSprite_; // タイトル背景プレート

    // タイトル
    std::unique_ptr<Sprite> pauseTitleSprite_;
    std::unique_ptr<Sprite> optionsTitleSprite_;
    std::unique_ptr<Sprite> soundTitleSprite_;

    // メインメニュー項目
    std::unique_ptr<Sprite> resumeSprite_;
    std::unique_ptr<Sprite> optionsSprite_;
    std::unique_ptr<Sprite> retrySprite_;
    std::unique_ptr<Sprite> titleSprite_;

    // オプションメニュー項目
    std::unique_ptr<Sprite> soundSettingSprite_;
    std::unique_ptr<Sprite> optionBackSprite_;

    // サウンドメニュー項目
    std::unique_ptr<Sprite> bgmVolumeSprite_;
    std::unique_ptr<Sprite> seVolumeSprite_;
    std::unique_ptr<Sprite> soundBackSprite_;

    // 音量バー
    std::unique_ptr<Sprite> bgmBarBg_;
    std::unique_ptr<Sprite> bgmBarFill_;
    std::unique_ptr<Sprite> seBarBg_;
    std::unique_ptr<Sprite> seBarFill_;

    // カーソル（星）
    std::unique_ptr<Sprite> cursorLeft_;
    std::unique_ptr<Sprite> cursorRight_;

    // ヘルパー
    void SetupItemSprite(Sprite* sprite, const Vector2& centerPos, bool isSelected, const Vector2& baseSize);
    void PopMenu();
    void ClearMenu();

    void UpdateMainMenu();
    void UpdateOptionsMenu();
    void UpdateSoundMenu();
};
