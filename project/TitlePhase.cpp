#include "TitlePhase.h"
#include "Input.h"
#include "SceneManager.h"
#include "Fade.h"
#include "TextureManager.h"
#include "WinApp.h"
#include <cmath>

void TitlePhase::Initialize(Scene* scene)
{
    isTransitioning_ = false;
    blinkTimer_ = 0.0f;
    currentBlinkSpeed_ = 4.0f; // 通常時の点滅速度（約1.6秒周期）

    // テクスチャロードとスプライト生成
    TextureManager::GetInstance()->LoadTexture("resources/Gaid/Start.png");
    TextureManager::GetInstance()->LoadTexture("resources/Gaid/push.png");
    startSprite_ = std::make_unique<Sprite>();
    startSprite_->Initialize("resources/Gaid/Start.png");
    startSprite_->SetAnchorPoint(Anchor::Center);

    baseSize_ = startSprite_->GetSize();
    // 基準サイズに応じたスケール調整
    if (baseSize_.x <= 0.0f || baseSize_.y <= 0.0f) {
        baseSize_ = { 320.0f, 80.0f };
    } else if (baseSize_.x < 200.0f) {
        // 小さいアイコン画像の場合は視認性を高めるため2倍スケール
        baseSize_ = { baseSize_.x * 2.0f, baseSize_.y * 2.0f };
    }
    startSprite_->SetSize(baseSize_);

    // 画面下部中央（幅1280の中央 640、高さ720の下寄り 580）に配置
    startSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.5f, static_cast<float>(WinApp::kClientHeight) * 0.80f });

    // 決定効果音のロード
    decisionSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
}

void TitlePhase::Update(Scene* scene)
{
    blinkTimer_ += 1.0f / 60.0f;

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
            currentBlinkSpeed_ = 35.0f; // ボタン押下で高速点滅モードへ加速！

            // 決定音の再生
            if (decisionSE_ != 0) {
                Audio::GetInstance()->PlayAudio(decisionSE_, false, 0.8f, "SE");
            }

            // フェードアウト開始（1.2秒かけて暗転）
            Fade::GetInstance()->StartFadeOut(1.2f);
        }
    } else {
        // ボタン押下後：高速ピカピカ点滅（明瞭なオン・オフ）＆少し拡大してフィードバック
        float wave = std::sin(blinkTimer_ * currentBlinkSpeed_);
        float fastAlpha = (wave > 0.0f) ? 1.0f : 0.08f;
        if (startSprite_) {
            startSprite_->SetSize({ baseSize_.x * 1.15f, baseSize_.y * 1.15f });
            startSprite_->SetColor({ 1.0f, 1.0f, 1.0f, fastAlpha });
        }

        // フェードアウトが完了したらゲームシーンへ遷移
        if (!Fade::GetInstance()->IsFading()) {
            SceneManager::GetInstance()->ChangeScene("GameScene");
        }
    }

    if (startSprite_) {
        startSprite_->Update();
    }
}

void TitlePhase::Draw(Scene* scene)
{
    if (startSprite_) {
        startSprite_->Draw();
    }
}

void TitlePhase::Finalize(Scene* scene)
{
    startSprite_.reset();
}

