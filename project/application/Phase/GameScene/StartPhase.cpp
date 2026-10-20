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
#include "Collider.h"
void StartPhase::Initialize(Scene* scene)
{
    // フェードインを開始 (暗転 -> 明転)
    Fade::GetInstance()->StartFadeIn(kFadeDuration_);

    GameScene* gameScene = static_cast<GameScene*>(scene);
    timer_ = 0.0f;
    isArrived_ = false;

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

    timer_ += 1.0f / 60.0f;

    if (player) {
        if (timer_ <= kRunDuration_) {
            // 駆け込み中: 最初は勢いよく走り、目標位置の手前でブレーキをかけて止まる (Cubic Ease-Out)
            float t = std::clamp(timer_ / kRunDuration_, 0.0f, 1.0f);
            float ease = 1.0f - std::pow(1.0f - t, 3.0f);

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
            // 到着した瞬間に1度だけ本来のレール位置へ正式同期
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

    // エネミーのトランスフォーム更新
    for (auto& enemy : gameScene->GetEnemies()) {
        if (enemy) {
            enemy->UpdateTransform();
        }
    }

    // 駆け込み演出とフェードインが完了したら PlayPhase に遷移する
    if (timer_ >= (kRunDuration_ + kWaitDuration_) && !Fade::GetInstance()->IsFading()) {
        gameScene->ChangePhase(std::make_unique<PlayPhase>());
    }
}

void StartPhase::Draw(Scene* scene)
{
    // フェードは SceneManager::Draw で一括描画されます
}

void StartPhase::Finalize(Scene* scene)
{
    // 終了処理があれば記載
}
