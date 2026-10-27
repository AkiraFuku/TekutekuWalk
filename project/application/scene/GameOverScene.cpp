#include "GameOverScene.h"
#include "Phase.h"
#include "Sprite.h"
#include "WinApp.h"
#include "SceneManager.h"
#include "Input.h"
#include "Fade.h"
#include "TextureManager.h"
#include "Object3dCommon.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

GameOverScene::GameOverScene() = default;
GameOverScene::~GameOverScene() = default;

void GameOverScene::Initialize()
{
    camera_ = std::make_unique<Camera>();
    camera_->SetTranslate({ 0.0f, 0.0f, -5.0f });
    cameraMap_["Main"] = std::move(camera_);
    ChangeActiveCamera(cameraMap_["Main"].get());

    skyBox_ = std::make_unique<SkyBox>();
    skyBox_->Initialize();
    skyBox_->SetCamera(activeCamera_);
    skyBox_->SetTextureByFilePath("resources/output_skybox.dds");
    Object3dCommon::GetInstance()->SetDefaultSkyBox(skyBox_.get());

    Fade::GetInstance()->StartFadeIn(1.0f);

    isTransitioning_ = false;
    selectedMenu_ = MenuItem::StageSelect;

    // ゲームオーバーロゴスプライト
    gameOverLogo_ = std::make_unique<Sprite>();
    gameOverLogo_->Initialize("resources/GAMOVER/GameOver.png");
    gameOverLogo_->SetPosition(WinApp::GetInstance()->GetWindowCenter());
    gameOverLogo_->SetAnchorPoint({ 0.5f, 0.5f });

    decisionSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
}

void GameOverScene::Update()
{
    activeCamera_->Update();
    skyBox_->SetTranslate(activeCamera_->GetTranslate());
    skyBox_->Update();

    if (gameOverLogo_) {
        gameOverLogo_->Update();
    }

    Input* input = Input::GetInstance();

    // ── ImGui デバッグウィンドウ ──
#ifdef USE_IMGUI
    ImGui::Begin("Game Over Scene (Debug Window)");
    ImGui::Text("=== GAME OVER SCENE ===");

    bool triggerStageSelect = false;
    bool triggerTitle = false;

    const char* menuNames[] = { "Stage Select Scene", "Title Scene" };
    int currentItem = static_cast<int>(selectedMenu_);
    if (ImGui::Combo("Next Destination", &currentItem, menuNames, IM_ARRAYSIZE(menuNames))) {
        selectedMenu_ = static_cast<MenuItem>(currentItem);
    }

    ImGui::Separator();
    if (ImGui::Button("Go to Stage Select", ImVec2(200, 40))) {
        selectedMenu_ = MenuItem::StageSelect;
        triggerStageSelect = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Go to Title", ImVec2(200, 40))) {
        selectedMenu_ = MenuItem::Title;
        triggerTitle = true;
    }
    ImGui::End();
#endif

    if (!isTransitioning_) {
        // キーボード（↑/↓、W/S）またはパッド十字キーで選択切り替え
        bool moveUp = input->TriggerKeyDown(DIK_UP) || input->TriggerKeyDown(DIK_W) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP);
        bool moveDown = input->TriggerKeyDown(DIK_DOWN) || input->TriggerKeyDown(DIK_S) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN);

        if (moveUp) {
            selectedMenu_ = MenuItem::StageSelect;
        } else if (moveDown) {
            selectedMenu_ = MenuItem::Title;
        }

        // 決定操作（SPACE, ENTER, パッドA, またはImGuiボタン）
        bool isPressed = input->TriggerKeyDown(DIK_SPACE) ||
                         input->TriggerKeyDown(DIK_RETURN) ||
                         input->TriggerPadDown(0, XINPUT_GAMEPAD_A) ||
                         input->TriggerPadDown(0, XINPUT_GAMEPAD_START)
#ifdef USE_IMGUI
                         || triggerStageSelect || triggerTitle
#endif
                         ;

        if (isPressed) {
            isTransitioning_ = true;
            if (decisionSE_ != 0) {
                Audio::GetInstance()->PlayAudio(decisionSE_, false, 0.8f, "SE");
            }
            Fade::GetInstance()->StartFadeOut(1.0f);
        }
    } else {
        if (!Fade::GetInstance()->IsFading()) {
            if (selectedMenu_ == MenuItem::StageSelect) {
                SceneManager::GetInstance()->ChangeScene("StageSelectScene");
            } else {
                SceneManager::GetInstance()->ChangeScene("TitleScene");
            }
        }
    }
}

void GameOverScene::Draw()
{
    skyBox_->Draw();
    if (gameOverLogo_) {
        gameOverLogo_->Draw();
    }
}

void GameOverScene::Finalize()
{
    gameOverLogo_.reset();
    skyBox_.reset();
}
