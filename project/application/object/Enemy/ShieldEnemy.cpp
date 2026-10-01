#include "ShieldEnemy.h"
#include "ShieldRobot.h"
#include "Player.h"
#include "DXCommon.h"
#include "RailMover.h"

ShieldEnemy::ShieldEnemy() {}

ShieldEnemy::~ShieldEnemy() = default;

void ShieldEnemy::Initialize() {
    Enemy::Initialize();

    if (object_) {
        // 通常の樽モデルを使用（テクスチャで差別化も可能）
        object_->SetTexture("resources/taru/taru.png");
    }

    // シールドロボットを登録
    SetRobot(std::make_unique<ShieldRobot>());
}

void ShieldEnemy::Update() {
    Enemy::Update();
}

void ShieldEnemy::Draw() {
    Enemy::Draw();
}

void ShieldEnemy::OnCollision(GameObject* other) {
    Player* player = dynamic_cast<Player*>(other);
    if (!player) {
        Enemy::OnCollision(other);
        return;
    }

    // プレイヤーが攻撃中（突進攻撃中）の場合
    if (player->IsAttackHitboxActive()) {
        // プレイヤーと敵の位置関係から正面判定（敵は通常、進行方向逆向き＝手前を向いて構える）
        float enemyDist = railMover_ ? railMover_->GetCurrentDistance() : GetWorldPosition().x;
        float playerDist = player->GetRailMover() ? player->GetRailMover()->GetCurrentDistance() : player->GetWorldPosition().x;

        // プレイヤーが手前（左）から敵に向かって正面突進してきた場合
        bool isFrontAttack = (playerDist <= enemyDist);

        if (isFrontAttack) {
            // シールドで防御成功！プレイヤーの攻撃を弾き返し、無敵余韻を与えてダメージを与えない
            player->TriggerInvincibility(0.35f);
            player->PlayHitSE();
            return;
        }
    }

    // 背後からの攻撃、または非攻撃接触は基底クラスの処理（撃破・乗っ取り等）へ委譲
    Enemy::OnCollision(other);
}
