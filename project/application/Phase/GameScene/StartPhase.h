#pragma once
#include "Phase.h"
#include "Vector3.h"
#include "Vector2.h"

class StartPhase : public Phase
{
public:
    ~StartPhase() override = default;

    void Initialize(Scene* scene) override;
    void Update(Scene* scene) override;
    void Draw(Scene* scene) override;
    void Finalize(Scene* scene) override;

private:
    float timer_ = 0.0f;
    const float kRunDuration_ = 1.4f;   // 走ってくる時間（秒）
    const float kWaitDuration_ = 0.3f;  // 到着後の短いタメ（秒）
    const float kFadeDuration_ = 0.8f;  // フェードイン時間（秒）

    Vector3 startWorldPos_{};
    Vector3 targetWorldPos_{};
    Vector2 targetRailPos_{};
    Vector3 forwardDir_{ 0.0f, 0.0f, 1.0f };
    bool isArrived_ = false;
};