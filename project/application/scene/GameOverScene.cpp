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
    blinkTimer_ = 0.0f;

    // ゲームオーバーロゴスプライト（画面上部中央へ配置）
    gameOverLogo_ = std::make_unique<Sprite>();
    gameOverLogo_->Initialize("resources/GAMOVER/tekutekuGameOver.png");
    gameOverLogo_->SetSize(gameOverLogoBaseSize_);
    gameOverLogo_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.5f, static_cast<float>(WinApp::kClientHeight) * 0.32f });
    gameOverLogo_->SetAnchorPoint({ 0.5f, 0.5f });
    gameOverLogo_->Update();

    // ステージセレクトボタンスプライト（下部左側）
    stageSelectButtonSprite_ = std::make_unique<Sprite>();
    stageSelectButtonSprite_->Initialize("resources/GAMOVER/StageSerect.png");
    stageSelectButtonSprite_->SetSize(stageSelectBaseSize_);
    stageSelectButtonSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.36f, static_cast<float>(WinApp::kClientHeight) * 0.72f });
    stageSelectButtonSprite_->SetAnchorPoint({ 0.5f, 0.5f });
    stageSelectButtonSprite_->Update();

    // タイトルボタンスプライト（下部右側）
    titleButtonSprite_ = std::make_unique<Sprite>();
    titleButtonSprite_->Initialize("resources/GAMOVER/GoToTitle.png");
    titleButtonSprite_->SetSize(titleBaseSize_);
    titleButtonSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.64f, static_cast<float>(WinApp::kClientHeight) * 0.72f });
    titleButtonSprite_->SetAnchorPoint({ 0.5f, 0.5f });
    titleButtonSprite_->Update();

    decisionSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
}

void GameOverScene::Update()
{
    activeCamera_->Update();
    skyBox_->SetTranslate(activeCamera_->GetTranslate());
    skyBox_->Update();
    blinkTimer_ += 1.0f / 60.0f;

    if (gameOverLogo_) {
        gameOverLogo_->Update();
    }

    // 選択ボタンのアニメーション（拡大パルス演出＆明暗ハイライト）
    float pulse = 1.0f + 0.04f * std::sin(blinkTimer_ * 6.0f);
    if (stageSelectButtonSprite_) {
        bool isSelected = (selectedMenu_ == MenuItem::StageSelect);
        float scale = isSelected ? (1.12f * pulse) : 0.95f;
        stageSelectButtonSprite_->SetSize(stageSelectBaseSize_ * scale);
        stageSelectButtonSprite_->SetColor(isSelected ? Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } : Vector4{ 0.55f, 0.55f, 0.55f, 0.75f });
        stageSelectButtonSprite_->Update();
    }
    if (titleButtonSprite_) {
        bool isSelected = (selectedMenu_ == MenuItem::Title);
        float scale = isSelected ? (1.12f * pulse) : 0.95f;
        titleButtonSprite_->SetSize(titleBaseSize_ * scale);
        titleButtonSprite_->SetColor(isSelected ? Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } : Vector4{ 0.55f, 0.55f, 0.55f, 0.75f });
        titleButtonSprite_->Update();
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
        // キーボード（上下・左右、W/S/A/D）またはパッド十字キーで選択切り替え
        bool selectStage = input->TriggerKeyDown(DIK_UP) || input->TriggerKeyDown(DIK_LEFT) ||
                           input->TriggerKeyDown(DIK_W) || input->TriggerKeyDown(DIK_A) ||
                           input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_LEFT);
        bool selectTitle = input->TriggerKeyDown(DIK_DOWN) || input->TriggerKeyDown(DIK_RIGHT) ||
                           input->TriggerKeyDown(DIK_S) || input->TriggerKeyDown(DIK_D) ||
                           input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT);

        if (selectStage) {
            selectedMenu_ = MenuItem::StageSelect;
        } else if (selectTitle) {
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
    if (stageSelectButtonSprite_) {
        stageSelectButtonSprite_->Draw();
    }
    if (titleButtonSprite_) {
        titleButtonSprite_->Draw();
    }
}

void GameOverScene::Finalize()
{
    gameOverLogo_.reset();
    skyBox_.reset();
    stageSelectButtonSprite_.reset();
    titleButtonSprite_.reset();
}
