#include "PlayerAction.h"
#include "Player.h"
#include "Projectile.h"
#include "Scene.h"
#include "GameScene.h"
#include "MathFunction.h"
#include "EffectManager.h"

void NormalMoveAction::Execute(Player* player) {
    // Player.cpp にあった移動ロジックをここに移譲
    player->Move(speed_);
}

void NormalJumpAction::Execute(Player* player) {
    player->Jump();

}
// PlayerAction.cpp
void NormalAttackAction::Execute(Player* player) {
    EffectManager::GetInstance()->TriggerEffect(PostEffectFlag::RadialBlur, 0.75f, EffectManager::CreateRadialBlurFunctional(0.03f, 0.0f));
    player->PlayHitSE();
}

void ShootRobotAction::Execute(Player* player) {
    // PlayerAction.cpp
        player->PlayHitSE();


    Scene* scene = player->GetScene();
    GameScene* gameScene = dynamic_cast<GameScene*>(scene);
    if (!gameScene) return;

    // --- 修正箇所 ---

    // 弾の速さ（スカラー値）を定義
    float baseSpeed = 0.5f;

    // 入力方向ベクトル (aimDir_) をそのまま使い、速さを掛ける
    // もし入力がない (0,0) の場合は、プレイヤーの向いている方向に飛ばす
    Vector2 finalDir = aimDir_;
    if (finalDir.x == 0.0f && finalDir.y == 0.0f) {
        finalDir = { (float)player->GetMoveDirection(), 0.0f };
    }

    // 方向を正規化（斜め入力でも速さが変わらないようにする）
    finalDir = Normalize(finalDir);

    // 弾のパラメータを設定
    Projectile::ProjectileSpawnParam param;
    param.position = { player->GetRailProgress(), player->GetCenterPosition().y };
    param.direction = finalDir; // ここで上下(y)も含まれたベクトルを渡す
    param.speed = baseSpeed;

    gameScene->AddProjectile(param, Projectile::ProjectileOwner::Player);

    // 状態を戻す
    player->ChangeBehavior(std::make_unique<BehaviorRoot>());
    player->ChangeState(PlayerStateFactory::CreateState(PlayerFormType::Normal));
}
