#include "defeatPhase.h"
#include "Sprite.h"
#include "WinApp.h"
#include "SceneManager.h"
#include "Input.h"
#include "Fade.h"
#include "TextureManager.h"
#include "GameScene.h"
#include "Player.h"
#include "Enemy.h"
#include "CameraController.h"
#include "Camera.h"
#include "ParticleManager.h"
#include "MathFunction.h"
#include <cmath>
#include <algorithm>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void defeatPhase::Initialize(Scene* scene)
{
    isTransitioning_ = false;
    blinkTimer_ = 0.0f;
    currentBlinkSpeed_ = 4.0f;
    selectedMenu_ = MenuItem::Retry;
    step_ = Step::DeathKnockback;
    bounceCount_ = 0;
    settleTimer_ = 0.0f;
    dropTimer_ = 0.0f;
    darkFadeAlpha_ = 0.0f;
    isFallDeath_ = false;

    GameScene* gameScene = static_cast<GameScene*>(scene);
    Player* player = gameScene ? gameScene->GetPlayer() : nullptr;
    CameraController* camCtrl = gameScene ? gameScene->GetCamera() : nullptr;

    // 1. プレイヤーの強調表示＆吹っ飛び初期パラメータ計算
    if (player && player->GetObject3d()) {
        player->ResetHitInvincibility(); // 無敵点滅を強制解除して確実にモデルを描画

        Vector3 basePos = player->GetWorldPosition();
        Vector3 camPos = camCtrl ? camCtrl->GetCamera()->GetTranslate() : Vector3(basePos.x, basePos.y + 2.0f, basePos.z - 10.0f);

        // 落下死の場合、奈落（カメラ画角外）にいるためカメラ下端付近から飛び上がるようY座標を補正
        isFallDeath_ = (basePos.y < -5.0f);
        if (isFallDeath_) {
            basePos.y = camPos.y - 3.5f;
        }
        groundY_ = basePos.y;

        // 【最前面描画】深度テストを無効化（カービィ風）
        player->GetObject3d()->SetDepthTestEnable(false);

        // カメラ手前方向へオフセット（控えめにして過剰な視点ズレを防止）
        Vector3 toCam = camPos - basePos;
        toCam.y = 0.0f;
        float len = Length(toCam);
        if (len > 0.001f) {
            toCam = toCam / len;
        } else {
            toCam = { 0.0f, 0.0f, -1.0f };
        }
        playerAnimPos_ = basePos + toCam * 0.3f;

        // プレイヤーの向きと後方ベクトル
        int moveDir = player->GetMoveDirection();
        facingDir_ = (moveDir >= 0) ? 1.0f : -1.0f;
        Vector3 fwd = player->GetDirection();
        if (Length(fwd) < 0.001f) {
            fwd = { 1.0f, 0.0f, 0.0f };
        } else {
            fwd = Normalize(fwd);
        }
        knockbackDir_ = fwd * (-facingDir_); // 後方方向

        // 初速設定（上向き＋後方）
        vy_ = 11.5f;
        vx_ = 4.5f;
        rotZSpeed_ = -facingDir_ * 10.0f; // 後方に回転

        playerAnimRot_ = player->GetObject3d()->GetRotate();
        playerBaseScale_ = player->GetObject3d()->GetScale();

        // 強調スケール（1.2倍）を適用
        player->GetObject3d()->SetTranslate(playerAnimPos_);
        player->GetObject3d()->SetScale(playerBaseScale_ * 1.2f);
        player->GetObject3d()->Update();

        // エフェクト発生
        ParticleManager::GetInstance()->Emit("GameEffects", "Hit", playerAnimPos_, 16);
    }

    // カメラの中央をプレイヤーに即座に向ける
    UpdateCameraFocus(scene);

    // 死亡演出初期化直後もエネミーのWVP行列を最新カメラで同期
    if (gameScene) {
        for (const auto& enemy : gameScene->GetEnemies()) {
            if (enemy) {
                enemy->UpdateMatrix();
            }
        }
    }

    // 1.5 死亡演出用ダークフェードスプライトの初期化（黒の薄っすらオーバーレイ）
    darkFadeSprite_ = std::make_unique<Sprite>();
    darkFadeSprite_->Initialize("resources/white.png");
    darkFadeSprite_->SetPosition({ 0.0f, 0.0f });
    darkFadeSprite_->SetSize({ static_cast<float>(WinApp::kClientWidth), static_cast<float>(WinApp::kClientHeight) });
    darkFadeSprite_->SetAnchorPoint(Anchor::TopLeft);
    darkFadeSprite_->SetColor({ 0.0f, 0.0f, 0.0f, darkFadeAlpha_ });
    darkFadeSprite_->Update();

    // 2. スプライト初期化（初期位置は画面外の上部に配置）
    spriteTargetY_ = static_cast<float>(WinApp::kClientHeight) * 0.32f;
    spriteStartY_ = -200.0f;

    defeatSprite_ = std::make_unique<Sprite>();
    defeatSprite_->Initialize("resources/GAMOVER/QContenue.png");
    defeatSprite_->SetSize(defeatSpriteBaseSize_);
    defeatSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.5f, spriteStartY_ });
    defeatSprite_->SetAnchorPoint({ 0.5f, 0.5f });

    // 3. 下部選択ボタンスプライト（コンテニュー / NO）の初期化
    continueButtonSprite_ = std::make_unique<Sprite>();
    continueButtonSprite_->Initialize("resources/GAMOVER/continue.png");
    continueButtonSprite_->SetSize(continueButtonBaseSize_);
    continueButtonSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.36f, static_cast<float>(WinApp::kClientHeight) * 0.72f });
    continueButtonSprite_->SetAnchorPoint({ 0.5f, 0.5f });
    continueButtonSprite_->Update();

    noButtonSprite_ = std::make_unique<Sprite>();
    noButtonSprite_->Initialize("resources/GAMOVER/NO.png");
    noButtonSprite_->SetSize(noButtonBaseSize_);
    noButtonSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.64f, static_cast<float>(WinApp::kClientHeight) * 0.72f });
    noButtonSprite_->SetAnchorPoint({ 0.5f, 0.5f });
    noButtonSprite_->Update();

    // 音声
    defeatSE = Audio::GetInstance()->LoadAudio("resources/Audio/BGM/GameOver.mp3");
    Play_ = Audio::GetInstance()->PlayAudio(defeatSE, false, 0.25f, "SE");
    decisionSE_ = Audio::GetInstance()->LoadAudio("resources/Audio/SE/Hit.mp3");
}

void defeatPhase::Update(Scene* scene)
{
    float dt = 1.0f / 60.0f;
    blinkTimer_ += dt;

    // 薄っすらダークフェードを段々かけていく
    if (darkFadeAlpha_ < kMaxDarkFadeAlpha_) {
        darkFadeAlpha_ = (std::min)(darkFadeAlpha_ + dt * 0.6f, kMaxDarkFadeAlpha_);
    }
    if (darkFadeSprite_) {
        darkFadeSprite_->SetColor({ 0.0f, 0.0f, 0.0f, darkFadeAlpha_ });
        darkFadeSprite_->Update();
    }

    GameScene* gameScene = static_cast<GameScene*>(scene);
    Player* player = gameScene ? gameScene->GetPlayer() : nullptr;
    Input* input = Input::GetInstance();

    // スキップ入力の検知（SPACE, RETURN, パッドA）
    bool isSkipTriggered = input->TriggerKeyDown(DIK_SPACE) ||
                          input->TriggerKeyDown(DIK_RETURN) ||
                          input->TriggerPadDown(0, XINPUT_GAMEPAD_A);

    // ──────── Step 1: 死亡吹っ飛び＆バウンド ────────
    if (step_ == Step::DeathKnockback) {
        if (isSkipTriggered) {
            // 【スキップ】即座に接地・倒れ状態にしてスプライト降下演出へ移行
            darkFadeAlpha_ = kMaxDarkFadeAlpha_;
            if (player && player->GetObject3d()) {
                playerAnimPos_.y = groundY_;
                playerAnimRot_.z = -facingDir_ * 1.57f; // 90度倒れた状態
                player->GetObject3d()->SetTranslate(playerAnimPos_);
                player->GetObject3d()->SetRotate(playerAnimRot_);
                player->GetObject3d()->Update();
            }
            UpdateCameraFocus(scene);
            step_ = Step::SpriteDropIn;
            dropTimer_ = 0.0f;
        } else {
            if (player && player->GetObject3d()) {
                const float kGravity = -28.0f;
                vy_ += kGravity * dt;
                playerAnimPos_ += knockbackDir_ * (vx_ * dt);
                playerAnimPos_.y += vy_ * dt;
                playerAnimRot_.z += rotZSpeed_ * dt;

                // 着地判定
                if (playerAnimPos_.y <= groundY_) {
                    playerAnimPos_.y = groundY_;
                    bounceCount_++;

                    if (bounceCount_ <= 2) {
                        vy_ = -vy_ * 0.42f; // バウンド反発
                        vx_ *= 0.6f;
                        rotZSpeed_ *= 0.6f;
                        ParticleManager::GetInstance()->Emit("GameEffects", "Hit", playerAnimPos_, 6);
                    } else {
                        // バウンド終了、静止
                        vy_ = 0.0f;
                        vx_ = 0.0f;
                        rotZSpeed_ = 0.0f;
                        settleTimer_ += dt;
                        if (settleTimer_ >= 0.25f) {
                            step_ = Step::SpriteDropIn;
                            dropTimer_ = 0.0f;
                        }
                    }
                }

                player->GetObject3d()->SetTranslate(playerAnimPos_);
                player->GetObject3d()->SetRotate(playerAnimRot_);
                player->GetObject3d()->Update();

                // 吹っ飛び・バウンド中もカメラ中央をプレイヤーに固定
                UpdateCameraFocus(scene);
            } else {
                step_ = Step::SpriteDropIn;
            }
        }
    }
    // ──────── Step 2: スプライト降下演出 ────────
    else if (step_ == Step::SpriteDropIn) {
        UpdateCameraFocus(scene);
        if (isSkipTriggered) {
            // 【スキップ】即座にスプライトを着地させてメニュー選択へ移行
            darkFadeAlpha_ = kMaxDarkFadeAlpha_;
            step_ = Step::MenuSelect;
            if (defeatSprite_) {
                defeatSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.5f, spriteTargetY_ });
                defeatSprite_->Update();
            }
            if (decisionSE_ != 0) {
                Audio::GetInstance()->PlayAudio(decisionSE_, false, 0.5f, "SE");
            }
        } else {
            dropTimer_ += dt;
            float progress = (std::min)(dropTimer_ / kDropDuration_, 1.0f);

            // EaseOutBack イージング
            float c1 = 1.70158f;
            float c3 = c1 + 1.0f;
            float ease = 1.0f + c3 * std::pow(progress - 1.0f, 3.0f) + c1 * std::pow(progress - 1.0f, 2.0f);

            float currentY = spriteStartY_ + (spriteTargetY_ - spriteStartY_) * ease;
            if (defeatSprite_) {
                defeatSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.5f, currentY });
                defeatSprite_->Update();
            }

            if (progress >= 1.0f) {
                step_ = Step::MenuSelect;
                if (defeatSprite_) {
                    defeatSprite_->SetPosition({ static_cast<float>(WinApp::kClientWidth) * 0.5f, spriteTargetY_ });
                }
                if (decisionSE_ != 0) {
                    Audio::GetInstance()->PlayAudio(decisionSE_, false, 0.5f, "SE");
                }
            }
        }
    }
    // ──────── Step 3: メニュー選択待機 ────────
    else if (step_ == Step::MenuSelect) {
        UpdateCameraFocus(scene);
        if (defeatSprite_) {
            defeatSprite_->Update();
        }

        // 選択中ボタンのアニメーション（拡大パルス演出＆ハイライト）
        float pulse = 1.0f + 0.04f * std::sin(blinkTimer_ * 6.0f);
        if (continueButtonSprite_) {
            bool isSelected = (selectedMenu_ == MenuItem::Retry);
            float scale = isSelected ? (1.12f * pulse) : 0.95f;
            continueButtonSprite_->SetSize(continueButtonBaseSize_ * scale);
            continueButtonSprite_->SetColor(isSelected ? Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } : Vector4{ 0.55f, 0.55f, 0.55f, 0.75f });
            continueButtonSprite_->Update();
        }
        if (noButtonSprite_) {
            bool isSelected = (selectedMenu_ == MenuItem::Quit);
            float scale = isSelected ? (1.12f * pulse) : 0.95f;
            noButtonSprite_->SetSize(noButtonBaseSize_ * scale);
            noButtonSprite_->SetColor(isSelected ? Vector4{ 1.0f, 1.0f, 1.0f, 1.0f } : Vector4{ 0.55f, 0.55f, 0.55f, 0.75f });
            noButtonSprite_->Update();
        }

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
            bool moveLeft = input->TriggerKeyDown(DIK_LEFT) || input->TriggerKeyDown(DIK_A) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_LEFT);
            bool moveRight = input->TriggerKeyDown(DIK_RIGHT) || input->TriggerKeyDown(DIK_D) || input->TriggerPadDown(0, XINPUT_GAMEPAD_DPAD_RIGHT);

            if (moveLeft) selectedMenu_ = MenuItem::Retry;
            else if (moveRight) selectedMenu_ = MenuItem::Quit;

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
                step_ = Step::FadeOut;
                if (decisionSE_ != 0) {
                    Audio::GetInstance()->PlayAudio(decisionSE_, false, 0.8f, "SE");
                }
                Fade::GetInstance()->StartFadeOut(1.0f);
            }
        }
    }
    // ──────── Step 4: フェードアウト後遷移 ────────
    else if (step_ == Step::FadeOut) {
        if (!Fade::GetInstance()->IsFading()) {
            if (selectedMenu_ == MenuItem::Retry) {
                SceneManager::GetInstance()->ChangeScene("GameScene");
            } else {
                SceneManager::GetInstance()->ChangeScene("GameOverScene");
            }
        }
    }

    // ──────── 最新カメラ行列に合わせてエネミー等のWVP行列（定数バッファ）を同期 ────────
    if (gameScene) {
        for (const auto& enemy : gameScene->GetEnemies()) {
            if (enemy) {
                enemy->UpdateMatrix();
            }
        }
    }

    if (!Audio::GetInstance()->IsPlaying(Play_)) {
        if (!Audio::GetInstance()->IsPlaying(scene->getBGMPlayHundle())) {
            Audio::GetInstance()->ResumeAudio(scene->getBGMPlayHundle());
        }
    }
}

void defeatPhase::Draw(Scene* scene)
{
    GameScene* gameScene = static_cast<GameScene*>(scene);
    Player* player = gameScene ? gameScene->GetPlayer() : nullptr;

    // 1. フェード（奥：背景や敵、UIを薄暗く覆う）
    if (darkFadeSprite_ && darkFadeAlpha_ > 0.001f) {
        darkFadeSprite_->Draw();
    }

    // 2. プレイヤー（中間：深度テスト無効でフェードの上に鮮明に描画）
    if (player) {
        player->Draw();
    }

    // 3. ゲームオーバースプライト（手前：最前面に重なる）
    if (step_ >= Step::SpriteDropIn) {
        if (defeatSprite_) {
            defeatSprite_->Draw();
        }
    }
    // 4. 下部ボタンスプライト（コンテニュー / NO）
    if (step_ >= Step::MenuSelect) {
        if (continueButtonSprite_) {
            continueButtonSprite_->Draw();
        }
        if (noButtonSprite_) {
            noButtonSprite_->Draw();
        }
    }
}

void defeatPhase::Finalize(Scene* scene)
{
    GameScene* gameScene = static_cast<GameScene*>(scene);
    Player* player = gameScene ? gameScene->GetPlayer() : nullptr;
    if (player && player->GetObject3d()) {
        player->GetObject3d()->SetDepthTestEnable(true); // 深度テスト有効に戻す
        player->GetObject3d()->SetScale(playerBaseScale_); // スケールを戻す
    }
    defeatSprite_.reset();
    darkFadeSprite_.reset();
    continueButtonSprite_.reset();
    noButtonSprite_.reset();
}

void defeatPhase::UpdateCameraFocus(Scene* scene)
{
    // 落下死の場合はカメラの中央に固定しない（カメラの向き・注視点をそのままキープ）
    if (isFallDeath_) {
        return;
    }

    GameScene* gameScene = static_cast<GameScene*>(scene);
    CameraController* camCtrl = gameScene ? gameScene->GetCamera() : nullptr;
    if (!camCtrl || !camCtrl->GetCamera()) return;

    Camera* camera = camCtrl->GetCamera();
    Vector3 camPos = camera->GetTranslate();

    // 画面中央にプレイヤーの重心（中心高さ）を捉えるための視線ベクトル計算 (LookAt)
    // 足元ではなくキャラクター中心高さ（+1.8f）を注視することで、カメラが下を向きすぎて敵が床に埋まるのを防止
    Vector3 lookTarget = playerAnimPos_;
    lookTarget.y += 1.8f;
    Vector3 toPlayer = lookTarget - camPos;
    float len = Length(toPlayer);
    if (len > 0.001f) {
        Vector3 dir = toPlayer / len;
        float angleY = std::atan2(dir.x, dir.z);
        float distanceXZ = std::sqrt(dir.x * dir.x + dir.z * dir.z);
        float angleX = std::atan2(-dir.y, distanceXZ);
        camera->SetRotate({ angleX, angleY, 0.0f });
        camera->Update();
    }
}
