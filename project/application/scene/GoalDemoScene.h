#pragma once
#include "Scene.h"
#include "Camera.h"
#include "SkyBox.h"
#include "Player.h"
#include "GoalObject.h"
#include "CameraController.h"
#include "StageManager.h"
#include "Vector2.h"
#include "Vector3.h"
#include <memory>
#include <string>

class ClearPhase;

/// <summary>
/// ゴール演出（ClearPhase）の確認・検証・調整を行う専用デモシーン
/// </summary>
class GoalDemoScene : public Scene
{
public:
    enum class DemoState {
        Approaching,   // ゴールに向かって移動中（自動または手動）
        PlayingClear,  // ClearPhase（ゴール演出）再生中
        FinishedWait   // 演出終了後の待機（ループ再生待ち）
    };

    GoalDemoScene();
    ~GoalDemoScene() override;

    void Initialize() override;
    void Finalize() override;
    void Update() override;
    void Draw() override;

    // Scene 共通インターフェース
    Player* GetPlayer() override { return player_.get(); }
    GoalObject* GetGoal() override { return goal_.get(); }
    CameraController* GetCamera() override { return cameraController_.get(); }
    const std::vector<Triangle>& GetTriangle() const override;
    bool IsGameTimeStopped() const override { return isPaused_; }

private:
    void ResetDemo();
    void StartClearSequence();
    void TriggerGoalNow();
    void UpdateApproaching(float dt);
    void DrawImGui();

    std::unique_ptr<StageManager> stageManager_;
    std::unique_ptr<Player> player_;
    std::unique_ptr<GoalObject> goal_;
    std::unique_ptr<CameraController> cameraController_;
    std::unique_ptr<SkyBox> skyBox_;

    std::string stagePath_ = "resources/Stagemap/goal_demo_stage.json";
    Vector2 spawnRailPos_ = { 0.05f, 0.5f };
    Vector2 goalRailPos_ = { 0.35f, 0.5f };
    Vector3 playerBaseScale_ = { 1.0f, 1.0f, 1.0f };
    Vector3 goalBaseScale_ = { 1.0f, 1.0f, 1.0f };

    DemoState demoState_ = DemoState::Approaching;
    ClearPhase* activeClearPhase_ = nullptr; // currentPhase_ の非所有ポインタ

    // デモ再生設定
    bool autoApproach_ = true;
    bool loopPlayback_ = true;
    bool transitionToClearScene_ = true;
    bool isPaused_ = false;
    bool stepFrameRequested_ = false;
    float timeScale_ = 1.0f;
    float approachSpeed_ = 0.22f; // レール進捗/秒
    float currentRailProgress_ = 0.05f;
    float finishedWaitTimer_ = 0.0f;
    float loopWaitDuration_ = 0.6f;
};
