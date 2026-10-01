#include "EnemyBehavior.h"
#include "Enemy.h"
#include "EnemyAction.h"
#include "ChaseEnemy.h"
#include "GameScene.h"
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

void EnemyBehaviorChase::Update(Enemy* enemy)
{
    if (!enemy) return;

    ChaseEnemy* chaser = dynamic_cast<ChaseEnemy*>(enemy);

    float searchRadius = chaser ? chaser->GetSearchRadius() : 10.0f;
    float lostDistance = chaser ? chaser->GetLostDistance() : 14.0f;
    float chaseSpeed   = chaser ? chaser->GetChaseSpeed()   : 5.5f;
    float patrolSpeed  = chaser ? chaser->GetPatrolSpeed()  : 2.0f;
    bool isChasing     = chaser ? chaser->IsChasing()        : false;

    // シーンからプレイヤーを取得
    GameScene* gs = dynamic_cast<GameScene*>(enemy->GetScene());
    Player* player = gs ? gs->GetPlayer() : nullptr;

    // 【離脱フェーズ】プレイヤー接触後の離脱処理
    if (chaser && chaser->IsRetreating()) {
        chaser->UpdateRetreatTimer(enemy->GetDeltaTime());

        // プレイヤーから確実に離れる向きを維持
        if (player) {
            float enemyDist = enemy->GetCurrentDistance();
            float playerDist = player->GetCurrentDistance();
            enemy->SetMoveDirection((playerDist >= enemyDist) ? -1.0f : 1.0f);
        }
        enemy->SetMoveSpeed(chaser->GetRetreatSpeed());

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
        if (!isChasing && distance <= searchRadius) {
            isChasing = true; // 検知範囲内に入ったため追跡開始
        } else if (isChasing && distance >= lostDistance) {
            isChasing = false; // 見失い距離を超えたため巡回復帰
        }

        if (chaser) {
            chaser->SetChasing(isChasing);
        }

        if (isChasing) {
            // 【追跡中】プレイヤーの方向へ向きを変えて高速移動
            enemy->SetMoveSpeed(chaseSpeed);

            float enemyDist = enemy->GetCurrentDistance();
            float playerDist = player->GetCurrentDistance();

            if (playerDist > enemyDist + 0.1f) {
                enemy->SetMoveDirection(1.0f);  // 前方のプレイヤーを追撃
            } else if (playerDist < enemyDist - 0.1f) {
                enemy->SetMoveDirection(-1.0f); // 後方のプレイヤーを追撃
            }
        } else {
            // 【通常巡回中】低速で移動
            enemy->SetMoveSpeed(patrolSpeed);
        }
    } else {
        if (chaser) {
            chaser->SetChasing(false);
        }
        enemy->SetMoveSpeed(patrolSpeed);
    }

    if (currentAction_) {
        currentAction_->Execute(enemy);
    }

    enemy->UpdateGravity();
}

void EnemyBehaviorChase::Finalize(Enemy* enemy)
{
}

