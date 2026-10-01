#pragma once
#include "Enemy.h"

class ChaseEnemy : public Enemy {
public:
    ChaseEnemy();
    ~ChaseEnemy() override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    void OnCollideWithPlayer(Player* player) override; // プレイヤー接触時の退避処理
    void ApplyProperties(const std::unordered_map<std::string, std::string>& properties) override;

    // 追跡パラメータのゲッター・セッター
    float GetSearchRadius() const { return searchRadius_; }
    void SetSearchRadius(float radius) { searchRadius_ = radius; }

    float GetLostDistance() const { return lostDistance_; }
    void SetLostDistance(float dist) { lostDistance_ = dist; }

    float GetChaseSpeed() const { return chaseSpeed_; }
    void SetChaseSpeed(float speed) { chaseSpeed_ = speed; }

    float GetPatrolSpeed() const { return patrolSpeed_; }
    void SetPatrolSpeed(float speed) { patrolSpeed_ = speed; }

    bool IsChasing() const { return isChasing_; }
    void SetChasing(bool chasing) { isChasing_ = chasing; }

    // 離脱（退避）パラメータのゲッター・セッター
    float GetRetreatDuration() const { return retreatDuration_; }
    void SetRetreatDuration(float duration) { retreatDuration_ = duration; }

    float GetRetreatSpeed() const { return retreatSpeed_; }
    void SetRetreatSpeed(float speed) { retreatSpeed_ = speed; }

    float GetRetreatTimer() const { return retreatTimer_; }
    bool IsRetreating() const { return isRetreating_; }
    void SetRetreating(bool retreating) { isRetreating_ = retreating; }

    // 離脱処理の開始・タイマー更新
    void StartRetreat(Player* player);
    void UpdateRetreatTimer(float deltaTime);

private:
    float searchRadius_ = 10.0f;  // プレイヤー検知・追跡開始範囲 (m)
    float lostDistance_ = 14.0f;  // 追跡解除・見失い距離 (m)
    float chaseSpeed_ = 5.5f;     // 追跡時の移動速度 (m/s)
    float patrolSpeed_ = 2.0f;    // 索敵・巡回時の移動速度 (m/s)
    bool isChasing_ = false;      // 現在追跡中かどうか

    float retreatDuration_ = 2.0f; // 接触後に離れる時間（秒）
    float retreatSpeed_ = 4.5f;    // 離れるときの移動速度 (m/s)
    float retreatTimer_ = 0.0f;    // 離脱タイマー
    bool isRetreating_ = false;    // 現在離脱中かどうか
};
