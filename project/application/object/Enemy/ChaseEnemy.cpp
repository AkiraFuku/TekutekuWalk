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

    SetTexture("resources/taru/taru3.png");

    // 初期移動速度を巡回速度に設定
    SetMoveSpeed(patrolSpeed_);

    // 追跡専用ビヘイビアを設定
    auto chaseBehavior = std::make_unique<EnemyBehaviorChase>();
    chaseBehavior->SetParams(searchRadius_, lostDistance_, chaseSpeed_, patrolSpeed_, retreatSpeed_, retreatDuration_);
    ChangeBehavior(std::move(chaseBehavior));

    // 撃破時に出現するロボットを設定
    SetRobot(std::make_unique<TestRobot>());
}

void ChaseEnemy::Update() {
    Enemy::Update();
}

void ChaseEnemy::ApplyProperties(const std::unordered_map<std::string, std::string>& properties) {
    auto itRad = properties.find("search_radius");
    if (itRad != properties.end()) {
        try { SetSearchRadius(std::stof(itRad->second)); } catch (...) {}
    }
    auto itLost = properties.find("lost_distance");
    if (itLost != properties.end()) {
        try { SetLostDistance(std::stof(itLost->second)); } catch (...) {}
    }
    auto itSpeed = properties.find("chase_speed");
    if (itSpeed != properties.end()) {
        try { SetChaseSpeed(std::stof(itSpeed->second)); } catch (...) {}
    }
    auto itPatrol = properties.find("patrol_speed");
    if (itPatrol != properties.end()) {
        try {
            SetPatrolSpeed(std::stof(itPatrol->second));
            SetMoveSpeed(patrolSpeed_);
        } catch (...) {}
    }

    if (auto chaseBehavior = dynamic_cast<EnemyBehaviorChase*>(GetBehavior())) {
        chaseBehavior->SetParams(searchRadius_, lostDistance_, chaseSpeed_, patrolSpeed_, retreatSpeed_, retreatDuration_);
    }
}

void ChaseEnemy::OnCollideWithPlayer(Player* player) {
    if (!player) return;
    // すでに被弾・ノックバック・死亡演出中はスキップ
    if (IsDamaged() || IsKnockback() || IsDeathFinished()) return;

    if (auto chaseBehavior = dynamic_cast<EnemyBehaviorChase*>(GetBehavior())) {
        if (chaseBehavior->IsRetreating()) return;
        chaseBehavior->StartRetreat(this, player);
    } else {
        if (isRetreating_) return;
        StartRetreat(player);
    }
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
