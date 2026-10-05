#include "HoverEnemy.h"
#include "HoverRobot.h"
#include "DXCommon.h"
#include <cmath>

HoverEnemy::HoverEnemy() {}

HoverEnemy::~HoverEnemy() = default;

void HoverEnemy::Initialize() {
    Enemy::Initialize();

    SetTexture("resources/taru/taru2.png");

    baseWorldY_ = GetWorldPosition().y;
    // 地面近くにスポーンした場合は浮遊高度を確保
    if (baseWorldY_ < 1.8f) {
        baseWorldY_ = 2.2f;
    }

    // ホバーロボットを登録
    SetRobot(std::make_unique<HoverRobot>());
}

void HoverEnemy::Update() {
    float dt = GetDeltaTime();

    if (!IsKnockback()) {
        hoverTimer_ += dt;
        patrolTimer_ += dt;

        // 一定時間ごとに左右反転
        if (patrolTimer_ >= kPatrolSwitchInterval) {
            patrolTimer_ = 0.0f;
            patrolDir_ = -patrolDir_;
        }

        // レールに沿って左右パトロール移動 (MoveSpeedに対する比率を渡す)
        float moveRatio = (GetMoveSpeed() > 0.001f) ? (patrolSpeed_ / GetMoveSpeed()) : 1.0f;
        Move(float(patrolDir_) * moveRatio);

        // 空中でふわふわ浮遊する上下サイン波運動
        Vector3 pos = GetWorldPosition();
        pos.y = baseWorldY_ + std::sin(hoverTimer_ * hoverFrequency_) * hoverAmplitude_;
        SetPosition(pos);
    }

    // 基底クラスのUpdateを実行（被弾クールダウン・ノックバック・死亡処理の更新）
    Enemy::Update();
}

void HoverEnemy::Draw() {
    Enemy::Draw();
}
