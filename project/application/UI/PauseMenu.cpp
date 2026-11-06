#include "PauseMenu.h"
#include "GameScene.h"
#include "Input.h"
#include "Audio.h"
#include "SceneManager.h"
#include "WinApp.h"
#include "imgui.h"
#include "ImGuiManager.h"

void PauseMenu::Initialize(GameScene* gameScene)
{
    gameScene_ = gameScene;
    isPaused_ = false;
    selectedIndex_ = 0;

    // 効果音のロード（存在する場合）
    selectSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
    decideSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
}

void PauseMenu::SetPaused(bool paused)
{
    if (isPaused_ == paused) return;
    isPaused_ = paused;

    if (isPaused_) {
        ImGuiManager::GetInstance()->SetShow(true);
        selectedIndex_ = 0;
        if (gameScene_) {
            Audio::GetInstance()->PauseAudio(gameScene_->getBGMPlayHundle());
        }
    } else {
        if (gameScene_) {
            Audio::GetInstance()->ResumeAudio(gameScene_->getBGMPlayHundle());
        }
    }
}

void PauseMenu::Update()
{
    Input* input = Input::GetInstance();

    // ポーズ切り替え（ESCキー / Pキー / パッドSTART）
    if (input->TriggerKeyDown(DIK_ESCAPE) || input->TriggerKeyDown(DIK_P) || input->TriggerPadDown(0, XINPUT_GAMEPAD_START)) {
        SetPaused(!isPaused_);
        return;
    }

    if (!isPaused_) return;

    // 上下選択
    bool moved = false;
    if (input->TriggerKeyDown(DIK_UP) || input->TriggerKeyDown(DIK_W) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP)) {
        selectedIndex_ = (selectedIndex_ - 1 + static_cast<int>(MenuAction::Count)) % static_cast<int>(MenuAction::Count);
        moved = true;
    }
    if (input->TriggerKeyDown(DIK_DOWN) || input->TriggerKeyDown(DIK_S) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN)) {
        selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(MenuAction::Count);
        moved = true;
    }

    if (moved && selectSE_ != 0) {
        Audio::GetInstance()->PlayAudio(selectSE_, false, 0.4f, "SE");
    }

    // 決定（SPACE / RETURN / パッドA）
    if (input->TriggerKeyDown(DIK_SPACE) || input->TriggerKeyDown(DIK_RETURN) || input->TriggerPadDown(0, XINPUT_GAMEPAD_A)) {
        if (decideSE_ != 0) {
            Audio::GetInstance()->PlayAudio(decideSE_, false, 0.6f, "SE");
        }

        switch (static_cast<MenuAction>(selectedIndex_)) {
        case MenuAction::Resume:
            SetPaused(false);
            break;
        case MenuAction::Retry:
            SetPaused(false);
            if (gameScene_ && gameScene_->GetSceneManager()) {
                gameScene_->GetSceneManager()->ChangeScene("GAME");
            }
            break;
        case MenuAction::Title:
            SetPaused(false);
            if (gameScene_ && gameScene_->GetSceneManager()) {
                gameScene_->GetSceneManager()->ChangeScene("TITLE");
            }
            break;
        default:
            break;
        }
    }
}

void PauseMenu::Draw()
{
    if (!isPaused_) return;

    // 全画面の半透明暗転オーバーレイ & ポーズメニュー描画
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight)), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.65f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("PauseOverlay", nullptr, flags)) {
        float centerX = static_cast<float>(WinApp::kClientWidth) * 0.5f;
        float centerY = static_cast<float>(WinApp::kClientHeight) * 0.35f;

        // タイトル表示
        ImGui::SetWindowFontScale(2.5f);
        const char* title = "=== PAUSE ===";
        ImVec2 titleSize = ImGui::CalcTextSize(title);
        ImGui::SetCursorPos(ImVec2(centerX - titleSize.x * 0.5f, centerY));
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", title);

        // 選択項目表示
        ImGui::SetWindowFontScale(1.8f);
        for (int i = 0; i < static_cast<int>(MenuAction::Count); ++i) {
            std::string label = (i == selectedIndex_) ? "> " + menuLabels_[i] + " <" : "  " + menuLabels_[i] + "  ";
            ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            ImGui::SetCursorPos(ImVec2(centerX - textSize.x * 0.5f, centerY + 80.0f + i * 50.0f));

            if (i == selectedIndex_) {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", label.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", label.c_str());
            }
        }
        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::End();
}

