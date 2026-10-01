#pragma once
#include "Enemy.h"
#include "EnemyBehavior.h"
#include "EnemyAction.h"

class HoverRobot;
class Player;

class HoverEnemy : public Enemy {
public:
    HoverEnemy();
    ~HoverEnemy() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;

private:
    float hoverTimer_ = 0.0f;
    float baseWorldY_ = 2.0f;
    float hoverAmplitude_ = 0.35f;
    float hoverFrequency_ = 2.2f;
    float patrolSpeed_ = 2.5f;
    int patrolDir_ = 1;
    float patrolTimer_ = 0.0f;
    const float kPatrolSwitchInterval = 3.0f; // 3秒ごとに折り返し
};
