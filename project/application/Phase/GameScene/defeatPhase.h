#pragma once
#include "Phase.h"
#include <memory>
#include "Audio.h"
#include "Vector2.h"
#include "Vector3.h"

class Sprite;

class defeatPhase :
    public Phase
{
public:
    enum class Step {
        DeathKnockback, // 吹っ飛び・バウンド演出
        SpriteDropIn,   // スプライト降下演出
        MenuSelect,     // メニュー選択待機
        FadeOut         // 決定後フェードアウト
    };

    enum class MenuItem {
        Retry = 0,
        Quit,
        Count
    };

    void Initialize(Scene* scene) override;
    void Update(Scene* scene) override;
    void Draw(Scene* scene) override;
    void Finalize(Scene* scene) override;

private:
    Step step_ = Step::DeathKnockback;

    // ── 死亡吹っ飛び・バウンド用パラメータ ──
    Vector3 playerAnimPos_ = { 0.0f, 0.0f, 0.0f };
    Vector3 playerAnimRot_ = { 0.0f, 0.0f, 0.0f };
    Vector3 playerBaseScale_ = { 1.0f, 1.0f, 1.0f };
    Vector3 knockbackDir_ = { 0.0f, 0.0f, 0.0f };
    float vy_ = 0.0f;
    float vx_ = 0.0f;
    float rotZSpeed_ = 0.0f;
    float groundY_ = 0.0f;
    float facingDir_ = 1.0f;
    int bounceCount_ = 0;
    float settleTimer_ = 0.0f;

    // ── カメラ画面中央固定用関数 ──
    void UpdateCameraFocus(Scene* scene);
    bool isFallDeath_ = false;

    // ── スプライト降下用パラメータ ──
    float dropTimer_ = 0.0f;
    const float kDropDuration_ = 0.65f;
    float spriteStartY_ = -200.0f;
    float spriteTargetY_ = 0.0f;

    // ── 死亡演出用ダークフェード ──
    std::unique_ptr<Sprite> darkFadeSprite_;
    float darkFadeAlpha_ = 0.0f;
    const float kMaxDarkFadeAlpha_ = 0.65f; // 薄っすら暗くする最大アルファ値

    // コンテニュー？スプライト (QContenue.png)
    std::unique_ptr<Sprite> defeatSprite_;
    Vector2 defeatSpriteBaseSize_ = { 576.0f, 136.5f };

    // 下部選択ボタンスプライト
    std::unique_ptr<Sprite> continueButtonSprite_; // resources/GAMOVER/continue.png
    std::unique_ptr<Sprite> noButtonSprite_;       // resources/GAMOVER/NO.png
    Vector2 continueButtonBaseSize_ = { 320.0f, 78.5f };
    Vector2 noButtonBaseSize_ = { 160.0f, 99.5f };

    // 選択項目
    MenuItem selectedMenu_ = MenuItem::Retry;

    // ボタン入力指示スプライト (Start.png / push.png)
    std::unique_ptr<Sprite> startSprite_;
    float blinkTimer_ = 0.0f;
    float currentBlinkSpeed_ = 4.0f; // 点滅角速度
    Vector2 baseSize_ = Vector2(0.0f, 0.0f);

    Audio::SoundHandle defeatSE = 0;
    Audio::VoiceHandle Play_ = 0;
    Audio::SoundHandle decisionSE_ = 0;

    bool isTransitioning_ = false;
};

