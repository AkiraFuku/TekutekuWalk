#include "HoverEnemy.h"
#include "HoverRobot.h"
#include "DXCommon.h"
#include "JsonManager.h"
#include "Logger.h"
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void HoverEnemy::LoadGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    if (!jsonMgr->Load("EnemyTypesConfig", filePath)) {
        return;
    }
    const auto& root = jsonMgr->Get("EnemyTypesConfig");
    if (!root.contains("hover")) return;

    const auto& j = root["hover"];
    s_config.texture = j.value("texture", s_config.texture);
    s_config.baseHeight = j.value("baseHeight", s_config.baseHeight);
    s_config.patrolSwitchInterval = j.value("patrolSwitchInterval", s_config.patrolSwitchInterval);
    s_config.patrolSpeed = j.value("patrolSpeed", s_config.patrolSpeed);
    s_config.hoverAmplitude = j.value("hoverAmplitude", s_config.hoverAmplitude);
    s_config.hoverFrequency = j.value("hoverFrequency", s_config.hoverFrequency);
}

void HoverEnemy::SaveGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    nlohmann::json root;
    if (jsonMgr->Load("EnemyTypesConfig", filePath)) {
        root = jsonMgr->Get("EnemyTypesConfig");
    }

    root["hover"] = {
        {"texture", s_config.texture},
        {"baseHeight", s_config.baseHeight},
        {"patrolSwitchInterval", s_config.patrolSwitchInterval},
        {"patrolSpeed", s_config.patrolSpeed},
        {"hoverAmplitude", s_config.hoverAmplitude},
        {"hoverFrequency", s_config.hoverFrequency}
    };

    jsonMgr->Save(filePath, root);
}

HoverEnemy::HoverEnemy() {}

HoverEnemy::~HoverEnemy() = default;

void HoverEnemy::Initialize() {
    static bool s_loaded = false;
    if (!s_loaded) {
        LoadGlobalConfig();
        s_loaded = true;
    }

    Enemy::Initialize();

    patrolSpeed_ = s_config.patrolSpeed;
    patrolSwitchInterval_ = s_config.patrolSwitchInterval;
    hoverAmplitude_ = s_config.hoverAmplitude;
    hoverFrequency_ = s_config.hoverFrequency;

    SetTexture(s_config.texture);

    baseWorldY_ = GetWorldPosition().y;
    // 地面近くにスポーンした場合は浮遊高度を確保
    if (baseWorldY_ < 1.8f) {
        baseWorldY_ = s_config.baseHeight;
    }

    // ホバーロボットを登録
    SetRobot(std::make_unique<HoverRobot>());
}

void HoverEnemy::Update() {
    float dt = DXCommon::kDeltaTime;
    hoverTimer_ += dt;
    patrolTimer_ += dt;

    // 一定時間ごとに左右反転
    if (patrolTimer_ >= patrolSwitchInterval_) {
        patrolTimer_ = 0.0f;
        patrolDir_ = -patrolDir_;
    }

    // レールに沿って左右パトロール移動
    Move(float(patrolDir_) * patrolSpeed_ * dt);

    // 空中でふわふわ浮遊する上下サイン波運動
    Vector3 pos = GetWorldPosition();
    pos.y = baseWorldY_ + std::sin(hoverTimer_ * hoverFrequency_) * hoverAmplitude_;
    SetPosition(pos);

    UpdateTransform();
}

void HoverEnemy::Draw() {
    Enemy::Draw();
}
