#include "EnemyBehavior.h"
#include "Enemy.h"
#include "EnemyAction.h"
#include "Scene.h"
#include "Player.h"
#include "MathFunction.h"

void EnemyBehaviorPatrol::Initialize(Enemy* enemy)
{
    currentAction_= std::make_unique<MoveAction>(1.0f);
}
EnemyBehaviorPatrol::~EnemyBehaviorPatrol() = default;
void EnemyBehaviorPatrol::Update(Enemy* enemy)
{// 1. Actionを生成して実行する
    // 例: 毎フレーム速度 1.0f で移動するアクションを実行
    currentAction_->Execute(enemy);

    enemy->UpdateGravity();
}

void EnemyBehaviorPatrol::Finalize(Enemy* enemy)
{
}

// --- EnemyBehaviorChase ---
EnemyBehaviorChase::EnemyBehaviorChase() = default;
EnemyBehaviorChase::~EnemyBehaviorChase() = default;

void EnemyBehaviorChase::Initialize(Enemy* enemy)
{
    currentAction_ = std::make_unique<MoveAction>(1.0f);
}

void EnemyBehaviorChase::SetParams(float searchRad, float lostDist, float chaseSpd, float patrolSpd, float retreatSpd, float retreatDur)
{
    searchRadius_ = searchRad;
    lostDistance_ = lostDist;
    chaseSpeed_ = chaseSpd;
    patrolSpeed_ = patrolSpd;
    retreatSpeed_ = retreatSpd;
    retreatDuration_ = retreatDur;
}

void EnemyBehaviorChase::StartRetreat(Enemy* enemy, Player* player)
{
    if (!enemy) return;
    isRetreating_ = true;
    retreatTimer_ = retreatDuration_;

    if (player) {
        float enemyDist = enemy->GetCurrentDistance();
        float playerDist = player->GetCurrentDistance();
        enemy->SetMoveDirection((playerDist >= enemyDist) ? -1.0f : 1.0f);
    }
    enemy->SetMoveSpeed(retreatSpeed_);
}

void EnemyBehaviorChase::Update(Enemy* enemy)
{
    if (!enemy) return;

    // シーンからプレイヤーを取得（ポリモーフィック）
    Player* player = enemy->GetScene() ? enemy->GetScene()->GetPlayer() : nullptr;

    // 【離脱フェーズ】プレイヤー接触後の離脱処理
    if (isRetreating_) {
        retreatTimer_ -= enemy->GetDeltaTime();
        if (retreatTimer_ <= 0.0f) {
            retreatTimer_ = 0.0f;
            isRetreating_ = false;
        }

        // プレイヤーから確実に離れる向きを維持
        if (player) {
            float enemyDist = enemy->GetCurrentDistance();
            float playerDist = player->GetCurrentDistance();
            enemy->SetMoveDirection((playerDist >= enemyDist) ? -1.0f : 1.0f);
        }
        enemy->SetMoveSpeed(retreatSpeed_);

        if (currentAction_) {
            currentAction_->Execute(enemy);
        }
        enemy->UpdateGravity();
        return;
    }

    if (player && player->IsAlive()) {
        Vector3 enemyPos = enemy->GetWorldPosition();
        Vector3 playerPos = player->GetWorldPosition();
        Vector3 diff = Subtract(playerPos, enemyPos);
        float distance = Length(diff);

        // 索敵・追跡の判定（ヒステリシスを持たせてバタつき防止）
        if (!isChasing_ && distance <= searchRadius_) {
            isChasing_ = true; // 検知範囲内に入ったため追跡開始
        } else if (isChasing_ && distance >= lostDistance_) {
            isChasing_ = false; // 見失い距離を超えたため巡回復帰
        }

        if (isChasing_) {
            // 【追跡中】プレイヤーの方向へ向きを変えて高速移動
            enemy->SetMoveSpeed(chaseSpeed_);

            float enemyDist = enemy->GetCurrentDistance();
            float playerDist = player->GetCurrentDistance();

            if (playerDist > enemyDist + 0.1f) {
                enemy->SetMoveDirection(1.0f);  // 前方のプレイヤーを追撃
            } else if (playerDist < enemyDist - 0.1f) {
                enemy->SetMoveDirection(-1.0f); // 後方のプレイヤーを追撃
            }
        } else {
            // 【通常巡回中】低速で移動
            enemy->SetMoveSpeed(patrolSpeed_);
        }
    } else {
        isChasing_ = false;
        enemy->SetMoveSpeed(patrolSpeed_);
    }

    if (currentAction_) {
        currentAction_->Execute(enemy);
    }

    enemy->UpdateGravity();
}

void EnemyBehaviorChase::Finalize(Enemy* enemy)
{
}

