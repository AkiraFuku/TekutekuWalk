#include "ChaseEnemy.h"
#include "TestRobot.h"
#include "Robot.h"
#include "EnemyBehavior.h"
#include "EnemyState.h"
#include "PrimitiveDrawer.h"
#include "Player.h"
#include "JsonManager.h"
#include "Logger.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void ChaseEnemy::LoadGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    if (!jsonMgr->Load("EnemyTypesConfig", filePath)) {
        return;
    }
    const auto& root = jsonMgr->Get("EnemyTypesConfig");
    if (!root.contains("chase")) return;

    const auto& j = root["chase"];
    s_config.texture = j.value("texture", s_config.texture);
    s_config.searchRadius = j.value("searchRadius", s_config.searchRadius);
    s_config.lostDistance = j.value("lostDistance", s_config.lostDistance);
    s_config.chaseSpeed = j.value("chaseSpeed", s_config.chaseSpeed);
    s_config.patrolSpeed = j.value("patrolSpeed", s_config.patrolSpeed);
    s_config.retreatSpeed = j.value("retreatSpeed", s_config.retreatSpeed);
    s_config.retreatDuration = j.value("retreatDuration", s_config.retreatDuration);
}

void ChaseEnemy::SaveGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    nlohmann::json root;
    if (jsonMgr->Load("EnemyTypesConfig", filePath)) {
        root = jsonMgr->Get("EnemyTypesConfig");
    }

    root["chase"] = {
        {"texture", s_config.texture},
        {"searchRadius", s_config.searchRadius},
        {"lostDistance", s_config.lostDistance},
        {"chaseSpeed", s_config.chaseSpeed},
        {"patrolSpeed", s_config.patrolSpeed},
        {"retreatSpeed", s_config.retreatSpeed},
        {"retreatDuration", s_config.retreatDuration}
    };

    jsonMgr->Save(filePath, root);
}

ChaseEnemy::ChaseEnemy() {}
ChaseEnemy::~ChaseEnemy() = default;

void ChaseEnemy::Initialize() {
    static bool s_loaded = false;
    if (!s_loaded) {
        LoadGlobalConfig();
        s_loaded = true;
    }

    Enemy::Initialize();

    searchRadius_ = s_config.searchRadius;
    lostDistance_ = s_config.lostDistance;
    chaseSpeed_ = s_config.chaseSpeed;
    patrolSpeed_ = s_config.patrolSpeed;
    retreatSpeed_ = s_config.retreatSpeed;
    retreatDuration_ = s_config.retreatDuration;

    SetTexture(s_config.texture);

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
