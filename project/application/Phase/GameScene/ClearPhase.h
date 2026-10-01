#pragma once
#include "Phase.h"
#include "Audio.h"
#include "Vector2.h"
#include <memory>

class Sprite;

class ClearPhase :
    public Phase
{
public:
    void Initialize(Scene* scene) override;
    void Update(Scene* scene) override;
    void Draw(Scene* scene) override;
    void Finalize(Scene* scene) override;

private:
    // ステージクリアスプライト (tekutekuClear.png)
    std::unique_ptr<Sprite> clearSprite_;

    // ボタン入力指示スプライト (Start.png)
    std::unique_ptr<Sprite> startSprite_;
    float blinkTimer_ = 0.0f;
    float currentBlinkSpeed_ = 4.0f; // 点滅角速度 (通常時: 約1.6秒周期)
    Vector2 baseSize_ = { 0.0f, 0.0f };

    Audio::SoundHandle ClearSE = 0;
    Audio::VoiceHandle Play_ = 0;
    Audio::SoundHandle decisionSE_ = 0;

    bool isTransitioning_ = false;
};

