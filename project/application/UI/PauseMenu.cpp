#include "PauseMenu.h"
#include "GameScene.h"
#include "Input.h"
#include "Audio.h"
#include "SceneManager.h"
#include "WinApp.h"
#include "imgui.h"
#include "ImGuiManager.h"
#include <algorithm>

void PauseMenu::Initialize(GameScene* gameScene)
{
    gameScene_ = gameScene;
    isPaused_ = false;
    ClearMenu();

    // 効果音のロード
    selectSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
    decideSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
    cancelSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
}

void PauseMenu::ClearMenu()
{
    while (!menuStack_.empty()) {
        menuStack_.pop();
    }
}

void PauseMenu::PopMenu()
{
    if (menuStack_.empty()) return;

    if (cancelSE_ != 0) {
        Audio::GetInstance()->PlayAudio(cancelSE_, false, 0.5f, "SE");
    }

    menuStack_.pop();

    // 全て閉じたらポーズ解除
    if (menuStack_.empty()) {
        SetPaused(false);
    }
}

void PauseMenu::SetPaused(bool paused)
{
    if (isPaused_ == paused) return;
    isPaused_ = paused;

    ClearMenu();

    if (isPaused_) {
        ImGuiManager::GetInstance()->SetShow(true);
        // トップ画面を積んで開く
        menuStack_.push({ MenuId::PauseMain, 0 });

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

    // ポーズ切り替え（ESC / P / START）
    if (input->TriggerKeyDown(DIK_P) || input->TriggerPadDown(0, XINPUT_GAMEPAD_START)) {
        SetPaused(!isPaused_);
        return;
    }

    if (!isPaused_ || menuStack_.empty()) return;

    // キャンセルボタン（ESCキー / パッドBボタン）で直前へ戻る (pop)
    if (input->TriggerKeyDown(DIK_ESCAPE) || input->TriggerPadDown(0, XINPUT_GAMEPAD_B)) {
        PopMenu();
        return;
    }

    // 現在一番上にある画面に応じた処理
    MenuState& current = menuStack_.top();
    switch (current.menuId) {
    case MenuId::PauseMain:
        UpdateMainMenu();
        break;
    case MenuId::Options:
        UpdateOptionsMenu();
        break;
    case MenuId::Sound:
        UpdateSoundMenu();
        break;
    }
}

void PauseMenu::UpdateMainMenu()
{
    Input* input = Input::GetInstance();
    MenuState& current = menuStack_.top();
    int count = static_cast<int>(MainAction::Count);

    bool moved = false;
    if (input->TriggerKeyDown(DIK_UP) || input->TriggerKeyDown(DIK_W) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP)) {
        current.selectedIndex = (current.selectedIndex - 1 + count) % count;
        moved = true;
    }
    if (input->TriggerKeyDown(DIK_DOWN) || input->TriggerKeyDown(DIK_S) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN)) {
        current.selectedIndex = (current.selectedIndex + 1) % count;
        moved = true;
    }

    if (moved && selectSE_ != 0) {
        Audio::GetInstance()->PlayAudio(selectSE_, false, 0.4f, "SE");
    }

    // 決定
    if (input->TriggerKeyDown(DIK_SPACE) || input->TriggerKeyDown(DIK_RETURN) || input->TriggerPadDown(0, XINPUT_GAMEPAD_A)) {
        if (decideSE_ != 0) {
            Audio::GetInstance()->PlayAudio(decideSE_, false, 0.6f, "SE");
        }

        switch (static_cast<MainAction>(current.selectedIndex)) {
        case MainAction::Resume:
            SetPaused(false);
            break;
        case MainAction::Options:
            // 設定メニューを push（カーソルは0から開始）
            menuStack_.push({ MenuId::Options, 0 });
            break;
        case MainAction::Retry:
            SetPaused(false);
            if (gameScene_ && gameScene_->GetSceneManager()) {
                gameScene_->GetSceneManager()->ChangeScene("GAME");
            }
            break;
        case MainAction::Title:
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

void PauseMenu::UpdateOptionsMenu()
{
    Input* input = Input::GetInstance();
    MenuState& current = menuStack_.top();
    int count = static_cast<int>(OptionAction::Count);

    bool moved = false;
    if (input->TriggerKeyDown(DIK_UP) || input->TriggerKeyDown(DIK_W) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP)) {
        current.selectedIndex = (current.selectedIndex - 1 + count) % count;
        moved = true;
    }
    if (input->TriggerKeyDown(DIK_DOWN) || input->TriggerKeyDown(DIK_S) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN)) {
        current.selectedIndex = (current.selectedIndex + 1) % count;
        moved = true;
    }

    if (moved && selectSE_ != 0) {
        Audio::GetInstance()->PlayAudio(selectSE_, false, 0.4f, "SE");
    }

    // 決定
    if (input->TriggerKeyDown(DIK_SPACE) || input->TriggerKeyDown(DIK_RETURN) || input->TriggerPadDown(0, XINPUT_GAMEPAD_A)) {
        if (decideSE_ != 0) {
            Audio::GetInstance()->PlayAudio(decideSE_, false, 0.6f, "SE");
        }

        switch (static_cast<OptionAction>(current.selectedIndex)) {
        case OptionAction::Sound:
            // サウンド設定を push
            menuStack_.push({ MenuId::Sound, 0 });
            break;
        case OptionAction::Back:
            PopMenu();
            break;
        default:
            break;
        }
    }
}

void PauseMenu::UpdateSoundMenu()
{
    Input* input = Input::GetInstance();
    MenuState& current = menuStack_.top();
    int count = static_cast<int>(SoundAction::Count);

    bool moved = false;
    if (input->TriggerKeyDown(DIK_UP) || input->TriggerKeyDown(DIK_W) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_UP)) {
        current.selectedIndex = (current.selectedIndex - 1 + count) % count;
        moved = true;
    }
    if (input->TriggerKeyDown(DIK_DOWN) || input->TriggerKeyDown(DIK_S) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_DOWN)) {
        current.selectedIndex = (current.selectedIndex + 1) % count;
        moved = true;
    }

    if (moved && selectSE_ != 0) {
        Audio::GetInstance()->PlayAudio(selectSE_, false, 0.4f, "SE");
    }

    // 左右キーで音量変更
    if (current.selectedIndex == static_cast<int>(SoundAction::BgmVolume)) {
        if (input->TriggerKeyDown(DIK_LEFT) || input->TriggerKeyDown(DIK_A) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_LEFT)) {
            bgmVolume_ -= 0.1f;
            if (bgmVolume_ < 0.0f) bgmVolume_ = 0.0f;
            Audio::GetInstance()->SetCategoryVolume("BGM", bgmVolume_);
        }
        if (input->TriggerKeyDown(DIK_RIGHT) || input->TriggerKeyDown(DIK_D) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT)) {
            bgmVolume_ += 0.1f;
            if (bgmVolume_ > 1.0f) bgmVolume_ = 1.0f;
            Audio::GetInstance()->SetCategoryVolume("BGM", bgmVolume_);
        }
    } else if (current.selectedIndex == static_cast<int>(SoundAction::SeVolume)) {
        if (input->TriggerKeyDown(DIK_LEFT) || input->TriggerKeyDown(DIK_A) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_LEFT)) {
            seVolume_ -= 0.1f;
            if (seVolume_ < 0.0f) seVolume_ = 0.0f;
            Audio::GetInstance()->SetCategoryVolume("SE", seVolume_);
            if (selectSE_ != 0) Audio::GetInstance()->PlayAudio(selectSE_, false, 0.4f, "SE");
        }
        if (input->TriggerKeyDown(DIK_RIGHT) || input->TriggerKeyDown(DIK_D) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT)) {
            seVolume_ += 0.1f;
            if (seVolume_ > 1.0f) seVolume_ = 1.0f;
            Audio::GetInstance()->SetCategoryVolume("SE", seVolume_);
            if (selectSE_ != 0) Audio::GetInstance()->PlayAudio(selectSE_, false, 0.4f, "SE");
        }
    }

    // 決定
    if (input->TriggerKeyDown(DIK_SPACE) || input->TriggerKeyDown(DIK_RETURN) || input->TriggerPadDown(0, XINPUT_GAMEPAD_A)) {
        if (current.selectedIndex == static_cast<int>(SoundAction::Back)) {
            PopMenu();
        }
    }
}

void PauseMenu::Draw()
{
    if (!isPaused_ || menuStack_.empty()) return;

    MenuState& current = menuStack_.top();
    switch (current.menuId) {
    case MenuId::PauseMain:
        DrawMenuItems("=== PAUSE ===", mainLabels_, current.selectedIndex);
        break;
    case MenuId::Options:
        DrawMenuItems("=== OPTIONS ===", optionLabels_, current.selectedIndex);
        break;
    case MenuId::Sound: {
        std::vector<std::string> soundLabelsDisplay = soundLabels_;
        soundLabelsDisplay[0] += " < " + std::to_string(static_cast<int>(bgmVolume_ * 100)) + "% >";
        soundLabelsDisplay[1] += " < " + std::to_string(static_cast<int>(seVolume_ * 100)) + "% >";
        DrawMenuItems("=== SOUND SETTINGS ===", soundLabelsDisplay, current.selectedIndex);
        break;
    }
    }
}

void PauseMenu::DrawMenuItems(const std::string& title, const std::vector<std::string>& labels, int selectedIndex)
{
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight)), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.65f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                            ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("PauseOverlay", nullptr, flags)) {
        float centerX = static_cast<float>(WinApp::kClientWidth) * 0.5f;
        float centerY = static_cast<float>(WinApp::kClientHeight) * 0.32f;

        // タイトル表示
        ImGui::SetWindowFontScale(2.3f);
        ImVec2 titleSize = ImGui::CalcTextSize(title.c_str());
        ImGui::SetCursorPos(ImVec2(centerX - titleSize.x * 0.5f, centerY));
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", title.c_str());

        // 階層表示（パンくずリスト）
        ImGui::SetWindowFontScale(1.2f);
        std::string breadcrumb = "MENU > ";
        if (menuStack_.size() >= 1) breadcrumb += "PAUSE";
        if (menuStack_.size() >= 2) breadcrumb += " > OPTIONS";
        if (menuStack_.size() >= 3) breadcrumb += " > SOUND";
        ImVec2 crumbSize = ImGui::CalcTextSize(breadcrumb.c_str());
        ImGui::SetCursorPos(ImVec2(centerX - crumbSize.x * 0.5f, centerY + 50.0f));
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 0.8f), "%s", breadcrumb.c_str());

        // 項目一覧表示
        ImGui::SetWindowFontScale(1.7f);
        for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
            std::string label = (i == selectedIndex) ? "> " + labels[i] + " <" : "  " + labels[i] + "  ";
            ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            ImGui::SetCursorPos(ImVec2(centerX - textSize.x * 0.5f, centerY + 100.0f + i * 48.0f));

            if (i == selectedIndex) {
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "%s", label.c_str());
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", label.c_str());
            }
        }

        // 操作ガイド（画面下部）
        ImGui::SetWindowFontScale(1.1f);
        const char* guide = "[W/S/十字キー]: 選択   [SPACE/ENTER/A]: 決定   [ESC/B]: 戻る";
        ImVec2 guideSize = ImGui::CalcTextSize(guide);
        ImGui::SetCursorPos(ImVec2(centerX - guideSize.x * 0.5f, static_cast<float>(WinApp::kClientHeight) - 80.0f));
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 0.9f), "%s", guide);

        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::End();
}
