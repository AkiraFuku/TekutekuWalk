#pragma once
#include "Phase.h"
#include "Audio.h"
#include "Vector2.h"
#include "Vector3.h"
#include <memory>
#include <vector>

class Sprite;

class ClearPhase :
    public Phase
{
public:
    enum class Step {
        GoalZoom,       // ゴール地点でプレイヤーが正面を向いて喜び、カメラがズームイン
        IrisClose,      // 放射状（アイリス/トンネル風）トランジションで画面を覆う
        ChangeScene     // ClearSceneへ自動遷移
    };

    void Initialize(Scene* scene) override;
    void Update(Scene* scene) override;
    void Draw(Scene* scene) override;
    void Finalize(Scene* scene) override;

private:
    void UpdateCameraAndPlayer(Scene* scene, float dt);
    void InitializeIrisSprites();
    void UpdateIrisSprites(float progress);

    Step step_ = Step::GoalZoom;
    float timer_ = 0.0f;

    static constexpr float kZoomDuration_ = 1.55f;
    static constexpr float kIrisDuration_ = 0.60f;

    // カメラズーム補間用
    Vector3 startCamPos_ = { 0.0f, 0.0f, -10.0f };
    Vector3 targetCamPos_ = { 0.0f, 0.0f, -4.0f };
    Vector3 startLookAt_ = { 0.0f, 0.0f, 0.0f };
    Vector3 targetLookAt_ = { 0.0f, 0.0f, 0.0f };

    // プレイヤー＆ゴール持ち上げ演出補間用
    Vector3 playerStartPos_ = { 0.0f, 0.0f, 0.0f };
    Vector3 playerTargetPos_ = { 0.0f, 0.0f, 0.0f };
    Vector3 playerBaseScale_ = { 1.0f, 1.0f, 1.0f };
    float startPlayerYaw_ = 0.0f;
    float targetPlayerYaw_ = 0.0f;

    Vector3 goalStartPos_ = { 0.0f, 0.0f, 0.0f };
    Vector3 goalBaseScale_ = { 1.0f, 1.0f, 1.0f };
    float goalYaw_ = 0.0f;
    float goalHoistOffsetY_ = 1.25f; // プレイヤー中心から頭上ゴールまでの高さオフセット
    bool hasEmittedHoistEffect_ = false;

    // 放射状トランジション（先輩のゲームの00:09風の集中線・扇形ワイプ）
    static constexpr int kIrisBladeCount_ = 16;
    std::vector<std::unique_ptr<Sprite>> irisBlades_;
    std::unique_ptr<Sprite> irisCenterCircle_;
    float irisProgress_ = 0.0f;

    Audio::SoundHandle goalSE_ = 0;
};


