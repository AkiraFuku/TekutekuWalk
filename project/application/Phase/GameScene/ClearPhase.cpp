#include "ClearPhase.h"
#include "Sprite.h"
#include "WinApp.h"
#include "SceneManager.h"
#include "Input.h"
#include "Fade.h"
#include "TextureManager.h"
#include <cmath>

void ClearPhase::Initialize(Scene* scene)
{
    isTransitioning_ = false;
    blinkTimer_ = 0.0f;
    currentBlinkSpeed_ = 4.0f;

    // ステージクリア表示スプライト
    clearSprite_ = std::make_unique<Sprite>();
    clearSprite_->Initialize("resources/StageClear/tekutekuClear.png");
    clearSprite_->SetPosition(WinApp::GetInstance()->GetWindowCenter());
    clearSprite_->SetAnchorPoint({ 0.5f, 0.5f });

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
    ClearSE = Audio::GetInstance()->LoadAudio("resources/Audio/BGM/Stageclear.mp3");
    Play_ = Audio::GetInstance()->PlayAudio(ClearSE, false, 0.25f, "SE");
    decisionSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
}

void ClearPhase::Update(Scene* scene)
{
    blinkTimer_ += 1.0f / 60.0f;

    if (clearSprite_) {
        clearSprite_->Update();
    }

    Input* input = Input::GetInstance();

    if (!isTransitioning_) {
        // 通常のゆったりとしたサイン波点滅（アルファ 0.25 〜 1.0）
        float wave = std::sin(blinkTimer_ * currentBlinkSpeed_);
        float alpha = 0.25f + 0.75f * (0.5f + 0.5f * wave);
        if (startSprite_) {
            startSprite_->SetSize(baseSize_);
            startSprite_->SetColor({ 1.0f, 1.0f, 1.0f, alpha });
        }

        // キーボード（SPACE, RETURN）またはパッド（A, START）の入力検知
        bool isPressed = input->TriggerKeyDown(DIK_SPACE) ||
                         input->TriggerKeyDown(DIK_RETURN) ||
                         input->TriggerPadDown(0, XINPUT_GAMEPAD_A) ||
                         input->TriggerPadDown(0, XINPUT_GAMEPAD_START);

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
            SceneManager::GetInstance()->ChangeScene("TitleScene");
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

void ClearPhase::Draw(Scene* scene)
{
    if (clearSprite_) {
        clearSprite_->Draw();
    }
    if (startSprite_) {
        startSprite_->Draw();
    }
}

void ClearPhase::Finalize(Scene* scene)
{
    clearSprite_.reset();
    startSprite_.reset();
}
