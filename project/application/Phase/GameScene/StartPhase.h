#pragma once
#include "Phase.h"
#include "Vector3.h"
#include "Vector2.h"
#include "Sprite.h"
#include <memory>

class StartPhase : public Phase
{
public:
    ~StartPhase() override = default;

    void Initialize(Scene* scene) override;
    void Update(Scene* scene) override;
    void Draw(Scene* scene) override;
    void Finalize(Scene* scene) override;

private:
    // プレイヤーの移動入力が行われているかを判定
    bool HasMovementInput() const;

    float timer_ = 0.0f;
    float runProgress_ = 0.0f;          // プレイヤーの移動進行度 (0.0f -> 1.0f)
    float bannerTimer_ = 0.0f;          // スプライト専用の独立タイマー
    const float kBannerDuration_ = 4.0f; // スプライトが画面を通過する時間（秒）
    const float kRunDuration_ = 2.4f;   // 走ってくる時間（秒）
    const float kWaitDuration_ = 0.5f;  // 到着後の短いタメ（秒）
    const float kFadeDuration_ = 0.8f;  // フェードイン時間（秒）

    Vector3 startWorldPos_{};
    Vector3 targetWorldPos_{};
    Vector2 targetRailPos_{};
    Vector3 forwardDir_{ 0.0f, 0.0f, 1.0f };
    bool isArrived_ = false;

    // 画面上部を左から右へ流れる演出スプライト
    std::unique_ptr<Sprite> bannerSprite_;
    float bannerStartY_ = 100.0f;
    float bannerStartX_ = -200.0f;
    float bannerEndX_ = 1480.0f;
};