#include "StartPhase.h"
#include "GameScene.h"
#include "PlayPhase.h"
#include "Fade.h"
#include "RailPath.h"
#include "GoalObject.h"
#include "Player.h"
#include "CameraController.h"
#include "StageManager.h"
#include "MathFunction.h"
#include <cmath>
#include <algorithm>
#include "WinApp.h"
#include "Collider.h"
#include "Input.h"

bool StartPhase::HasMovementInput() const
{
    Input* input = Input::GetInstance();
    if (!input) {
        return false;
    }

    // キーボードの移動入力 (A/Dキー、左右矢印キー)
    if (input->PushedKeyDown(DIK_A) || input->PushedKeyDown(DIK_D) ||
        input->PushedKeyDown(DIK_LEFT) || input->PushedKeyDown(DIK_RIGHT)) {
        return true;
    }

    // ゲームパッドのスティック・十字キー入力
    XINPUT_STATE state;
    if (input->GetJoyStick(0, state)) {
        float stickX = static_cast<float>(state.Gamepad.sThumbLX) / 32767.0f;
        if (std::abs(stickX) > 0.2f) {
            return true;
        }
        if (state.Gamepad.wButtons & (XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_RIGHT)) {
            return true;
        }
    }
    return false;
}

void StartPhase::Initialize(Scene* scene)
{
    // フェードインを開始 (暗転 -> 明転)
    Fade::GetInstance()->StartFadeIn(kFadeDuration_);

    GameScene* gameScene = static_cast<GameScene*>(scene);
    timer_ = 0.0f;
    runProgress_ = 0.0f;
    bannerTimer_ = 0.0f;
    isArrived_ = false;

    // 画面上部の左から右へ流れる演出スプライトの初期化
    bannerSprite_ = std::make_unique<Sprite>();
    bannerSprite_->Initialize("resources/gradationLine.png");
    bannerSprite_->SetAnchorPoint(Anchor::Center);

    bannerStartY_ = 100.0f;

    // スプライトの横幅を取得し、全体が完全に画面外に出る開始・終了座標を動的に計算
    float spriteWidth = bannerSprite_->GetSize().x;
    if (spriteWidth <= 0.0f) {
        spriteWidth = 300.0f; // 取得できなかった場合のフォールバック幅
    }
    const float halfWidth = spriteWidth * 0.5f;
    const float margin = 50.0f; // 画面外へ確実に出すための余白
    bannerStartX_ = -halfWidth - margin;
    bannerEndX_ = static_cast<float>(WinApp::kClientWidth) + halfWidth + margin;
    bannerSprite_->SetPosition({ bannerStartX_, bannerStartY_ });

    auto player = gameScene->GetPlayer();

    if (player && gameScene->GetStageManager()) {
        // 本来のスポーン座標・レール位置を取得
        targetRailPos_ = gameScene->GetStageManager()->GetPlayerSpawnRailPosition();
        targetWorldPos_ = player->GetWorldPosition();

        // プレイヤーの進行方向（正面）を取得
        forwardDir_ = player->GetDirection();
        if (Length(forwardDir_) < 0.001f) {
            forwardDir_ = { 0.0f, 0.0f, 1.0f };
        } else {
            forwardDir_ = Normalize(forwardDir_);
        }

        // 本来の開始地点より手前（約14m手前・画面外）を開始位置とする
        const float runInDistance = 14.0f;
        startWorldPos_ = targetWorldPos_ - forwardDir_ * runInDistance;

        // 初期フレームの位置と向きを設定
        float angleY = std::atan2(forwardDir_.x, forwardDir_.z);
        player->GetObject3d()->SetRotate({ 0.0f, angleY, 0.0f });
        player->GetObject3d()->SetTranslate(startWorldPos_);
        player->GetObject3d()->Update();
    }
}

void StartPhase::Update(Scene* scene)
{
    GameScene* gameScene = static_cast<GameScene*>(scene);
    auto* player = gameScene->GetPlayer();

    if (gameScene->GetGoal()) {
        gameScene->GetGoal()->Update();
    }
    if (gameScene->GetStageRaill()) {
        gameScene->GetStageRaill()->Update();
    }

    bool hasInput = HasMovementInput();
    timer_ += 1.0f / 60.0f;

    // 進行速度: 操作時の快適な速度(7.0m/s)と完全一致させ、14mを2.0秒で駆け込み
    float runSpeed = 1.0f / 2.0f;
    runProgress_ = std::min(runProgress_ + runSpeed * (1.0f / 60.0f), 1.0f);

    if (player) {
        if (runProgress_ < 1.0f) {
            float p = runProgress_;
            // 入力がある時は減速せず等速(Linear)で走り抜け、入力なしならブレーキ(SmoothStep)
            float ease = hasInput ? p : (p * p * (3.0f - 2.0f * p));

            Vector3 currentPos = Lerp(startWorldPos_, targetWorldPos_, ease);
            float angleY = std::atan2(forwardDir_.x, forwardDir_.z);

            // レール位置による上書きを回避し、直接モデルの位置・向き・アニメーションを更新
            player->GetObject3d()->SetRotate({ 0.0f, angleY, 0.0f });
            player->GetObject3d()->SetTranslate(currentPos);
            player->GetObject3d()->Update();

            if (auto* col = player->GetCollider()) {
                col->Update();
            }
        } else if (!isArrived_) {
            // 目標位置に物理的に到着した瞬間にレール位置へ正式同期
            isArrived_ = true;
            player->SetRailPosition(targetRailPos_);
            player->UpdateTransform();
            player->GetObject3d()->Update();
        }
    }

    // カメラコントローラーを更新してプレイヤーを追尾・注視
    if (gameScene->GetCamera()) {
        gameScene->GetCamera()->Update();
    }

    // 画面上部スプライトの移動アニメーション更新（プレイヤー加速に影響されず等速でゆっくり流す）
    bannerTimer_ += 1.0f / 60.0f;
    if (bannerSprite_) {
        float t = std::clamp(bannerTimer_ / kBannerDuration_, 0.0f, 1.0f);
        // 等速（Linear）補間で画面中央でも加速せず、穏やかに流す
        float currentX = bannerStartX_ + (bannerEndX_ - bannerStartX_) * t;
        bannerSprite_->SetPosition({ currentX, bannerStartY_ });
        bannerSprite_->Update();
    }

    // エネミーのトランスフォーム更新
    for (auto& enemy : gameScene->GetEnemies()) {
        if (enemy) {
            enemy->UpdateTransform();
        }
    }

    // 目標地点に「物理的に到着した(runProgress_ >= 1.0f)」ことを前提とした安全な遷移:
    // ・到着済み＋入力あり: 待機時間なしで即座にPlayPhaseへ移行して走り続ける
    // ・到着済み＋入力なし: 到着後 0.5秒のタメを経てPlayPhaseへ移行
    bool isRunFinished = (runProgress_ >= 1.0f);
    bool transitionReady = isRunFinished && !Fade::GetInstance()->IsFading() && (hasInput || timer_ >= (kRunDuration_ + kWaitDuration_));

    if (transitionReady) {
        if (player && !isArrived_) {
            isArrived_ = true;
            player->SetRailPosition(targetRailPos_);
            player->UpdateTransform();
            player->GetObject3d()->Update();
        }
        // スプライトが途中で消えないよう、残りの移動を GameScene に引き継ぐ
        if (bannerSprite_) {
            gameScene->SetBannerSprite(std::move(bannerSprite_), bannerStartX_, bannerEndX_, bannerStartY_, kBannerDuration_, bannerTimer_);
        }
        auto playPhase = std::make_unique<PlayPhase>();
        auto* playPhasePtr = playPhase.get();
        gameScene->ChangePhase(std::move(playPhase));
        // 切り替わった同一フレームで即座にPlayPhaseの更新を実行し、1フレームの停止すら発生させずシームレスに走り抜ける
        playPhasePtr->Update(gameScene);
        return;
    }
}

void StartPhase::Draw(Scene* scene)
{
    if (bannerSprite_) {
        bannerSprite_->Draw();
    }
}

void StartPhase::Finalize(Scene* scene)
{
    bannerSprite_.reset();
}
