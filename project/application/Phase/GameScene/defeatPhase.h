#pragma once
#include "Phase.h"
#include <memory>
#include "Audio.h"
#include "Vector2.h"

class Sprite;

class defeatPhase :
    public Phase
{
public:
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
    // ゲームオーバースプライト (tekutekuGameOver.png)
    std::unique_ptr<Sprite> defeatSprite_;

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

