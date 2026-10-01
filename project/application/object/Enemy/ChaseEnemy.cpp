#include "ChaseEnemy.h"
#include "TestRobot.h"
#include "Robot.h"
#include "EnemyBehavior.h"
#include "EnemyState.h"
#include "PrimitiveDrawer.h"
#include "imgui.h"
#include "Player.h"

ChaseEnemy::ChaseEnemy() {}
ChaseEnemy::~ChaseEnemy() = default;

void ChaseEnemy::Initialize() {
    Enemy::Initialize();

    // ユーザー指定の taru3.png を適用
    object_->SetTexture("resources/taru/taru3.png");

    // 初期移動速度を巡回速度に設定
    SetMoveSpeed(patrolSpeed_);

    // 追跡専用ビヘイビアを設定
    ChangeBehavior(std::make_unique<EnemyBehaviorChase>());

    // 撃破時に出現するロボットを設定
    SetRobot(std::make_unique<TestRobot>());
}

void ChaseEnemy::Update() {
    Enemy::Update();
}

void ChaseEnemy::OnCollideWithPlayer(Player* player) {
    if (!player) return;
    // すでに離脱中、または被弾・ノックバック・死亡演出中はスキップ
    if (isRetreating_ || isDamaged_ || isKnockback_ || isDeathFinished_) return;

    StartRetreat(player);
}

void ChaseEnemy::StartRetreat(Player* player) {
    isRetreating_ = true;
    retreatTimer_ = retreatDuration_;

    // プレイヤーから遠ざかる方向へ向きを設定
    if (player) {
        float enemyDist = GetCurrentDistance();
        float playerDist = player->GetCurrentDistance();
        // プレイヤーが前方にいれば後退(-1.0f)、後方にいれば前進(1.0f)
        SetMoveDirection((playerDist >= enemyDist) ? -1.0f : 1.0f);
    }
    SetMoveSpeed(retreatSpeed_);
}

void ChaseEnemy::UpdateRetreatTimer(float deltaTime) {
    if (retreatTimer_ > 0.0f) {
        retreatTimer_ -= deltaTime;
        if (retreatTimer_ <= 0.0f) {
            retreatTimer_ = 0.0f;
            isRetreating_ = false; // 離脱完了、通常の追跡・巡回に復帰
        }
    }
}

void ChaseEnemy::Draw() {
    Enemy::Draw();

#ifdef USE_LINE
    // デバッグ用：検知範囲スフィアの可視化（離脱中は青、追跡中は赤、索敵中は黄色）
    Sphere searchSphere;
    searchSphere.center = GetWorldPosition();
    searchSphere.radius = isChasing_ ? lostDistance_ : searchRadius_;
    Vector4 sphereColor = isRetreating_ ? Vector4{ 0.2f, 0.5f, 1.0f, 0.6f } :
                          (isChasing_ ? Vector4{ 1.0f, 0.2f, 0.2f, 0.6f } : Vector4{ 1.0f, 0.9f, 0.2f, 0.4f });
    PrimitiveDrawer::GetInstance()->DrawSphere(searchSphere, sphereColor);
#endif // USE_LINE
}
