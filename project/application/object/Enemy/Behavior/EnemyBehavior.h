#pragma once
#include <memory>
#include "BehaviorState.h"
class Enemy;
class IEnemyAction;

class IEnemyBehavior :public IBehavior {
public:
    virtual ~IEnemyBehavior() = default; // 基底クラスの仮想デストラクタ
    virtual void Initialize(Enemy* enemy) = 0;
    virtual void Update(Enemy* enemy) = 0;
    virtual void Finalize(Enemy* enemy) = 0;

};
class EnemyBehaviorPatrol : public IEnemyBehavior {
public:

    EnemyBehaviorPatrol()=default;  // コンストラクタを宣言
    ~EnemyBehaviorPatrol(); // ★デストラクタをここで宣言（インライン実装しない）

    void Initialize(Enemy* enemy) override;
    void Update(Enemy* enemy) override;
    void Finalize(Enemy* enemy) override;
    const char* GetName() const override {
        return "Patrol";
    }
private:

    std::unique_ptr<IEnemyAction> currentAction_;

};

class EnemyBehaviorChase : public IEnemyBehavior {
public:
    EnemyBehaviorChase();
    ~EnemyBehaviorChase() override;

    void Initialize(Enemy* enemy) override;
    void Update(Enemy* enemy) override;
    void Finalize(Enemy* enemy) override;
    const char* GetName() const override {
        return "Chase";
    }

    void SetParams(float searchRad, float lostDist, float chaseSpd, float patrolSpd, float retreatSpd = 4.5f, float retreatDur = 2.0f);
    void StartRetreat(Enemy* enemy, Player* player);
    bool IsChasing() const { return isChasing_; }
    bool IsRetreating() const { return isRetreating_; }

private:
    std::unique_ptr<IEnemyAction> currentAction_;
    float searchRadius_ = 10.0f;
    float lostDistance_ = 14.0f;
    float chaseSpeed_ = 5.5f;
    float patrolSpeed_ = 2.0f;
    bool isChasing_ = false;

    float retreatSpeed_ = 4.5f;
    float retreatDuration_ = 2.0f;
    float retreatTimer_ = 0.0f;
    bool isRetreating_ = false;
};