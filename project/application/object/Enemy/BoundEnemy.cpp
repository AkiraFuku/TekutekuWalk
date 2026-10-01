#include "BoundEnemy.h"

#include "TestEnemy.h"
#include "TestRobot.h"
#include "Robot.h"
#include "Player.h"
#include "PlayerState.h"
#include "EnemyState.h"
#include "boundRobot.h"
BoundEnemy::BoundEnemy() {}

BoundEnemy::~BoundEnemy() = default;

void BoundEnemy::Initialize() {
    Enemy::Initialize();

    SetTexture("resources/taru/taru2.png");
    // バウンドエネミー専用のビヘイビアに上書き！
    ChangeBehavior(std::make_unique<EnemyBehaviorBounce>());

    // 基底クラスのメソッドを使ってロボットを登録するだけ
    SetRobot(std::make_unique<BoundRobot>());
}

void BoundEnemy::Update() {
    Enemy::Update();


    // ロボット自体は更新が必要ないため、スキップ
}

void BoundEnemy::Draw() {
    Enemy::Draw();


    // ロボット自体は描画が必要ないため、スキップ
}

void EnemyBehaviorBounce::Initialize(Enemy* enemy)
{

    currentAction_ = std::make_unique<JumpAction>();
    dynamic_cast<JumpAction*>(currentAction_.get())->SetAcceleration(16.0f);

}

void EnemyBehaviorBounce::Update(Enemy* enemy)
{
    if (enemy->IsGround()) {
        // 【地上にいるとき】
        // 1. 前進はストップ or 減速
        // 2. 着地タメタイマーを進める
      //  bounceTimer_ += enemy->GetDeltaTime(); // デルタタイムを利用

        // 3. タメが終わったらジャンプ！
   //     if (bounceTimer_ >= kChargeTime) {
        currentAction_->Execute(enemy); // JumpActionを実行
        //    bounceTimer_ = 0.0f;
        // }

         // 地上ではアニメーションを縮める（潰れ演出）などの処理
    } else {
        // 【空中にいるとき】
        float velY = enemy->GetVelocity().y;

        if (velY > 0.0f) {
            // ① 上昇中：頂点に達するまでフワッとさせる
            //（ここを緩やかにすると、しっかり高さが出ます）
            if (velY < 5.0f) {
                enemy->SetGravityScale(0.1f);
            } else {
                enemy->SetGravityScale(0.6f);
            }
        } else {
          // ② 下降中（ここが落下のホバリング計算コードです）

            float currentY = enemy->GetWorldPosition().y;
            float groundY = enemy->GetGroundY(); // 追加された関数で地面のYを取得[cite: 54]
            
            // 地面の正確な着地目標の高さ（groundY + 補正値）
            // ※Enemy::UpdateGravity内の補正に合わせるため 0.5f（kHeightOffset）を足します[cite: 53]
            float targetLandingY = groundY + 0.5f;

            // 地面からの残り距離（高さの差分）を計算
            float heightFromGround = currentY - targetLandingY;

            // 【調整用パラメータ】
            // 地面からこの高さ（例: 0.2f）より上にいる間は、ずっと重力を弱くしてホバリングさせる
            float hoverLimitHeight = 0.2f;

            if (heightFromGround > hoverLimitHeight) {
                // まだ地面から離れているので、ゆっくり降下（ホバリング維持）
                enemy->SetGravityScale(0.1f);
            } else {
                // 地面ギリギリ（残り20cm等）まで来たら、着地漏れを防ぐために通常の重力に戻して優しく着地させる
                enemy->SetGravityScale(0.6f);
            }
        }
    }
    enemy->UpdateGravity();
}

void EnemyBehaviorBounce::Finalize(Enemy* enemy)
{}
