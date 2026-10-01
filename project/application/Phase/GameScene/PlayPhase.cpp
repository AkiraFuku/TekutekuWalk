#include "PlayPhase.h"
#include "GameScene.h"
#include "Player.h"
#include "Collider.h"
#include "CollisionManager.h"
#include "GoalObject.h"
#include "CameraController.h"
#include "RailPath.h"
#include "Projectile.h"
#include "Enemy.h"
#include "StageManager.h"

void PlayPhase::Initialize(Scene* scene)
{}

void PlayPhase::Update(Scene* scene)
{
    GameScene* gameScene = static_cast<GameScene*>(scene);

    gameScene->GetGoal()->Update();
    gameScene->GetCamera()->Update();
    gameScene->GetStageRaill()->Update();

   

    // 2. ヒットストップ中はキャラクターや物理演算、衝突判定の更新をスキップする
    if (gameScene->IsHitStopActive()) {
        return; // ここから下の更新・衝突判定は一切行わない
    }

    gameScene->GetPlayer()->Update();

    // --- 修正箇所: auto& (参照) で受け取る ---
    const auto& projectiles = gameScene->GetProjectile();
    for (auto& projectile : projectiles) {
        if (projectile) {
            projectile->Update();
        }
    }
    // ※注意：参照で受け取っている場合、ここでの erase は不要（あるいは不可）です。
    // 消去(remove_if)は、元の所有者である GameScene 側で行うべき処理です。

    // --- 修正箇所: auto& (参照) で受け取る ---
    const auto& enemies = gameScene->GetEnemies();
    for (auto& enemy : enemies) {
        enemy->Update();
    }
    // ※同様にここでの erase も削除します。
     // --- 衝突判定の実行 ---
    CollisionManager* colManager = CollisionManager::GetInstance();
    std::vector<Collider*> colliders;

    const auto& player = gameScene->GetPlayer();
    const auto& goal_ = gameScene->GetGoal();


    if (player) {
        if (Collider* col = player->GetCollider()) {
            colliders.push_back(col);
        }
        if (Collider* atkCol = player->GetAttackCollider()) {
            if (atkCol->IsCollide()) {
                colliders.push_back(atkCol);
            }
        }
    }


    for (auto& enemy : enemies) {
        if (enemy && !enemy->IsDead() && !enemy->IsKnockback()) {
            if (Collider* col = enemy->GetCollider()) {
                colliders.push_back(col);
            }
        }
    }

    for (auto& projectile : projectiles) {
        if (projectile && !projectile->IsDead()) {
            if (Collider* col = projectile->GetCollider()) {
                colliders.push_back(col);
            }
        }
    }

    if (goal_) {
        if (Collider* col = goal_->GetCollider()) {
            colliders.push_back(col);
        }
    }

    //// コインのコライダー登録
    //if (auto* stageMgr = gameScene->GetStageManager()) {
    //    for (auto& coin : stageMgr->GetCoins()) {
    //        if (coin && !coin->IsDead() && !coin->IsCollected()) {
    //            if (Collider* col = coin->GetCollider()) {
    //                colliders.push_back(col);
    //            }
    //        }
    //    }
    //}

    colManager->CheckAllCollisions(colliders);


}

void PlayPhase::Draw(Scene* scene)
{

}

void PlayPhase::Finalize(Scene* scene)
{

            // 必要に応じてフェードアウト演出やSE再生をここで行う
        if (Audio::GetInstance()->IsPlaying(scene->getBGMPlayHundle())) {
            Audio::GetInstance()->PauseAudio(scene->getBGMPlayHundle());
        }
}
