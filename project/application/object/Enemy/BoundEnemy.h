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
    };

    static void LoadGlobalConfig(const std::string& filePath = "resources/config/enemy_types.json");
    static void SaveGlobalConfig(const std::string& filePath = "resources/config/enemy_types.json");
    static BoundConfig& GetGlobalConfig() { return s_config; }

    BoundEnemy();
    ~BoundEnemy()override;

    void Initialize() override;
    void Update() override;
    void Draw() override;

private:
    static inline BoundConfig s_config;
};

class EnemyBehaviorBounce : public IEnemyBehavior {
public:
    EnemyBehaviorBounce() = default;
    ~EnemyBehaviorBounce() override = default;

    void Initialize(Enemy* enemy) override;
    void Update(Enemy* enemy) override;
    void Finalize(Enemy* enemy) override;


  

    const char* GetName() const override {
        return "Bounce";
    }

private:

    std::unique_ptr<IEnemyAction> currentAction_;

    const float kBounceForce = 18.0f; // 跳ね上がる強さ（ゲームに合わせて調整してください）
    const float acceleration_ = 24.0f;    // 前進する速度
};