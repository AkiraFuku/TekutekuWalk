#include "GoalDemoScene.h"
#include "ClearPhase.h"
#include "ClearScene.h"
#include "Phase.h"
#include "SceneManager.h"
#include "LightManager.h"
#include "Object3dCommon.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "ParticleManager.h"
#include "CollisionManager.h"
#include "Collider.h"
#include "RailPath.h"
#include "Input.h"
#include "Fade.h"
#include "MathFunction.h"
#include <algorithm>
#include <cmath>
#include <numbers>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

GoalDemoScene::GoalDemoScene() = default;
GoalDemoScene::~GoalDemoScene() = default;

void GoalDemoScene::Initialize()
{
    // 1. メインカメラ生成
    auto mainCamera = std::make_unique<Camera>();
    mainCamera->SetTranslate({ 0.0f, 2.0f, -6.0f });
    cameraMap_["Main"] = std::move(mainCamera);
    ChangeActiveCamera(cameraMap_["Main"].get());

    // 2. ライト＆パーティクルカメラ設定
    LightManager::GetInstance()->AddDirectionalLight({ 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }, 1.0f);
    ParticleManager::GetInstance()->SetCamera(activeCamera_);

    // 3. 共通テクスチャ・モデル読み込み
    TextureManager::GetInstance()->LoadTexture("resources/grass.png");
    TextureManager::GetInstance()->LoadTexture("resources/white.png");
    TextureManager::GetInstance()->LoadTexture("resources/circle2.png");
    TextureManager::GetInstance()->LoadTexture("resources/gradationLine.png");
    ModelManager::GetInstance()->LoadModel("resources/player/", "player.obj");

    // 4. スカイボックス初期化
    skyBox_ = std::make_unique<SkyBox>();
    skyBox_->Initialize();
    skyBox_->SetCamera(activeCamera_);
    skyBox_->SetTextureByFilePath("resources/output_skybox.dds");
    Object3dCommon::GetInstance()->SetDefaultSkyBox(skyBox_.get());

    // 5. 専用デモステージ読み込み
    stageManager_ = std::make_unique<StageManager>();
    stageManager_->Load(stagePath_, activeCamera_);

    spawnRailPos_ = stageManager_->GetPlayerSpawnRailPosition();
    goalRailPos_ = stageManager_->GetGoalRailPosition();

    // 6. プレイヤー初期化
    player_ = std::make_unique<Player>();
    player_->Initialize();
    player_->SetCamera(activeCamera_);
    player_->SetScene(this);
    player_->SetPosition(stageManager_->GetPlayerSpawnPosition());
    if (player_->GetObject3d()) {
        playerBaseScale_ = player_->GetObject3d()->GetScale();
    }

    // 7. ゴールオブジェクト初期化
    goal_ = std::make_unique<GoalObject>();
    goal_->Initialize();
    goal_->SetCamera(activeCamera_);
    if (goal_->GetObject3d()) {
        goalBaseScale_ = goal_->GetObject3d()->GetScale();
    }

    // 8. カメラコントローラー初期化
    cameraController_ = std::make_unique<CameraController>();
    cameraController_->Initialize(cameraMap_["Main"].get());
    cameraController_->SetTarget(player_.get());
    cameraController_->SetStageRail(stageManager_->GetStageRail());
    cameraController_->SetAutoOffsetParams(
        stageManager_->GetStageCameraDistance(),
        stageManager_->GetStageCameraHeight()
    );
    cameraController_->SetDrawDistance(stageManager_->GetStageCameraDrawDistance());

    if (stageManager_->GetStageCameraMode() == "CAMERA_RAIL" && stageManager_->GetCameraRail()) {
        cameraController_->SetCameraMode(CameraController::CameraMode::RailCamera);
        cameraController_->SetRailPath(stageManager_->GetCameraRail());
    } else {
        cameraController_->SetCameraMode(CameraController::CameraMode::AutoOffset);
        if (stageManager_->GetCameraRail()) {
            cameraController_->SetRailPath(stageManager_->GetCameraRail());
        }
    }

    ResetDemo();
    Fade::GetInstance()->StartFadeIn(0.4f);
}

void GoalDemoScene::Finalize()
{
    if (currentPhase_) {
        currentPhase_->Finalize(this);
        currentPhase_.reset();
    }
    activeClearPhase_ = nullptr;

    if (BGMHandle_ != 0 && Audio::GetInstance()->IsPlaying(BGMHandle_)) {
        Audio::GetInstance()->StopAudio(BGMHandle_);
    }
}

void GoalDemoScene::ResetDemo()
{
    if (currentPhase_) {
        currentPhase_->Finalize(this);
        currentPhase_.reset();
    }
    activeClearPhase_ = nullptr;

    RailPath* stageRail = stageManager_ ? stageManager_->GetStageRail() : nullptr;

    currentRailProgress_ = spawnRailPos_.x;
    finishedWaitTimer_ = 0.0f;
    demoState_ = DemoState::Approaching;

    if (player_) {
        player_->ResetHitInvincibility();
        if (stageRail) {
            player_->SetRail(stageRail);
            player_->SetRailPosition(spawnRailPos_);
        }
        if (player_->GetObject3d()) {
            player_->GetObject3d()->ResetSubParts();
            player_->GetObject3d()->SetScale(playerBaseScale_);
            player_->GetObject3d()->SetRotate({ 0.0f, 0.0f, 0.0f });
            player_->GetObject3d()->Update();
        }
    }

    if (goal_) {
        goal_->ResetCleared();
        if (stageRail) {
            goal_->SetRail(stageRail);
            goal_->SetRailPosition(goalRailPos_);
        }
        if (goal_->GetObject3d()) {
            goal_->GetObject3d()->SetScale(goalBaseScale_);
            goal_->GetObject3d()->SetRotate({ 0.0f, 0.0f, 0.0f });
            goal_->GetObject3d()->Update();
        }
    }

    if (cameraController_) {
        cameraController_->SetTarget(player_.get());
        cameraController_->Update();
    }
}

void GoalDemoScene::StartClearSequence()
{
    if (demoState_ == DemoState::PlayingClear) {
        return;
    }

    demoState_ = DemoState::PlayingClear;

    ClearScene::SetAutoReturnToGoalDemo(transitionToClearScene_ && loopPlayback_, 4.0f);

    auto clearPhase = std::make_unique<ClearPhase>();
    clearPhase->SetDemoMode(true, [this]() {
        demoState_ = DemoState::FinishedWait;
        finishedWaitTimer_ = 0.0f;
    });
    clearPhase->SetTransitionToClearSceneInDemo(transitionToClearScene_);
    clearPhase->SetTimeScale(timeScale_);
    clearPhase->SetPaused(isPaused_);

    activeClearPhase_ = clearPhase.get();
    ChangePhase(std::move(clearPhase));
}

void GoalDemoScene::TriggerGoalNow()
{
    if (demoState_ == DemoState::PlayingClear) {
        ResetDemo();
    }

    RailPath* stageRail = stageManager_ ? stageManager_->GetStageRail() : nullptr;
    if (player_ && stageRail) {
        currentRailProgress_ = goalRailPos_.x;
        player_->SetRailPosition({ currentRailProgress_, spawnRailPos_.y });
    }
    if (cameraController_) {
        cameraController_->Update();
    }
    StartClearSequence();
}

void GoalDemoScene::UpdateApproaching(float dt)
{
    if (goal_) {
        goal_->Update();
    }

    if (autoApproach_) {
        RailPath* stageRail = stageManager_ ? stageManager_->GetStageRail() : nullptr;
        if (player_ && stageRail) {
            currentRailProgress_ += approachSpeed_ * dt;
            if (currentRailProgress_ >= goalRailPos_.x) {
                currentRailProgress_ = goalRailPos_.x;
            }
            player_->SetRailPosition({ currentRailProgress_, spawnRailPos_.y });
            if (player_->GetObject3d()) {
                // 進行方向を向かせる（直線Z+方向）
                player_->GetObject3d()->SetRotate({ 0.0f, 0.0f, 0.0f });
                player_->GetObject3d()->Update();
            }

            if (currentRailProgress_ >= goalRailPos_.x - 0.01f) {
                StartClearSequence();
                return;
            }
        }
    } else {
        // 手動操作モード
        if (player_) {
            player_->Update();
        }
        std::vector<Collider*> colliders;
        if (player_ && player_->GetCollider()) {
            colliders.push_back(player_->GetCollider());
        }
        if (goal_ && goal_->GetCollider()) {
            colliders.push_back(goal_->GetCollider());
        }
        CollisionManager::GetInstance()->CheckAllCollisions(colliders);

        if (goal_ && goal_->IsCleared()) {
            StartClearSequence();
            return;
        }
    }

    if (cameraController_) {
        cameraController_->Update();
    }
}

void GoalDemoScene::Update()
{
    Input* input = Input::GetInstance();

    // キーボードショートカット
    if (input->TriggerKeyDown(DIK_R)) {
        ResetDemo();
    }
    if (input->TriggerKeyDown(DIK_P)) {
        isPaused_ = !isPaused_;
        if (activeClearPhase_) {
            activeClearPhase_->SetPaused(isPaused_);
        }
    }
    if (input->TriggerKeyDown(DIK_G)) {
        TriggerGoalNow();
    }
    if (input->TriggerKeyDown(DIK_ESCAPE)) {
        ClearScene::SetAutoReturnToGoalDemo(false);
        SceneManager::GetInstance()->ChangeScene("StageSelectScene");
        return;
    }

    if (activeClearPhase_) {
        activeClearPhase_->SetTimeScale(timeScale_);
        activeClearPhase_->SetPaused(isPaused_);
        activeClearPhase_->SetTransitionToClearSceneInDemo(transitionToClearScene_);
        if (stepFrameRequested_) {
            activeClearPhase_->StepFrame();
        }
    }

    const bool canStep = !isPaused_ || stepFrameRequested_;
    const float dt = (1.0f / 60.0f) * (stepFrameRequested_ ? 1.0f : timeScale_);
    stepFrameRequested_ = false;

    if (canStep) {
        if (stageManager_ && player_) {
            stageManager_->Update(player_->GetWorldPosition());
        }

        switch (demoState_) {
        case DemoState::Approaching:
            UpdateApproaching(dt);
            break;

        case DemoState::PlayingClear:
            if (currentPhase_) {
                currentPhase_->Update(this);
            }
            break;

        case DemoState::FinishedWait:
            finishedWaitTimer_ += (1.0f / 60.0f);
            if (loopPlayback_ && finishedWaitTimer_ >= loopWaitDuration_) {
                ResetDemo();
            }
            break;
        }

        ParticleManager::GetInstance()->Update();
    }

    if (skyBox_ && activeCamera_) {
        skyBox_->SetTranslate(activeCamera_->GetTranslate());
        skyBox_->Update();
    }

#ifdef USE_IMGUI
    DrawImGui();
#endif
}

void GoalDemoScene::Draw()
{
    if (skyBox_) {
        skyBox_->Draw();
    }
    if (stageManager_) {
        stageManager_->Draw();
    }
    if (player_) {
        player_->Draw();
    }
    if (goal_) {
        goal_->Draw();
    }

    ParticleManager::GetInstance()->Draw();

    if (currentPhase_) {
        currentPhase_->Draw(this);
    }
}

const std::vector<Triangle>& GoalDemoScene::GetTriangle() const
{
    if (stageManager_) {
        return stageManager_->GetAllTriangles();
    }
    static const std::vector<Triangle> empty;
    return empty;
}

void GoalDemoScene::DrawImGui()
{
#ifdef USE_IMGUI
    ImGui::Begin("Goal Demo Control");

    const char* stateLabel = "Unknown";
    if (demoState_ == DemoState::Approaching) {
        stateLabel = autoApproach_ ? "Approaching (Auto)" : "Approaching (Manual)";
    } else if (demoState_ == DemoState::PlayingClear) {
        if (activeClearPhase_) {
            switch (activeClearPhase_->GetCurrentStep()) {
            case ClearPhase::Step::GoalZoom: stateLabel = "ClearPhase: GoalZoom"; break;
            case ClearPhase::Step::IrisClose: stateLabel = "ClearPhase: IrisClose"; break;
            case ClearPhase::Step::ChangeScene: stateLabel = "ClearPhase: Done"; break;
            }
        } else {
            stateLabel = "PlayingClear";
        }
    } else if (demoState_ == DemoState::FinishedWait) {
        stateLabel = "Finished (Waiting)";
    }

    ImGui::Text("State: %s", stateLabel);
    if (activeClearPhase_ && demoState_ == DemoState::PlayingClear) {
        float progress = 0.0f;
        if (activeClearPhase_->GetCurrentStep() == ClearPhase::Step::GoalZoom) {
            progress = std::clamp(activeClearPhase_->GetTimer() / ClearPhase::GetZoomDuration(), 0.0f, 1.0f);
            ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "Zoom & Hoist");
        } else if (activeClearPhase_->GetCurrentStep() == ClearPhase::Step::IrisClose) {
            progress = activeClearPhase_->GetIrisProgress();
            ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "Iris Wipe");
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Replay (R)")) {
        ResetDemo();
    }
    ImGui::SameLine();
    if (ImGui::Button("Trigger Goal Now (G)")) {
        TriggerGoalNow();
    }

    if (ImGui::Button(isPaused_ ? "Resume (P)" : "Pause (P)")) {
        isPaused_ = !isPaused_;
    }
    if (isPaused_) {
        ImGui::SameLine();
        if (ImGui::Button("Step Frame")) {
            stepFrameRequested_ = true;
        }
    }

    ImGui::Separator();
    ImGui::Checkbox("Auto Approach", &autoApproach_);
    ImGui::Checkbox("Loop Playback", &loopPlayback_);
    ImGui::Checkbox("Transition to ClearScene (Test)", &transitionToClearScene_);

    ImGui::SliderFloat("Time Scale", &timeScale_, 0.1f, 2.5f, "%.2fx");
    if (ImGui::Button("0.25x")) timeScale_ = 0.25f;
    ImGui::SameLine();
    if (ImGui::Button("0.5x")) timeScale_ = 0.5f;
    ImGui::SameLine();
    if (ImGui::Button("1.0x")) timeScale_ = 1.0f;
    ImGui::SameLine();
    if (ImGui::Button("2.0x")) timeScale_ = 2.0f;

    ImGui::SliderFloat("Approach Speed", &approachSpeed_, 0.05f, 1.0f, "%.2f");
    ImGui::SliderFloat("Loop Wait (s)", &loopWaitDuration_, 0.0f, 3.0f, "%.2f");

    ImGui::Separator();
    if (ImGui::Button("Back to Stage Select (ESC)")) {
        SceneManager::GetInstance()->ChangeScene("StageSelectScene");
    }

    ImGui::End();
#endif
}
