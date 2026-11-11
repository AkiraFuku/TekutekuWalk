#include "PauseMenu.h"
#include "GameScene.h"
#include "Input.h"
#include "Audio.h"
#include "SceneManager.h"
#include "Sprite.h"
#include "WinApp.h"
#include <algorithm>
#include <cmath>

void PauseMenu::Initialize(GameScene* gameScene)
{
    gameScene_ = gameScene;
    isPaused_ = false;
    animTimer_ = 0.0f;
    ClearMenu();

    // 効果音のロード
    selectSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
    decideSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
    cancelSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");

    // 1. 背景・枠スプライト
    backdropSprite_ = std::make_unique<Sprite>();
    backdropSprite_->Initialize("resources/white.png");
    backdropSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.5f, static_cast<float>(WinApp::kClientHeight) * 0.5f });
    backdropSprite_->SetSize({ static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight) });
    backdropSprite_->SetColor({ 0.0f, 0.0f, 0.0f, 0.65f });
    backdropSprite_->SetAnchorPoint(Anchor::Center);

    boardBackSprite_ = std::make_unique<Sprite>();
    boardBackSprite_->Initialize("resources/white.png");
    boardBackSprite_->SetPosition({ 640.0f, 360.0f });
    boardBackSprite_->SetSize({ 580.0f, 470.0f });
    boardBackSprite_->SetColor({ 0.35f, 0.22f, 0.10f, 0.98f }); // 木枠の外枠
    boardBackSprite_->SetAnchorPoint(Anchor::Center);

    boardInnerSprite_ = std::make_unique<Sprite>();
    boardInnerSprite_->Initialize("resources/white.png");
    boardInnerSprite_->SetPosition({ 640.0f, 360.0f });
    boardInnerSprite_->SetSize({ 564.0f, 454.0f });
    boardInnerSprite_->SetColor({ 0.13f, 0.10f, 0.08f, 0.96f }); // 内側ダークプレート
    boardInnerSprite_->SetAnchorPoint(Anchor::Center);

    headerPlateSprite_ = std::make_unique<Sprite>();
    headerPlateSprite_->Initialize("resources/white.png");
    headerPlateSprite_->SetPosition({ 640.0f, 180.0f });
    headerPlateSprite_->SetSize({ 430.0f, 58.0f });
    headerPlateSprite_->SetColor({ 0.78f, 0.58f, 0.26f, 0.98f }); // カービィ64風 木目調ゴールド
    headerPlateSprite_->SetAnchorPoint(Anchor::Center);

    // 2. タイトルスプライト
    pauseTitleSprite_ = std::make_unique<Sprite>();
    pauseTitleSprite_->Initialize("resources/Pause/PauseTitle.png");
    pauseTitleSprite_->SetPosition({ 640.0f, 180.0f });
    pauseTitleSprite_->SetSize({ 420.0f, 90.0f });
    pauseTitleSprite_->SetAnchorPoint(Anchor::Center);

    optionsTitleSprite_ = std::make_unique<Sprite>();
    optionsTitleSprite_->Initialize("resources/Pause/OptionsTitle.png");
    optionsTitleSprite_->SetPosition({ 640.0f, 180.0f });
    optionsTitleSprite_->SetSize({ 420.0f, 90.0f });
    optionsTitleSprite_->SetAnchorPoint(Anchor::Center);

    soundTitleSprite_ = std::make_unique<Sprite>();
    soundTitleSprite_->Initialize("resources/Pause/SoundTitle.png");
    soundTitleSprite_->SetPosition({ 640.0f, 180.0f });
    soundTitleSprite_->SetSize({ 420.0f, 90.0f });
    soundTitleSprite_->SetAnchorPoint(Anchor::Center);

    // 3. メインメニュー項目
    resumeSprite_ = std::make_unique<Sprite>();
    resumeSprite_->Initialize("resources/Pause/BtnResume.png");
    resumeSprite_->SetAnchorPoint(Anchor::Center);

    optionsSprite_ = std::make_unique<Sprite>();
    optionsSprite_->Initialize("resources/Pause/BtnOptions.png");
    optionsSprite_->SetAnchorPoint(Anchor::Center);

    retrySprite_ = std::make_unique<Sprite>();
    retrySprite_->Initialize("resources/Pause/BtnRetry.png");
    retrySprite_->SetAnchorPoint(Anchor::Center);

    titleSprite_ = std::make_unique<Sprite>();
    titleSprite_->Initialize("resources/Pause/BtnTitle.png");
    titleSprite_->SetAnchorPoint(Anchor::Center);

    // 4. オプションメニュー項目
    soundSettingSprite_ = std::make_unique<Sprite>();
    soundSettingSprite_->Initialize("resources/Pause/BtnSound.png");
    soundSettingSprite_->SetAnchorPoint(Anchor::Center);

    optionBackSprite_ = std::make_unique<Sprite>();
    optionBackSprite_->Initialize("resources/Pause/BtnBack.png");
    optionBackSprite_->SetAnchorPoint(Anchor::Center);

    // 5. サウンドメニュー項目
    bgmVolumeSprite_ = std::make_unique<Sprite>();
    bgmVolumeSprite_->Initialize("resources/Pause/BtnBgm.png");
    bgmVolumeSprite_->SetAnchorPoint(Anchor::Center);

    seVolumeSprite_ = std::make_unique<Sprite>();
    seVolumeSprite_->Initialize("resources/Pause/BtnSe.png");
    seVolumeSprite_->SetAnchorPoint(Anchor::Center);

    soundBackSprite_ = std::make_unique<Sprite>();
    soundBackSprite_->Initialize("resources/Pause/BtnBack.png");
    soundBackSprite_->SetAnchorPoint(Anchor::Center);

    // 音量バー背景・充填
    bgmBarBg_ = std::make_unique<Sprite>();
    bgmBarBg_->Initialize("resources/white.png");
    bgmBarBg_->SetAnchorPoint(Anchor::CenterLeft);
    bgmBarBg_->SetSize({ 140.0f, 16.0f });
    bgmBarBg_->SetColor({ 0.25f, 0.20f, 0.15f, 0.95f });

    bgmBarFill_ = std::make_unique<Sprite>();
    bgmBarFill_->Initialize("resources/white.png");
    bgmBarFill_->SetAnchorPoint(Anchor::CenterLeft);
    bgmBarFill_->SetSize({ 140.0f * bgmVolume_, 16.0f });
    bgmBarFill_->SetColor({ 0.96f, 0.82f, 0.25f, 1.0f });

    seBarBg_ = std::make_unique<Sprite>();
    seBarBg_->Initialize("resources/white.png");
    seBarBg_->SetAnchorPoint(Anchor::CenterLeft);
    seBarBg_->SetSize({ 140.0f, 16.0f });
    seBarBg_->SetColor({ 0.25f, 0.20f, 0.15f, 0.95f });

    seBarFill_ = std::make_unique<Sprite>();
    seBarFill_->Initialize("resources/white.png");
    seBarFill_->SetAnchorPoint(Anchor::CenterLeft);
    seBarFill_->SetSize({ 140.0f * seVolume_, 16.0f });
    seBarFill_->SetColor({ 0.96f, 0.82f, 0.25f, 1.0f });

    // 6. カーソル（星）
    cursorLeft_ = std::make_unique<Sprite>();
    cursorLeft_->Initialize("resources/Pause/CursorStar.png");
    cursorLeft_->SetAnchorPoint(Anchor::Center);

    cursorRight_ = std::make_unique<Sprite>();
    cursorRight_->Initialize("resources/Pause/CursorStar.png");
    cursorRight_->SetAnchorPoint(Anchor::Center);
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

void PauseMenu::SetupItemSprite(Sprite* sprite, const Vector2& centerPos, bool isSelected, const Vector2& baseSize)
{
    if (!sprite) return;
    float pulse = 1.0f + 0.05f * std::sin(animTimer_ * 6.0f);
    float scale = isSelected ? (1.08f * pulse) : 0.95f;
    sprite->SetPosition(centerPos);
    sprite->SetSize({ baseSize.x * scale, baseSize.y * scale });
    sprite->SetColor(isSelected ? Vector4{ 1.0f, 0.95f, 0.45f, 1.0f } : Vector4{ 0.70f, 0.70f, 0.70f, 0.80f });
    sprite->Update();
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

    animTimer_ += 1.0f / 60.0f;

    // キャンセルボタン（ESCキー / パッドBボタン）で直前へ戻る (pop)
    if (input->TriggerKeyDown(DIK_ESCAPE) || input->TriggerPadDown(0, XINPUT_GAMEPAD_B)) {
        PopMenu();
        return;
    }

    // 共通スプライト更新
    if (backdropSprite_) backdropSprite_->Update();
    if (boardBackSprite_) boardBackSprite_->Update();
    if (boardInnerSprite_) boardInnerSprite_->Update();
    if (headerPlateSprite_) headerPlateSprite_->Update();

    // 画面状態に応じた処理
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
        return;
    }

    // スプライト配置＆アニメーション
    if (pauseTitleSprite_) pauseTitleSprite_->Update();

    Vector2 baseBtnSize = { 340.0f, 70.0f };
    Vector2 positions[4] = {
        { 640.0f, 255.0f },
        { 640.0f, 320.0f },
        { 640.0f, 385.0f },
        { 640.0f, 450.0f }
    };

    SetupItemSprite(resumeSprite_.get(), positions[0], current.selectedIndex == 0, baseBtnSize);
    SetupItemSprite(optionsSprite_.get(), positions[1], current.selectedIndex == 1, baseBtnSize);
    SetupItemSprite(retrySprite_.get(), positions[2], current.selectedIndex == 2, baseBtnSize);
    SetupItemSprite(titleSprite_.get(), positions[3], current.selectedIndex == 3, baseBtnSize);

    // カーソル配置
    Vector2 selPos = positions[current.selectedIndex];
    float starPulse = 1.0f + 0.12f * std::sin(animTimer_ * 8.0f);
    Vector2 starSize = { 38.0f * starPulse, 38.0f * starPulse };

    cursorLeft_->SetPosition({ selPos.x - 175.0f, selPos.y });
    cursorLeft_->SetSize(starSize);
    cursorLeft_->SetRotation(animTimer_ * 3.0f);
    cursorLeft_->Update();

    cursorRight_->SetPosition({ selPos.x + 175.0f, selPos.y });
    cursorRight_->SetSize(starSize);
    cursorRight_->SetRotation(-animTimer_ * 3.0f);
    cursorRight_->Update();
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
            menuStack_.push({ MenuId::Sound, 0 });
            break;
        case OptionAction::Back:
            PopMenu();
            break;
        default:
            break;
        }
        return;
    }

    if (optionsTitleSprite_) optionsTitleSprite_->Update();

    Vector2 baseBtnSize = { 340.0f, 70.0f };
    Vector2 positions[2] = {
        { 640.0f, 290.0f },
        { 640.0f, 390.0f }
    };

    SetupItemSprite(soundSettingSprite_.get(), positions[0], current.selectedIndex == 0, baseBtnSize);
    SetupItemSprite(optionBackSprite_.get(), positions[1], current.selectedIndex == 1, baseBtnSize);

    Vector2 selPos = positions[current.selectedIndex];
    float starPulse = 1.0f + 0.12f * std::sin(animTimer_ * 8.0f);
    Vector2 starSize = { 38.0f * starPulse, 38.0f * starPulse };

    cursorLeft_->SetPosition({ selPos.x - 175.0f, selPos.y });
    cursorLeft_->SetSize(starSize);
    cursorLeft_->SetRotation(animTimer_ * 3.0f);
    cursorLeft_->Update();

    cursorRight_->SetPosition({ selPos.x + 175.0f, selPos.y });
    cursorRight_->SetSize(starSize);
    cursorRight_->SetRotation(-animTimer_ * 3.0f);
    cursorRight_->Update();
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
            if (selectSE_ != 0) Audio::GetInstance()->PlayAudio(selectSE_, false, 0.4f, "SE");
        }
        if (input->TriggerKeyDown(DIK_RIGHT) || input->TriggerKeyDown(DIK_D) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT)) {
            bgmVolume_ += 0.1f;
            if (bgmVolume_ > 1.0f) bgmVolume_ = 1.0f;
            Audio::GetInstance()->SetCategoryVolume("BGM", bgmVolume_);
            if (selectSE_ != 0) Audio::GetInstance()->PlayAudio(selectSE_, false, 0.4f, "SE");
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
            return;
        }
    }

    if (soundTitleSprite_) soundTitleSprite_->Update();

    Vector2 baseBtnSize = { 300.0f, 65.0f };
    Vector2 positions[3] = {
        { 515.0f, 275.0f },
        { 515.0f, 350.0f },
        { 640.0f, 435.0f }
    };

    SetupItemSprite(bgmVolumeSprite_.get(), positions[0], current.selectedIndex == 0, baseBtnSize);
    SetupItemSprite(seVolumeSprite_.get(), positions[1], current.selectedIndex == 1, baseBtnSize);
    SetupItemSprite(soundBackSprite_.get(), positions[2], current.selectedIndex == 2, { 320.0f, 65.0f });

    // 音量バーの更新
    bgmBarBg_->SetPosition({ 680.0f, 275.0f });
    bgmBarBg_->Update();

    bgmBarFill_->SetPosition({ 680.0f, 275.0f });
    bgmBarFill_->SetSize({ (std::max)(1.0f, 150.0f * bgmVolume_), 16.0f });
    bgmBarFill_->Update();

    seBarBg_->SetPosition({ 680.0f, 350.0f });
    seBarBg_->Update();

    seBarFill_->SetPosition({ 680.0f, 350.0f });
    seBarFill_->SetSize({ (std::max)(1.0f, 150.0f * seVolume_), 16.0f });
    seBarFill_->Update();

    // カーソル位置
    Vector2 selPos = positions[current.selectedIndex];
    float leftOffset = (current.selectedIndex == 2) ? 170.0f : 160.0f;
    float rightOffset = (current.selectedIndex == 2) ? 170.0f : 330.0f;

    float starPulse = 1.0f + 0.12f * std::sin(animTimer_ * 8.0f);
    Vector2 starSize = { 38.0f * starPulse, 38.0f * starPulse };

    cursorLeft_->SetPosition({ selPos.x - leftOffset, selPos.y });
    cursorLeft_->SetSize(starSize);
    cursorLeft_->SetRotation(animTimer_ * 3.0f);
    cursorLeft_->Update();

    cursorRight_->SetPosition({ selPos.x + rightOffset, selPos.y });
    cursorRight_->SetSize(starSize);
    cursorRight_->SetRotation(-animTimer_ * 3.0f);
    cursorRight_->Update();
}

void PauseMenu::Draw()
{
    if (!isPaused_ || menuStack_.empty()) return;

    // 共通の背景枠
    if (backdropSprite_) backdropSprite_->Draw();
    if (boardBackSprite_) boardBackSprite_->Draw();
    if (boardInnerSprite_) boardInnerSprite_->Draw();
    if (headerPlateSprite_) headerPlateSprite_->Draw();

    MenuState& current = menuStack_.top();
    switch (current.menuId) {
    case MenuId::PauseMain:
        if (pauseTitleSprite_) pauseTitleSprite_->Draw();
        if (resumeSprite_) resumeSprite_->Draw();
        if (optionsSprite_) optionsSprite_->Draw();
        if (retrySprite_) retrySprite_->Draw();
        if (titleSprite_) titleSprite_->Draw();
        if (cursorLeft_) cursorLeft_->Draw();
        if (cursorRight_) cursorRight_->Draw();
        break;

    case MenuId::Options:
        if (optionsTitleSprite_) optionsTitleSprite_->Draw();
        if (soundSettingSprite_) soundSettingSprite_->Draw();
        if (optionBackSprite_) optionBackSprite_->Draw();
        if (cursorLeft_) cursorLeft_->Draw();
        if (cursorRight_) cursorRight_->Draw();
        break;

    case MenuId::Sound:
        if (soundTitleSprite_) soundTitleSprite_->Draw();
        if (bgmVolumeSprite_) bgmVolumeSprite_->Draw();
        if (bgmBarBg_) bgmBarBg_->Draw();
        if (bgmBarFill_) bgmBarFill_->Draw();
        if (seVolumeSprite_) seVolumeSprite_->Draw();
        if (seBarBg_) seBarBg_->Draw();
        if (seBarFill_) seBarFill_->Draw();
        if (soundBackSprite_) soundBackSprite_->Draw();
        if (cursorLeft_) cursorLeft_->Draw();
        if (cursorRight_) cursorRight_->Draw();
        break;
    }
}
