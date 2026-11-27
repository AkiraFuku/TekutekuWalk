#pragma once
#include "Enemy.h"
#include "EnemyBehavior.h"
#include "EnemyAction.h"
class TestRobot;
class Player;


class BoundEnemy :
    public Enemy
{
public:
    struct BoundConfig {
        std::string texture = "resources/taru/taru2.png";
        float jumpAcceleration = 16.0f;
        float gravityScaleFloating = 0.1f;
        float gravityScaleNormal = 0.6f;
        float hoverLimitHeight = 0.2f;
        float detectRadius = 9.0f;
        float telegraphDuration = 0.65f;
        float shakePosAmplitude = 0.12f;
        float shakeRotAmplitude = 0.25f;
        float shakeFrequency = 45.0f;
        float attackSpeed = 12.0f;
        float attackDuration = 1.8f;
        float attackJumpAcceleration = 11.0f;
        float attackCooldown = 2.0f;
    };

    static void LoadGlobalConfig(const std::string& filePath = "resources/config/enemy_types.json");
    static void SaveGlobalConfig(const std::string& filePath = "resources/config/enemy_types.json");
    static BoundConfig& GetGlobalConfig() { return s_config; }

    BoundEnemy();
    ~BoundEnemy()override;

    void Initialize() override;
    void Update() override;
    void Draw() override;
    Vector3 GetCameraFocusPosition() const override;

private:
    static inline BoundConfig s_config;
};

class EnemyBehaviorBounce : public IEnemyBehavior {
public:
    enum class BouncePhase {
        NormalBounce, // 通常その場バウンド
        Telegraph,    // プレイヤー接近時の揺れ予兆
        Attack        // 左右移動での突撃
    };

    EnemyBehaviorBounce() = default;
    ~EnemyBehaviorBounce() override = default;

    void Initialize(Enemy* enemy) override;
    void Update(Enemy* enemy) override;
    void Finalize(Enemy* enemy) override;

    const char* GetName() const override {
        return "Bounce";
    }

    BouncePhase GetPhase() const { return phase_; }
    const Vector3& GetVisualPosOffset() const { return visualPosOffset_; }
    const Vector3& GetVisualRotOffset() const { return visualRotOffset_; }
    const Vector3& GetVisualScaleFactor() const { return visualScaleFactor_; }

private:
    void UpdateBouncePhysics(Enemy* enemy, float jumpAccel);
    void FaceToPlayer(Enemy* enemy, Player* player);

    std::unique_ptr<IEnemyAction> currentAction_;

    BouncePhase phase_ = BouncePhase::NormalBounce;
    float phaseTimer_ = 0.0f;
    float cooldownTimer_ = 0.0f;
    float shakeTime_ = 0.0f;

    Vector3 visualPosOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 visualRotOffset_ = { 0.0f, 0.0f, 0.0f };
    Vector3 visualScaleFactor_ = { 1.0f, 1.0f, 1.0f };

    const float kBounceForce = 18.0f; // 跳ね上がる強さ（ゲームに合わせて調整してください）
    const float acceleration_ = 24.0f;    // 前進する速度
};