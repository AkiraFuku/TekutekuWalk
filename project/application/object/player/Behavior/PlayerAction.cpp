#include "PlayerAction.h"
#include "Player.h"
#include "Projectile.h"
#include "RailPath.h"
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

    // クールダウンタイマー＆射撃姿勢タイマーをセット
    player->ResetShootCooldown();
    player->PlayHitSE();

    // 発射反動の微小な伸縮演出（機関砲の振動感、着地怯み演出中は上書きしない）
    if (!player->IsLandingStaggered()) {
        player->TriggerSquashStretch(-0.04f, 0.05f);
    }

    // 銃口の微小な上下バラつき（機関砲のガタつき）＋実弾の重力落下を見越したわずかな仰角
    shotCount_++;
    float spreadPattern = std::sin(static_cast<float>(shotCount_) * 2.7f) * rfConfig.bulletSpreadY + 0.015f;

    Vector2 finalDir = { static_cast<float>(player->GetMoveDirection()), spreadPattern };
    finalDir = Normalize(finalDir);

    // プレイヤー立方体（半径0.5m）の前面にある銃口位置（約0.65m前方）から発射
    float startProgress = player->GetRailProgress();
    if (const RailPath* rail = player->GetRailPath()) {
        float muzzleDist = player->GetCurrentDistance() + static_cast<float>(player->GetMoveDirection()) * 0.65f;
        startProgress = rail->GetTFromDistance(muzzleDist);
    }

    Projectile::ProjectileSpawnParam param;
    param.position = { startProgress, player->GetWorldY() };
    param.direction = finalDir;
    param.speed = rfConfig.bulletSpeed;

    gameScene->AddProjectile(param, Projectile::ProjectileOwner::Player);
}
