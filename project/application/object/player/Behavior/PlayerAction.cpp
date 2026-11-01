#include "PlayerAction.h"
#include "Player.h"
#include "Projectile.h"
#include "Scene.h"
#include "GameScene.h"
#include "MathFunction.h"
#include "EffectManager.h"
#include <cmath>

void NormalMoveAction::Execute(Player* player) {
    // Player.cpp にあった移動ロジックをここに移譲
    player->Move(speed_);
}

void NormalJumpAction::Execute(Player* player) {
    player->Jump();
}

void NormalAttackAction::Execute(Player* player) {
    // 近接打撃開始時の重厚なブラー＆伸縮演出・SE
    EffectManager::GetInstance()->TriggerEffect(
        PostEffectFlag::RadialBlur, 0.25f,
        EffectManager::CreateRadialBlurFunctional(0.02f, 0.0f));
    player->TriggerSquashStretch(-0.18f, 0.15f); // 踏み込み時の低い姿勢（沈み込み）
    player->PlayHitSE();
}

void RapidFireAction::Execute(Player* player) {
    if (!player || !player->CanShoot()) return;

    Scene* scene = player->GetScene();
    GameScene* gameScene = dynamic_cast<GameScene*>(scene);
    if (!gameScene) return;

    const auto& rfConfig = player->GetConfig().rapidFire;

    // クールダウンタイマーをセット
    player->ResetShootCooldown();
    player->PlayHitSE();

    // 発射反動の微小な伸縮演出（機関銃の振動感）
    player->TriggerSquashStretch(-0.06f, 0.06f);

    // 銃口の微小な上下バラつき（弾幕感）
    shotCount_++;
    float spreadPattern = std::sin(static_cast<float>(shotCount_) * 2.3f) * rfConfig.bulletSpreadY;

    Vector2 finalDir = { static_cast<float>(player->GetMoveDirection()), spreadPattern };
    finalDir = Normalize(finalDir);

    Projectile::ProjectileSpawnParam param;
    float forwardOffset = static_cast<float>(player->GetMoveDirection()) * 0.002f;
    param.position = { player->GetRailProgress() + forwardOffset, player->GetCenterPosition().y + 0.05f };
    param.direction = finalDir;
    param.speed = rfConfig.bulletSpeed;

    gameScene->AddProjectile(param, Projectile::ProjectileOwner::Player);
}
