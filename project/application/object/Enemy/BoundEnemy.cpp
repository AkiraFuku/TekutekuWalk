#include "BoundEnemy.h"

#include "TestEnemy.h"
#include "TestRobot.h"
#include "Robot.h"
#include "Player.h"
#include "PlayerState.h"
#include "EnemyState.h"
#include "boundRobot.h"
#include "Scene.h"
#include "RailMover.h"
#include "MathFunction.h"
#include "JsonManager.h"
#include "Logger.h"
#include <algorithm>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void BoundEnemy::LoadGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    if (!jsonMgr->Load("EnemyTypesConfig", filePath)) {
        return;
    }
    const auto& root = jsonMgr->Get("EnemyTypesConfig");
    if (!root.contains("bound")) return;

    const auto& j = root["bound"];
    s_config.texture = j.value("texture", s_config.texture);
    s_config.jumpAcceleration = j.value("jumpAcceleration", s_config.jumpAcceleration);
    s_config.gravityScaleFloating = j.value("gravityScaleFloating", s_config.gravityScaleFloating);
    s_config.gravityScaleNormal = j.value("gravityScaleNormal", s_config.gravityScaleNormal);
    s_config.hoverLimitHeight = j.value("hoverLimitHeight", s_config.hoverLimitHeight);
    s_config.detectRadius = j.value("detectRadius", s_config.detectRadius);
    s_config.telegraphDuration = j.value("telegraphDuration", s_config.telegraphDuration);
    s_config.shakePosAmplitude = j.value("shakePosAmplitude", s_config.shakePosAmplitude);
    s_config.shakeRotAmplitude = j.value("shakeRotAmplitude", s_config.shakeRotAmplitude);
    s_config.shakeFrequency = j.value("shakeFrequency", s_config.shakeFrequency);
    s_config.attackSpeed = j.value("attackSpeed", s_config.attackSpeed);
    s_config.attackDuration = j.value("attackDuration", s_config.attackDuration);
    s_config.attackJumpAcceleration = j.value("attackJumpAcceleration", s_config.attackJumpAcceleration);
    s_config.attackCooldown = j.value("attackCooldown", s_config.attackCooldown);
}

void BoundEnemy::SaveGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    nlohmann::json root;
    if (jsonMgr->Load("EnemyTypesConfig", filePath)) {
        root = jsonMgr->Get("EnemyTypesConfig");
    }

    root["bound"] = {
        {"texture", s_config.texture},
        {"jumpAcceleration", s_config.jumpAcceleration},
        {"gravityScaleFloating", s_config.gravityScaleFloating},
        {"gravityScaleNormal", s_config.gravityScaleNormal},
        {"hoverLimitHeight", s_config.hoverLimitHeight},
        {"detectRadius", s_config.detectRadius},
        {"telegraphDuration", s_config.telegraphDuration},
        {"shakePosAmplitude", s_config.shakePosAmplitude},
        {"shakeRotAmplitude", s_config.shakeRotAmplitude},
        {"shakeFrequency", s_config.shakeFrequency},
        {"attackSpeed", s_config.attackSpeed},
        {"attackDuration", s_config.attackDuration},
        {"attackJumpAcceleration", s_config.attackJumpAcceleration},
        {"attackCooldown", s_config.attackCooldown}
    };

    jsonMgr->Save(filePath, root);
}

BoundEnemy::BoundEnemy() {}

BoundEnemy::~BoundEnemy() = default;

void BoundEnemy::Initialize() {
    static bool s_loaded = false;
    if (!s_loaded) {
        LoadGlobalConfig();
        s_loaded = true;
    }

    Enemy::Initialize();

    SetTexture(s_config.texture);
    // バウンドエネミー専用のビヘイビアに上書き！
    ChangeBehavior(std::make_unique<EnemyBehaviorBounce>());

    // 基底クラスのメソッドを使ってロボットを登録するだけ
    SetRobot(std::make_unique<BoundRobot>());
}

void BoundEnemy::Update() {
    Enemy::Update();

    if (IsKnockback() || IsDead()) {
        return;
    }

    if (auto* bounceBehavior = dynamic_cast<EnemyBehaviorBounce*>(GetBehavior())) {
        if (auto* obj = GetObject3d()) {
            const Vector3& baseScale = GetConfig().transform.scale;
            const Vector3& scaleFactor = bounceBehavior->GetVisualScaleFactor();
            obj->SetScale({
                baseScale.x * scaleFactor.x,
                baseScale.y * scaleFactor.y,
                baseScale.z * scaleFactor.z
            });
            obj->SetTranslate(Add(obj->GetTranslate(), bounceBehavior->GetVisualPosOffset()));
            obj->SetRotate(Add(obj->GetRotate(), bounceBehavior->GetVisualRotOffset()));
            UpdateMatrix();
        }
    }
}

void BoundEnemy::Draw() {
    Enemy::Draw();
    // ロボット自体は描画が必要ないため、スキップ
}

Vector3 BoundEnemy::GetCameraFocusPosition() const {
    Vector3 pos = GetWorldPosition();
    if (IsRayHit()) {
        pos.y = GetGroundY() + GetConfig().movement.heightOffset;
    }
    return pos;
}

void EnemyBehaviorBounce::Initialize(Enemy* enemy)
{
    (void)enemy;
    currentAction_ = std::make_unique<JumpAction>();
    dynamic_cast<JumpAction*>(currentAction_.get())->SetAcceleration(BoundEnemy::GetGlobalConfig().jumpAcceleration);
    phase_ = BouncePhase::NormalBounce;
    phaseTimer_ = 0.0f;
    cooldownTimer_ = 0.0f;
    shakeTime_ = 0.0f;
    visualPosOffset_ = { 0.0f, 0.0f, 0.0f };
    visualRotOffset_ = { 0.0f, 0.0f, 0.0f };
    visualScaleFactor_ = { 1.0f, 1.0f, 1.0f };
}

void EnemyBehaviorBounce::FaceToPlayer(Enemy* enemy, Player* player)
{
    if (!enemy || !player) {
        return;
    }
    float enemyDist = enemy->GetCurrentDistance();
    float playerDist = player->GetCurrentDistance();
    if (playerDist > enemyDist + 0.05f) {
        enemy->SetMoveDirection(1.0f);
    } else if (playerDist < enemyDist - 0.05f) {
        enemy->SetMoveDirection(-1.0f);
    }
}

void EnemyBehaviorBounce::UpdateBouncePhysics(Enemy* enemy, float jumpAccel)
{
    if (!enemy) {
        return;
    }

    if (enemy->IsGround()) {
        if (auto* jumpAction = dynamic_cast<JumpAction*>(currentAction_.get())) {
            jumpAction->SetAcceleration(jumpAccel);
            jumpAction->Execute(enemy);
        }
    } else {
        float velY = enemy->GetVelocity().y;
        const auto& cfg = BoundEnemy::GetGlobalConfig();
        if (velY > 0.0f) {
            if (velY < 5.0f) {
                enemy->SetGravityScale(cfg.gravityScaleFloating);
            } else {
                enemy->SetGravityScale(cfg.gravityScaleNormal);
            }
        } else {
            float currentY = enemy->GetWorldPosition().y;
            float groundY = enemy->GetGroundY();
            float targetLandingY = groundY + 0.5f;
            float heightFromGround = currentY - targetLandingY;

            if (heightFromGround > cfg.hoverLimitHeight) {
                enemy->SetGravityScale(cfg.gravityScaleFloating);
            } else {
                enemy->SetGravityScale(cfg.gravityScaleNormal);
            }
        }
    }
}

void EnemyBehaviorBounce::Update(Enemy* enemy)
{
    if (!enemy) {
        return;
    }

    const float dt = enemy->GetDeltaTime();
    const auto& cfg = BoundEnemy::GetGlobalConfig();

    visualPosOffset_ = { 0.0f, 0.0f, 0.0f };
    visualRotOffset_ = { 0.0f, 0.0f, 0.0f };
    visualScaleFactor_ = { 1.0f, 1.0f, 1.0f };

    if (cooldownTimer_ > 0.0f) {
        cooldownTimer_ = (std::max)(0.0f, cooldownTimer_ - dt);
    }

    if (enemy->IsKnockback()) {
        phase_ = BouncePhase::NormalBounce;
        enemy->SetMoveSpeed(enemy->GetConfig().movement.moveSpeed);
        return;
    }

    Player* player = enemy->GetScene() ? enemy->GetScene()->GetPlayer() : nullptr;

    switch (phase_) {
    case BouncePhase::NormalBounce:
    {
        bool startTelegraph = false;
        if (cooldownTimer_ <= 0.0f && player && player->IsAlive() && enemy->IsGround()) {
            float dist = Length(Subtract(player->GetWorldPosition(), enemy->GetWorldPosition()));
            if (dist <= cfg.detectRadius) {
                startTelegraph = true;
            }
        }

        if (startTelegraph) {
            phase_ = BouncePhase::Telegraph;
            phaseTimer_ = cfg.telegraphDuration;
            shakeTime_ = 0.0f;
            FaceToPlayer(enemy, player);
        } else {
            UpdateBouncePhysics(enemy, cfg.jumpAcceleration);
        }
        break;
    }

    case BouncePhase::Telegraph:
    {
        phaseTimer_ -= dt;
        shakeTime_ += dt;

        FaceToPlayer(enemy, player);

        float duration = (std::max)(cfg.telegraphDuration, 0.001f);
        float progress = std::clamp(1.0f - (phaseTimer_ / duration), 0.0f, 1.0f);
        float wave = std::sin(shakeTime_ * cfg.shakeFrequency);

        Vector3 railDir = enemy->GetRailMover() ? enemy->GetRailMover()->GetCurrentDirection() : Vector3{ 1.0f, 0.0f, 0.0f };
        visualPosOffset_ = Multiply(wave * cfg.shakePosAmplitude * (0.5f + 0.5f * progress), railDir);
        visualRotOffset_.z = wave * cfg.shakeRotAmplitude;

        float squash = 1.0f - 0.18f * progress;
        float stretch = 1.0f + 0.12f * progress;
        visualScaleFactor_ = { stretch, squash, stretch };

        if (phaseTimer_ <= 0.0f) {
            phase_ = BouncePhase::Attack;
            phaseTimer_ = cfg.attackDuration;
            FaceToPlayer(enemy, player);
            enemy->SetMoveSpeed(cfg.attackSpeed);
            if (enemy->IsGround()) {
                enemy->SetVelocity({ 0.0f, cfg.attackJumpAcceleration, 0.0f });
            }
        }
        break;
    }

    case BouncePhase::Attack:
    {
        phaseTimer_ -= dt;

        enemy->Move(1.0f);
        UpdateBouncePhysics(enemy, cfg.attackJumpAcceleration);
        visualRotOffset_.x = 0.2f;

        if (phaseTimer_ <= 0.0f) {
            phase_ = BouncePhase::NormalBounce;
            cooldownTimer_ = cfg.attackCooldown;
            enemy->SetMoveSpeed(enemy->GetConfig().movement.moveSpeed);
        }
        break;
    }
    }

    enemy->UpdateGravity();
}

void EnemyBehaviorBounce::Finalize(Enemy* enemy)
{}
