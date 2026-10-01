#include "HoverEnemy.h"
#include "HoverRobot.h"
#include "DXCommon.h"
#include <cmath>

HoverEnemy::HoverEnemy() {}

HoverEnemy::~HoverEnemy() = default;

void HoverEnemy::Initialize() {
    Enemy::Initialize();

    // ホバーエネミー用の外観設定
    if (object_) {
        object_->SetTexture("resources/taru/taru2.png");
    }

    baseWorldY_ = GetWorldPosition().y;
    // 地面近くにスポーンした場合は浮遊高度を確保
    if (baseWorldY_ < 1.8f) {
        baseWorldY_ = 2.2f;
    }

    // ホバーロボットを登録
    SetRobot(std::make_unique<HoverRobot>());
}

void HoverEnemy::Update() {
    float dt = DXCommon::kDeltaTime;
    hoverTimer_ += dt;
    patrolTimer_ += dt;

    // 一定時間ごとに左右反転
    if (patrolTimer_ >= kPatrolSwitchInterval) {
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
