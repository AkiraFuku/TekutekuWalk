#include "StageSelectScene.h"
#include "Phase.h"
#include "SceneManager.h"
#include "GameScene.h"
#include "TextureManager.h"
#include "LightManager.h"
#include "Object3dCommon.h"
#include "Input.h"
#include "Fade.h"
#include "imgui.h"

StageSelectScene::StageSelectScene() = default;
StageSelectScene::~StageSelectScene() = default;

void StageSelectScene::Initialize()
{
    // カメラ初期化
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 0.0f, 2.0f, -6.0f });
    cameraMap_["Main"] = std::move(camera_);
    ChangeActiveCamera(cameraMap_["Main"].get());

    // ライト初期化
    LightManager::GetInstance()->AddDirectionalLight({ 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, 1.0f);

    // スカイボックス初期化
    skyBox_ = std::make_unique<SkyBox>();
    skyBox_->Initialize();
    skyBox_->SetCamera(activeCamera_);
    skyBox_->SetTextureByFilePath("resources/output_skybox.dds");
    Object3dCommon::GetInstance()->SetDefaultSkyBox(skyBox_.get());

    // ステージ情報の構築
    stageList_ = {
        { "Stage 1", "resources/Stagemap/stage1.json", "チュートリアル・基本ステージ" },
        { "Stage 2", "resources/Stagemap/stage2.json", "中級ステージ 2" },
        { "Stage 3", "resources/Stagemap/stage3.json", "上級ステージ 3" },
        { "Stage 4", "resources/Stagemap/stage4.json", "高難易度ステージ 4" },
        { "Stage 5", "resources/Stagemap/stage5.json", "巨大ステージ 5" },
        { "Test Stage", "resources/Stagemap/test_stage.json", "開発・テスト用ステージ" },
    };

    // フェードイン開始
    Fade::GetInstance()->StartFadeIn(0.5f);
    isTransitioning_ = false;
}

void StageSelectScene::Finalize()
{
}

void StageSelectScene::Update()
{
    if (activeCamera_) {
        activeCamera_->Update();
    }

    auto* input = Input::GetInstance();

    // 遷移中でなければ入力を受け付ける
    if (!isTransitioning_ && !Fade::GetInstance()->IsFading()) {
        // キーボード/パッドでの選択操作
        if (input->TriggerKeyDown(DIK_UP) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP)) {
            selectedStageIndex_ = (selectedStageIndex_ - 1 + static_cast<int>(stageList_.size())) % static_cast<int>(stageList_.size());
        }
        if (input->TriggerKeyDown(DIK_DOWN) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN)) {
            selectedStageIndex_ = (selectedStageIndex_ + 1) % static_cast<int>(stageList_.size());
        }

        // 決定 (Enter / Space / Aボタン)
        if (input->TriggerKeyDown(DIK_RETURN) || input->TriggerKeyDown(DIK_SPACE) || input->TriggerPadDown(0, XINPUT_GAMEPAD_A)) {
            isTransitioning_ = true;
            nextSceneName_ = "GameScene";
            GameScene::SetSelectedStagePath(stageList_[selectedStageIndex_].filePath);
            Fade::GetInstance()->StartFadeOut(0.5f);
        }

        // 戻る (Escape / Bボタン)
        if (input->TriggerKeyDown(DIK_ESCAPE) || input->TriggerPadDown(0, XINPUT_GAMEPAD_B)) {
            isTransitioning_ = true;
            nextSceneName_ = "TitleScene";
            Fade::GetInstance()->StartFadeOut(0.5f);
        }
    }

    // フェード完了後のシーン遷移
    if (isTransitioning_ && !Fade::GetInstance()->IsFading()) {
        SceneManager::GetInstance()->ChangeScene(nextSceneName_);
        return;
    }

    // --- ImGui ステージ選択 UI ---
#ifdef USE_IMGUI
    ImGui::Begin("Stage Select");
    ImGui::Text("Select Stage to Play:");
    ImGui::Separator();

    for (int i = 0; i < static_cast<int>(stageList_.size()); ++i) {
        if (ImGui::RadioButton(stageList_[i].name.c_str(), &selectedStageIndex_, i)) {
            // 選択更新
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", stageList_[i].description.c_str());
    }

    ImGui::Separator();
    ImGui::Text("Target File: %s", stageList_[selectedStageIndex_].filePath.c_str());

    if (ImGui::Button("Game Start", ImVec2(120, 30))) {
        if (!isTransitioning_) {
            isTransitioning_ = true;
            nextSceneName_ = "GameScene";
            GameScene::SetSelectedStagePath(stageList_[selectedStageIndex_].filePath);
            Fade::GetInstance()->StartFadeOut(0.5f);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Back to Title", ImVec2(120, 30))) {
        if (!isTransitioning_) {
            isTransitioning_ = true;
            nextSceneName_ = "TitleScene";
            Fade::GetInstance()->StartFadeOut(0.5f);
        }
    }

    ImGui::End();
#endif
}

void StageSelectScene::Draw()
{
    if (skyBox_) {
        skyBox_->Draw();
    }
}
