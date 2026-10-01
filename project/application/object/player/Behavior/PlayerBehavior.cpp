#include "PlayerBehavior.h"
#include "Player.h"
#include "Command.h"
#include "PlayerAction.h"
#include "PlayerState.h"
#include "IPlayerFactory.h" // ★ Factory のヘッダーをインクルード

// --- Helper 関数（State から Factory を安全に取得） ---
static IPlayerFactory* GetFactoryFromPlayer(Player* player) {
    auto state = player->GetState();
    return state ? state->GetFactory() : nullptr;
}

// --- BehaviorRoot ---
void BehaviorRoot::Initialize(Player* player) {}

void BehaviorRoot::Update(Player* player) {
    player->RayCastUpdate();
    player->UpdateGravity();
}

void BehaviorRoot::Finalize(Player* player) {}

void BehaviorRoot::HandleInput(Player* player, ICommand* command) {
    auto state = player->GetState();
    if (!state) return;

    auto moveAction = state->GetMoveAction();

    if (dynamic_cast<DashCommand*>(command)) {
        player->SetDashing(true);
        return;
    }

    if (auto moveCmd = dynamic_cast<MoveCommand*>(command)) {
        if (moveAction) {
            static_cast<NormalMoveAction*>(moveAction)->SetSpeed(moveCmd->GetSpeed());
            moveAction->Execute(player);
        }
    }

    // ★ Factory を取得して生成するように変更
    auto factory = state->GetFactory();
    if (!factory) return;

    if (dynamic_cast<SlideCommand*>(command)) {
        if (player->IsGround()) {
            state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Slide));
            return;
        }
    }

    if (dynamic_cast<JumpCommand*>(command)) {
        state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Jump));
    }
    if (dynamic_cast<AttackCommand*>(command)) {
        state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Attack));
    }
    if (dynamic_cast<GuardCommand*>(command)) {
        if (dynamic_cast<StateShield*>(state)) {
            state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Guard));
            return;
        }
    }

    if (dynamic_cast<PreShootCommand*>(command)) {
        if (dynamic_cast<IStateRideOn*>(state)) {
            state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Aim));
        }
    }
}

// --- BehaviorAttack ---
void BehaviorAttack::Initialize(Player* player) {
    timer_ = 0.0f;
    player->SetInvincible(true); // ダッシュ攻撃中は完全無敵
    player->SetAttackHitboxActive(true); // 攻撃ヒットボックスを有効化

    auto state = player->GetState();
    if (state) {
        auto attackAction = state->GetAttackAction_();
        if (attackAction) {
            attackAction->Execute(player);
        }
    }
}

void BehaviorAttack::Update(Player* player) {
    auto state = player->GetState();
    if (state) {
        auto attackAction = state->GetAttackAction_();
        if (attackAction) {
            // 高速突進（攻撃判定時間）を 0.15f から 0.28f に延長
            float speedMultiplier = (timer_ < 0.28f) ? 2.5f : 0.2f;
            int attackDir = player->GetMoveDirection();
            player->Move(float(attackDir) * 0.8f * speedMultiplier);
            player->UpdateRailPath();

            // 高速突進が終了したら攻撃判定をオフ
            if (timer_ >= 0.28f) {
                player->SetAttackHitboxActive(false);
            }
        }
    }

    // ダッシュ移動後の位置で床・壁のレイキャストを更新し、接地スナップと重力を適用（地面へのめり込み防止）
    player->RayCastUpdate();
    player->UpdateGravity();
    player->UpdateRailPath();

    timer_ += player->GetDeltaTime();
    if (timer_ >= kAttackDuration) {
        if (state) {
            // ★ Factory 経由で Root に戻る
            if (auto factory = state->GetFactory()) {
                state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Root));
            }
        }
    }
}

void BehaviorAttack::Finalize(Player* player) {
    player->SetInvincible(false);
    player->SetAttackHitboxActive(false); // 攻撃終了時にヒットボックスを確実にオフ
    player->TriggerInvincibility(0.35f); // 攻撃終了後0.35秒間の無敵余韻を付与
}
void BehaviorAttack::HandleInput(Player* player, ICommand* command) {}

// --- BehaviorJump ---
void BehaviorJump::Initialize(Player* player) {
    auto state = player->GetState();
    if (state) {
        auto jumpAction = state->GetJumpAction();
        if (jumpAction) {
            jumpAction->Execute(player);
        }
    }
}

void BehaviorJump::Update(Player* player) {
    if (player->IsGround()) {
        // 着地時に先行入力が残っていれば即座に再ジャンプを実行
        if (player->TryExecuteBufferedJump()) {
            return;
        }
        if (auto state = player->GetState()) {
            // ★ Factory 経由で Root に戻る
            if (auto factory = state->GetFactory()) {
                state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Root));
            }
        }
    }
    player->UpdateGravity();
}

void BehaviorJump::Finalize(Player* player) {}

void BehaviorJump::HandleInput(Player* player, ICommand* command) {
    auto state = player->GetState();
    if (!state) return;

    auto moveAction = state->GetMoveAction();

    if (auto moveCmd = dynamic_cast<MoveCommand*>(command)) {
        if (moveAction) {
            static_cast<NormalMoveAction*>(moveAction)->SetSpeed(moveCmd->GetSpeed());
            moveAction->Execute(player);
        }
    }

    // ★ Factory を取得して生成
    auto factory = state->GetFactory();
    if (!factory) return;

    // 空中滞空中でもジャンプボタンの先行入力を受け付ける
    if (dynamic_cast<JumpCommand*>(command)) {
        player->Jump();
    }

    if (dynamic_cast<AttackCommand*>(command)) {
        state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Attack));
    }
    if (dynamic_cast<PreShootCommand*>(command)) {
        if (dynamic_cast<IStateRideOn*>(state)) {
            state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Aim));
        }
    }
}

// --- BehaviorAim ---
void BehaviorAim::Initialize(Player* player) {
    aimX_ = (float)player->GetMoveDirection();
    aimY_ = 0.0f;
}

void BehaviorAim::Update(Player* player) {}
void BehaviorAim::Finalize(Player* player) {}

void BehaviorAim::HandleInput(Player* player, ICommand* command) {
    if (auto aimCmd = dynamic_cast<AimCommand*>(command)) {
        aimX_ = aimCmd->GetX();
        aimY_ = aimCmd->GetY();
    }

    if (dynamic_cast<ShootCommand*>(command)) {
        auto state = dynamic_cast<IStateRideOn*>(player->GetState());
        if (state) {
            auto shootAction = state->GetShootAction();
            if (shootAction) {
                static_cast<ShootRobotAction*>(shootAction)->SetAimVector(aimX_, aimY_);
                shootAction->Execute(player);
            }
        }
    }
}
// --- BehaviorBound ---
void BehaviorBound::Initialize(Player* player) {
    // 状態開始と同時に最初の跳躍を実行
    if (auto state = player->GetState()) {
        if (auto jumpAction = state->GetJumpAction()) {
            jumpAction->Execute(player);
        }
    }
}

void BehaviorBound::Update(Player* player) {
    // 地面に就いた瞬間に自動で再度ジャンプする（常に跳ね続ける）
    if (player->IsGround()) {
        if (auto state = player->GetState()) {
            if (auto jumpAction = state->GetJumpAction()) {
                jumpAction->Execute(player);
            }
        }
    }

    // 重力更新
    player->UpdateGravity();
}

void BehaviorBound::Finalize(Player* player) {}

void BehaviorBound::HandleInput(Player* player, ICommand* command) {
    auto state = player->GetState();
    if (!state) return;

    // 空中・着地問わず左右移動は受け付ける
    if (auto moveCmd = dynamic_cast<MoveCommand*>(command)) {
        if (auto moveAction = state->GetMoveAction()) {
            static_cast<NormalMoveAction*>(moveAction)->SetSpeed(moveCmd->GetSpeed());
            moveAction->Execute(player);
        }
    }
    // ★ Factory を取得して生成
    auto factory = state->GetFactory();
    if (!factory) return;

    if (dynamic_cast<AttackCommand*>(command)) {
        state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Attack));
    }
    if (dynamic_cast<PreShootCommand*>(command)) {
        if (dynamic_cast<IStateRideOn*>(state)) {
            state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Aim));
        }
    }
}

// --- BehaviorSlide ---
void BehaviorSlide::Initialize(Player* player) {
    timer_ = 0.0f;
    slideDir_ = player->GetMoveDirection();
    player->SetInvincible(true); // スライディング中は無敵判定
}

void BehaviorSlide::Update(Player* player) {
    // 時間経過とともに滑らかに減速（終盤も余速を維持して自然な停止に）
    float progress = timer_ / kSlideDuration;
    float speedFactor = (1.0f - progress * 0.65f);
    player->Move(-(float(slideDir_) * kSlideInitialSpeed * speedFactor * player->GetDeltaTime()));

    player->RayCastUpdate();
    player->UpdateGravity();
    player->UpdateRailPath();

    timer_ += player->GetDeltaTime();
    if (timer_ >= kSlideDuration) {
        auto state = player->GetState();
        if (state && state->GetFactory()) {
            state->ChangeBehavior(player, state->GetFactory()->CreateBehavior(BehaviorType::Root));
        }
    }
}

void BehaviorSlide::Finalize(Player* player) {
    player->SetInvincible(false);
    player->TriggerInvincibility(0.15f); // 終了後わずかな無敵余韻
}

void BehaviorSlide::HandleInput(Player* player, ICommand* command) {
    // スライディング中にジャンプ入力でキャンセル可能（スライディングジャンプ）
    if (dynamic_cast<JumpCommand*>(command)) {
        auto state = player->GetState();
        if (state && state->GetFactory()) {
            state->ChangeBehavior(player, state->GetFactory()->CreateBehavior(BehaviorType::Jump));
        }
    }
}

// --- BehaviorHover ---
void BehaviorHover::Initialize(Player* player) {
    timer_ = 0.0f;
    isHovering_ = true;

    // 初速度のYをリセットして、落下慣性を止めてホバー状態に突入
    Vector3 v = player->GetVelocity();
    v.y = 1.0f; // 少しフワッと浮く
    player->SetVelocity(v);
}

void BehaviorHover::Update(Player* player) {
    auto state = player->GetState();

    // 着地したら即座にRootに戻る
    if (player->IsGround()) {
        if (state && state->GetFactory()) {
            state->ChangeBehavior(player, state->GetFactory()->CreateBehavior(BehaviorType::Root));
        }
        return;
    }

    float dt = player->GetDeltaTime();
    timer_ += dt;

    if (isHovering_ && timer_ < kMaxHoverTime) {
        // 重力を大幅に抑え、微小降下で滑空する
        Vector3 v = player->GetVelocity();
        v.y = kHoverFallSpeed;
        player->SetVelocity(v);
    } else {
        // ホバー持続終了後は通常の重力落下
        player->UpdateGravity();
    }

    player->RayCastUpdate();
    player->UpdateRailPath();

    // ホバー時間終了時は通常のジャンプ落下へ遷移
    if (timer_ >= kMaxHoverTime) {
        if (state && state->GetFactory()) {
            state->ChangeBehavior(player, state->GetFactory()->CreateBehavior(BehaviorType::Jump));
        }
    }
}

void BehaviorHover::Finalize(Player* player) {}

void BehaviorHover::HandleInput(Player* player, ICommand* command) {
    auto state = player->GetState();
    if (!state) return;

    // 空中での左右移動は高速滑空
    if (auto moveCmd = dynamic_cast<MoveCommand*>(command)) {
        if (auto moveAction = state->GetMoveAction()) {
            static_cast<NormalMoveAction*>(moveAction)->SetSpeed(moveCmd->GetSpeed());
            moveAction->Execute(player);
        }
    }

    auto factory = state->GetFactory();
    if (!factory) return;

    if (dynamic_cast<AttackCommand*>(command)) {
        state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Attack));
    }
    if (dynamic_cast<PreShootCommand*>(command)) {
        if (dynamic_cast<IStateRideOn*>(state)) {
            state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Aim));
        }
    }
}

// --- BehaviorGuard ---
void BehaviorGuard::Initialize(Player* player) {
    guardTimer_ = 0.0f;
    isBashing_ = false;
    bashTimer_ = 0.0f;
    bashDir_ = player->GetMoveDirection();
    player->SetInvincible(true); // ガード中は完全無敵
}

void BehaviorGuard::Update(Player* player) {
    float dt = player->GetDeltaTime();
    auto state = player->GetState();

    if (isBashing_) {
        // シールドバッシュ突進中
        bashTimer_ += dt;
        float progress = bashTimer_ / kBashDuration;
        float speedFactor = (1.0f - progress * 0.5f);
        player->Move(float(bashDir_) * kBashSpeed * speedFactor * dt);

        if (bashTimer_ >= kBashDuration) {
            // バッシュ終了
            isBashing_ = false;
            player->SetAttackHitboxActive(false);
            if (state && state->GetFactory()) {
                state->ChangeBehavior(player, state->GetFactory()->CreateBehavior(BehaviorType::Root));
            }
            return;
        }
    } else {
        // ガード構え中（停止して完全防御）
        guardTimer_ += dt;
        if (guardTimer_ >= kMaxGuardTime) {
            if (state && state->GetFactory()) {
                state->ChangeBehavior(player, state->GetFactory()->CreateBehavior(BehaviorType::Root));
            }
            return;
        }
    }

    player->RayCastUpdate();
    player->UpdateGravity();
    player->UpdateRailPath();
}

void BehaviorGuard::Finalize(Player* player) {
    player->SetInvincible(false);
    player->SetAttackHitboxActive(false);
    player->TriggerInvincibility(0.2f); // ガード解除後わずかな無敵余韻
}

void BehaviorGuard::HandleInput(Player* player, ICommand* command) {
    auto state = player->GetState();
    if (!state) return;

    // ガード中に攻撃ボタンでシールドバッシュ発動！
    if (dynamic_cast<AttackCommand*>(command)) {
        if (!isBashing_) {
            isBashing_ = true;
            bashTimer_ = 0.0f;
            bashDir_ = player->GetMoveDirection();
            player->SetAttackHitboxActive(true); // バッシュ攻撃判定ON
            player->PlayHitSE();
        }
        return;
    }

    // ジャンプキーでガード解除＆ジャンプへ
    if (dynamic_cast<JumpCommand*>(command)) {
        if (auto factory = state->GetFactory()) {
            state->ChangeBehavior(player, factory->CreateBehavior(BehaviorType::Jump));
        }
    }
}