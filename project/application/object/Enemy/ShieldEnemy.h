#pragma once
#include "Enemy.h"
#include "EnemyBehavior.h"
#include "EnemyAction.h"

class ShieldRobot;
class Player;

class ShieldEnemy : public Enemy {
public:
    ShieldEnemy();
    ~ShieldEnemy() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void OnCollision(GameObject* other) override;

private:
    int facingDir_ = -1; // プレイヤーの方向 (-1: 左/後方, 1: 右/前方)
};
