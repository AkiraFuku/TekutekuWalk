#pragma once
#include "Enemy.h"
#include "EnemyBehavior.h"
#include "EnemyAction.h"

class HoverRobot;
class Player;

class HoverEnemy : public Enemy {
public:
    struct HoverConfig {
        std::string texture = "resources/taru/taru2.png";
        float baseHeight = 2.2f;
        float patrolSwitchInterval = 3.0f;
        float patrolSpeed = 2.0f;
        float hoverAmplitude = 0.3f;
        float hoverFrequency = 2.0f;
    };

    static void LoadGlobalConfig(const std::string& filePath = "resources/config/enemy_types.json");
    static void SaveGlobalConfig(const std::string& filePath = "resources/config/enemy_types.json");
    static HoverConfig& GetGlobalConfig() { return s_config; }

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
    float patrolSwitchInterval_ = 3.0f; // 折り返し間隔（秒）

    static inline HoverConfig s_config;
};
