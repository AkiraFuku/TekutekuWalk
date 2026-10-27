#include "defeatPhase.h"
#include "Sprite.h"
#include "WinApp.h"
#include "SceneManager.h"
#include "Input.h"
#include "Fade.h"
#include "TextureManager.h"
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void defeatPhase::Initialize(Scene* scene)
{
    isTransitioning_ = false;
    blinkTimer_ = 0.0f;
    currentBlinkSpeed_ = 4.0f;
    selectedMenu_ = MenuItem::Retry;

    // ゲームオーバー表示スプライト
    defeatSprite_ = std::make_unique<Sprite>();
    defeatSprite_->Initialize("resources/GAMOVER/tekutekuGameOver.png");
    defeatSprite_->SetPosition(WinApp::GetInstance()->GetWindowCenter());
    defeatSprite_->SetAnchorPoint({ 0.5f, 0.5f });

    // ボタン入力指示スプライト
    TextureManager::GetInstance()->LoadTexture("resources/Gaid/push.png");
    startSprite_ = std::make_unique<Sprite>();
    startSprite_->Initialize("resources/Gaid/push.png");
    startSprite_->SetAnchorPoint(Anchor::Center);

    baseSize_ = startSprite_->GetSize();
    if (baseSize_.x <= 0.0f || baseSize_.y <= 0.0f) {
        baseSize_ = { 320.0f, 80.0f };
    } else if (baseSize_.x < 200.0f) {
        baseSize_ = { baseSize_.x * 2.0f, baseSize_.y * 2.0f };
    }
    startSprite_->SetSize(baseSize_);
    startSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.5f, static_cast<float>(WinApp::kClientHeight) * 0.80f });

    // 音声
    defeatSE = Audio::GetInstance()->LoadAudio("resources/Audio/BGM/GameOver.mp3");
    Play_ = Audio::GetInstance()->PlayAudio(defeatSE, false, 0.25f, "SE");
    decisionSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
}

void defeatPhase::Update(Scene* scene)
{
    blinkTimer_ += 1.0f / 60.0f;

    if (defeatSprite_) {
        defeatSprite_->Update();
    }

    Input* input = Input::GetInstance();

    // ── ImGui デバッグウィンドウ ──
#ifdef USE_IMGUI
    ImGui::Begin("Game Over Menu (Defeat Phase)");
    ImGui::Text("=== YOU DIED ===");

    bool triggerRetry = false;
    bool triggerQuit = false;

    const char* menuNames[] = { "Retry Stage (GameScene)", "Quit (GameOverScene)" };
    int currentItem = static_cast<int>(selectedMenu_);
    if (ImGui::Combo("Selection", &currentItem, menuNames, IM_ARRAYSIZE(menuNames))) {
        selectedMenu_ = static_cast<MenuItem>(currentItem);
    }

    ImGui::Separator();
    if (ImGui::Button("Retry Stage (GameScene)", ImVec2(220, 40))) {
        selectedMenu_ = MenuItem::Retry;
        triggerRetry = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Quit to GameOverScene", ImVec2(220, 40))) {
        selectedMenu_ = MenuItem::Quit;
        triggerQuit = true;
    }
    ImGui::Text("Key/Pad: Left/Right to Select, Space/Enter/A to Confirm");
    ImGui::End();
#endif

    if (!isTransitioning_) {
        // 通常のゆったりとしたサイン波点滅（アルファ 0.25 〜 1.0）
        float wave = std::sin(blinkTimer_ * currentBlinkSpeed_);
        float alpha = 0.25f + 0.75f * (0.5f + 0.5f * wave);
        if (startSprite_) {
            startSprite_->SetSize(baseSize_);
            startSprite_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        }

        // 左右キー / パッド十字キーで選択切り替え
        bool moveLeft = input->TriggerKeyDown(DIK_LEFT) || input->TriggerKeyDown(DIK_A) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_LEFT);
        bool moveRight = input->TriggerKeyDown(DIK_RIGHT) || input->TriggerKeyDown(DIK_D) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT);

        if (moveLeft) {
            selectedMenu_ = MenuItem::Retry;
        } else if (moveRight) {
            selectedMenu_ = MenuItem::Quit;
        }

        // キーボード（SPACE, RETURN）またはパッド（A, START）、またはImGuiボタンによる決定
        bool isPressed = input->TriggerKeyDown(DIK_SPACE) ||
                         input->TriggerKeyDown(DIK_RETURN) ||
                         input->TriggerPadDown(0, XINPUT_GAMEPAD_A) ||
                         input->TriggerPadDown(0, XINPUT_GAMEPAD_START)
#ifdef USE_IMGUI
                         || triggerRetry || triggerQuit
#endif
                         ;

        if (isPressed) {
            isTransitioning_ = true;
            currentBlinkSpeed_ = 35.0f; // ボタン押下で高速点滅モードへ加速

            // 決定音の再生
            if (decisionSE_ != 0) {
                Audio::GetInstance()->PlayAudio(decisionSE_, false, 0.8f, "SE");
            }

            Fade::GetInstance()->StartFadeOut(1.0f); // 1.0秒かけてフェードアウト
        }
    } else {
        // ボタン押下後：高速点滅＆少し拡大フィードバック
        float wave = std::sin(blinkTimer_ * currentBlinkSpeed_);
        float fastAlpha = (wave > 0.0f) ? 1.0f : 0.08f;
        if (startSprite_) {
            startSprite_->SetSize({ baseSize_.x * 1.15f, baseSize_.y * 1.15f });
            startSprite_->SetColor({ 1.0f, 1.0f, 1.0f, fastAlpha });
        }

        if (!Fade::GetInstance()->IsFading()) {
            if (selectedMenu_ == MenuItem::Retry) {
                SceneManager::GetInstance()->ChangeScene("GameScene");
            } else {
                SceneManager::GetInstance()->ChangeScene("GameOverScene");
            }
        }
    }

    if (startSprite_) {
        startSprite_->Update();
    }

    if (!Audio::GetInstance()->IsPlaying(Play_)) {
        if (!Audio::GetInstance()->IsPlaying(scene->getBGMPlayHundle())) {
            Audio::GetInstance()->ResumeAudio(scene->getBGMPlayHundle());
        }
    }
}

void defeatPhase::Draw(Scene* scene)
{
    if (defeatSprite_) {
        defeatSprite_->Draw();
    }
    if (startSprite_) {
        startSprite_->Draw();
    }
}

void defeatPhase::Finalize(Scene* scene)
{
    defeatSprite_.reset();
    startSprite_.reset();
}
