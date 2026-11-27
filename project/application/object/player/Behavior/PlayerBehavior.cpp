#include "PlayerBehavior.h"
#include "Command.h"
#include "IPlayerFactory.h"
#include "Player.h"
#include "PlayerAction.h"
#include "PlayerState.h"
#include "GpuParticleManager.h"
#include <cmath>

// --- BehaviorRoot ---
void BehaviorRoot::Initialize(Player *player) {}

void BehaviorRoot::Update(Player *player) {
  (void)player;
  // レイキャスト・重力・レールトランスフォーム同期は Player::Update() で一元実行
}

void BehaviorRoot::Finalize(Player *player) {}

void BehaviorRoot::HandleInput(Player *player, ICommand *command) {
  if (!command)
    return;
  auto state = player->GetState();
  if (!state)
    return;

  CommandType type = command->GetType();

  if (type == CommandType::Dash) {
    if (!player->IsShootingStance()) {
      if (player->IsLandingStaggered()) {
        player->CancelLandingStagger();
      }
      player->SetDashing(true);
    }
    return;
  }

  if (type == CommandType::Move) {
    auto moveCmd = static_cast<MoveCommand *>(command);
    if (auto moveAction = state->GetMoveAction()) {
      static_cast<NormalMoveAction *>(moveAction)
          ->SetSpeed(moveCmd->GetSpeed());
      moveAction->Execute(player);
    }
    return;
  }

  // 移動中・待機中を問わずマシンガン連射が可能
  if (type == CommandType::Shoot) {
    if (auto shootAction = state->GetShootAction()) {
      shootAction->Execute(player);
    }
    return;
  }

  auto factory = state->GetFactory();
  if (!factory)
    return;

  if (type == CommandType::Slide) {
    if (player->IsGround()) {
      if (player->IsLandingStaggered()) {
        player->CancelLandingStagger();
      }
      state->ChangeBehavior(player,
                            factory->CreateBehavior(BehaviorType::Slide));
      return;
    }
  }

  if (type == CommandType::Jump) {
    // JumpAction 経由で先行入力チャージ＆TryExecuteBufferedJump を一元実行
    if (auto jumpAction = state->GetJumpAction()) {
      jumpAction->Execute(player);
    } else {
      player->Jump();
    }
    return;
  }

  if (type == CommandType::Attack) {
    BehaviorType nextAttack =
        player->IsDashing() ? BehaviorType::DashAttack : BehaviorType::Attack;
    state->ChangeBehavior(player, factory->CreateBehavior(nextAttack));
    return;
  }
}

// --- BehaviorAttack（ロボット近接格闘：構え → 踏み込みパンチ／パイル → 硬直）
// ---
void BehaviorAttack::Initialize(Player *player) {
  timer_ = 0.0f;
  isStrikePhase_ = false;
  attackDir_ = player->GetMoveDirection();
  player->SetAttackHitboxActive(false);
  player->StartAttackAnimation();

  auto state = player->GetState();
  if (state) {
    auto attackAction = state->GetAttackAction_();
    if (attackAction) {
      attackAction->Execute(player);
    }
  }
}

void BehaviorAttack::Update(Player *player) {
  auto state = player->GetState();
  const auto &config = player->GetConfig().attackAction;
  float dt = player->GetDeltaTime();
  timer_ += dt;

  float windupEnd = config.windupDuration;
  float strikeEnd = config.windupDuration + config.rushDuration;
  float totalDuration = strikeEnd + config.recoveryDuration;

  if (timer_ < windupEnd) {
    // Phase 1: 予備動作（装填・溜め）— 踏ん張って構える
    isStrikePhase_ = false;
    player->SetAttackHitboxActive(false);
  } else if (timer_ < strikeEnd) {
    // Phase 2: 踏み込み打撃（アームパンチ／パイルバンカー突き出し）
    if (!isStrikePhase_) {
      isStrikePhase_ = true;
      player->TriggerSquashStretch(0.15f, config.rushDuration);

      Vector3 forward = player->GetDirection() * (attackDir_ < 0 ? -1.0f : 1.0f);
      float len = std::sqrt(forward.x * forward.x + forward.y * forward.y + forward.z * forward.z);
      if (len > 1e-4f) {
        forward = forward / len;
      } else {
        forward = { 0.0f, 0.0f, 1.0f };
      }
      const auto &colCfg = player->GetConfig().collider;
      Vector3 emitPos = player->GetWorldPosition() + forward * (colCfg.attackOffsetForward * 0.45f);
      emitPos.y += colCfg.attackOffsetY;
      GpuParticleManager::GetInstance()->Emit("PlayerMelee", emitPos, forward);
    }
    // 打撃フェーズ中は常にヒットボックスを有効に維持する
    player->SetAttackHitboxActive(true);

    // ダッシュ体当たりではなく、設定倍率に応じたショートステップ移動
    float strikeProgress =
        (timer_ - windupEnd) / (std::max)(config.rushDuration, 0.001f);
    float stepFactor =
        1.0f - strikeProgress * 0.7f; // 踏み込み終わりに向けて減速
    player->Move(static_cast<float>(attackDir_) * config.rushSpeedMultiplier *
                 stepFactor);
  } else {
    // Phase 3: フォロースルー（排莢・硬直）— 攻撃判定を消してその場で止まる
    isStrikePhase_ = false;
    player->SetAttackHitboxActive(false);
  }

  if (timer_ >= totalDuration) {
    if (state) {
      if (auto factory = state->GetFactory()) {
        state->ChangeBehavior(player,
                              factory->CreateBehavior(BehaviorType::Root));
      }
    }
  }
}

void BehaviorAttack::Finalize(Player *player) {
  isStrikePhase_ = false;
  player->SetAttackHitboxActive(false);
  player->StopAttackAnimation();
  player->TriggerInvincibility(
      player->GetConfig().attackAction.invincibilityAfter);
}

void BehaviorAttack::HandleInput(Player *player, ICommand *command) {
  (void)player;
  (void)command;
}

// --- BehaviorDashAttack（ダッシュ派生：高速突進打撃） ---
void BehaviorDashAttack::Initialize(Player *player) {
  timer_ = 0.0f;
  isStrikePhase_ = false;
  attackDir_ = player->GetMoveDirection();
  player->SetDashAttackActive(true);
  player->SetAttackHitboxActive(false);
  player->StartAttackAnimation();

  auto state = player->GetState();
  if (state) {
    if (auto attackAction = state->GetAttackAction_()) {
      attackAction->Execute(player);
    }
  }
}

void BehaviorDashAttack::Update(Player *player) {
  auto state = player->GetState();
  const auto &config = player->GetConfig().dashAttack;
  float dt = player->GetDeltaTime();
  timer_ += dt;

  float windupEnd = config.windupDuration;
  float strikeEnd = windupEnd + config.rushDuration;
  float totalDuration = strikeEnd + config.recoveryDuration;

  if (timer_ < windupEnd) {
    isStrikePhase_ = false;
    player->SetAttackHitboxActive(false);
  } else if (timer_ < strikeEnd) {
    if (!isStrikePhase_) {
      isStrikePhase_ = true;
      player->TriggerSquashStretch(0.18f, config.rushDuration);

      Vector3 forward =
          player->GetDirection() * (attackDir_ < 0 ? -1.0f : 1.0f);
      float len = std::sqrt(forward.x * forward.x + forward.y * forward.y +
                            forward.z * forward.z);
      if (len > 1e-4f) {
        forward = forward / len;
      } else {
        forward = {0.0f, 0.0f, 1.0f};
      }
      const auto &colCfg = player->GetConfig().collider;
      Vector3 emitPos = player->GetWorldPosition() +
                        forward * (colCfg.attackOffsetForward * 0.45f);
      emitPos.y += colCfg.attackOffsetY;
      GpuParticleManager::GetInstance()->Emit("PlayerMelee", emitPos, forward);
    }
    player->SetAttackHitboxActive(true);

    float strikeProgress =
        (timer_ - windupEnd) / (std::max)(config.rushDuration, 0.001f);
    float speedFactor = 1.0f - strikeProgress * 0.5f;
    player->MoveAtSpeed(static_cast<float>(attackDir_) * config.rushSpeed *
                        speedFactor);
  } else {
    isStrikePhase_ = false;
    player->SetAttackHitboxActive(false);
  }

  if (timer_ >= totalDuration) {
    if (state) {
      if (auto factory = state->GetFactory()) {
        BehaviorType next =
            player->IsGround() ? BehaviorType::Root : BehaviorType::Jump;
        state->ChangeBehavior(player, factory->CreateBehavior(next));
      }
    }
  }
}

void BehaviorDashAttack::Finalize(Player *player) {
  isStrikePhase_ = false;
  player->SetDashAttackActive(false);
  player->SetAttackHitboxActive(false);
  player->StopAttackAnimation();
  player->TriggerInvincibility(
      player->GetConfig().dashAttack.invincibilityAfter);
}

void BehaviorDashAttack::HandleInput(Player *player, ICommand *command) {
  (void)player;
  (void)command;
}

// --- BehaviorJump ---
void BehaviorJump::Initialize(Player *player) {
  // Jump への遷移は Player::TryExecuteBufferedJump() 内でジャンプ初速設定後に行われるため、
  // Initialize での再度の Jump() 呼び出し（二重遷移・先行入力タイマー再チャージ）は行わない
  (void)player;
  jumpHoldTimer_ = 0.0f;
  isJumpHeldThisFrame_ = false;
}

void BehaviorJump::Update(Player *player) {
  if (player->IsGround()) {
    // 着地時に先行入力が残っていれば即座に再ジャンプを実行
    if (player->TryExecuteBufferedJump()) {
      return;
    }
    if (auto state = player->GetState()) {
      if (auto factory = state->GetFactory()) {
        state->ChangeBehavior(player,
                              factory->CreateBehavior(BehaviorType::Root));
      }
    }
    return;
  }

  if (isJumpHeldThisFrame_) {
    jumpHoldTimer_ += player->GetDeltaTime();
    const auto &hoverCfg = player->GetConfig().hover;
    if (jumpHoldTimer_ >= hoverCfg.holdTriggerDuration &&
        player->IsHoverConditionMet()) {
      if (auto state = player->GetState()) {
        if (auto factory = state->GetFactory()) {
          isJumpHeldThisFrame_ = false;
          state->ChangeBehavior(player,
                                factory->CreateBehavior(BehaviorType::Hover));
          return;
        }
      }
    }
  } else {
    jumpHoldTimer_ = 0.0f;
  }
  isJumpHeldThisFrame_ = false;
}

void BehaviorJump::Finalize(Player *player) {}

void BehaviorJump::HandleInput(Player *player, ICommand *command) {
  if (!command)
    return;
  auto state = player->GetState();
  if (!state)
    return;

  CommandType type = command->GetType();

  if (type == CommandType::JumpHold) {
    isJumpHeldThisFrame_ = true;
    return;
  }

  if (type == CommandType::Dash) {
    if (!player->IsShootingStance()) {
      if (player->IsLandingStaggered()) {
        player->CancelLandingStagger();
      }
      player->SetDashing(true);
    }
    return;
  }

  if (type == CommandType::Move) {
    auto moveCmd = static_cast<MoveCommand *>(command);
    if (auto moveAction = state->GetMoveAction()) {
      static_cast<NormalMoveAction *>(moveAction)
          ->SetSpeed(moveCmd->GetSpeed());
      moveAction->Execute(player);
    }
    return;
  }

  // 空中でもマシンガン連射が可能
  if (type == CommandType::Shoot) {
    if (auto shootAction = state->GetShootAction()) {
      shootAction->Execute(player);
    }
    return;
  }

  auto factory = state->GetFactory();
  if (!factory)
    return;

  if (type == CommandType::Slide) {
    if (player->IsLandingStaggered()) {
      player->CancelLandingStagger();
    }
    state->ChangeBehavior(player,
                          factory->CreateBehavior(BehaviorType::Slide));
    return;
  }

  // 空中滞空中でもジャンプボタンの先行入力を受け付ける
  if (type == CommandType::Jump) {
    player->Jump();
    return;
  }

  if (type == CommandType::Attack) {
    BehaviorType nextAttack =
        player->IsDashing() ? BehaviorType::DashAttack : BehaviorType::Attack;
    state->ChangeBehavior(player, factory->CreateBehavior(nextAttack));
    return;
  }
}

// --- BehaviorHover（ホバージャンプ：緩やかな上昇＆UIゲージ消費） ---
void BehaviorHover::Initialize(Player *player) {
  isJumpHeldThisFrame_ = true;
  player->SetHovering(true);
  player->SetGravityScale(0.0f);
  Vector3 vel = player->GetVelocity();
  vel.y = player->GetConfig().hover.ascendSpeed;
  player->SetVelocity(vel);
}

void BehaviorHover::Update(Player *player) {
  const float dt = player->GetDeltaTime();
  player->ConsumeHoverEnergy(dt);

  const auto &hoverCfg = player->GetConfig().hover;
  const float gainedHeight = player->GetWorldY() - player->GetJumpStartY();
  const bool reachedMaxHeight = (gainedHeight >= hoverCfg.maxTotalHeight);

  if (!isJumpHeldThisFrame_ || player->GetHoverEnergyRatio() <= 0.0f ||
      reachedMaxHeight || player->IsGround()) {
    if (auto state = player->GetState()) {
      if (auto factory = state->GetFactory()) {
        BehaviorType next =
            player->IsGround() ? BehaviorType::Root : BehaviorType::Jump;
        state->ChangeBehavior(player, factory->CreateBehavior(next));
        return;
      }
    }
  }

  Vector3 vel = player->GetVelocity();
  vel.y = hoverCfg.ascendSpeed;
  player->SetVelocity(vel);
  isJumpHeldThisFrame_ = false;
}

void BehaviorHover::Finalize(Player *player) {
  player->SetHovering(false);
  player->SetGravityScale(1.0f);
  Vector3 vel = player->GetVelocity();
  if (vel.y > 0.0f) {
    vel.y = 0.0f;
    player->SetVelocity(vel);
  }
}

void BehaviorHover::HandleInput(Player *player, ICommand *command) {
  if (!command)
    return;
  auto state = player->GetState();
  if (!state)
    return;

  CommandType type = command->GetType();

  if (type == CommandType::JumpHold) {
    isJumpHeldThisFrame_ = true;
    return;
  }

  if (type == CommandType::Dash) {
    if (!player->IsShootingStance()) {
      if (player->IsLandingStaggered()) {
        player->CancelLandingStagger();
      }
      player->SetDashing(true);
    }
    return;
  }

  if (type == CommandType::Move) {
    auto moveCmd = static_cast<MoveCommand *>(command);
    if (auto moveAction = state->GetMoveAction()) {
      static_cast<NormalMoveAction *>(moveAction)
          ->SetSpeed(moveCmd->GetSpeed());
      moveAction->Execute(player);
    }
    return;
  }

  if (type == CommandType::Shoot) {
    if (auto shootAction = state->GetShootAction()) {
      shootAction->Execute(player);
    }
    return;
  }

  auto factory = state->GetFactory();
  if (!factory)
    return;

  if (type == CommandType::Slide) {
    if (player->IsLandingStaggered()) {
      player->CancelLandingStagger();
    }
    state->ChangeBehavior(player,
                          factory->CreateBehavior(BehaviorType::Slide));
    return;
  }

  if (type == CommandType::Attack) {
    BehaviorType nextAttack =
        player->IsDashing() ? BehaviorType::DashAttack : BehaviorType::Attack;
    state->ChangeBehavior(player, factory->CreateBehavior(nextAttack));
    return;
  }
}

// --- BehaviorSlide ---
void BehaviorSlide::Initialize(Player *player) {
  timer_ = 0.0f;
  slideDir_ = player->GetMoveDirection();
  player->SetInvincible(true); // スライディング中は無敵判定
  player->SetDashStepActive(true);
}

void BehaviorSlide::Update(Player *player) {
  const auto &config = player->GetConfig().slide;
  // 時間経過とともに滑らかに減速（終盤も余速を維持して自然な停止に）
  float progress = (config.duration > 0.0f) ? (timer_ / config.duration) : 1.0f;
  player->SetDashStepProgress(progress);
  float speedFactor = (1.0f - progress * 0.65f);
  // 向き反転バグ（マイナス符号）と deltaTime 二重掛けを解消し、実速度(m/s)で前進
  player->MoveAtSpeed(float(slideDir_) * config.initialSpeed * speedFactor);

  timer_ += player->GetDeltaTime();
  if (timer_ >= config.duration) {
    auto state = player->GetState();
    if (state && state->GetFactory()) {
      state->ChangeBehavior(
          player, state->GetFactory()->CreateBehavior(BehaviorType::Root));
    }
  }
}

void BehaviorSlide::Finalize(Player *player) {
  player->SetInvincible(false);
  player->SetDashStepActive(false);
  player->TriggerInvincibility(
      player->GetConfig().slide.invincibilityAfter); // 終了後わずかな無敵余韻
}

void BehaviorSlide::HandleInput(Player *player, ICommand *command) {
  if (!command)
    return;
  // スライディング中にジャンプ入力でキャンセル可能（スライディングジャンプ）
  if (command->GetType() == CommandType::Jump) {
    player->Jump();
    return;
  }
  // スライディング中に攻撃入力でダッシュ派生攻撃へ
  if (command->GetType() == CommandType::Attack) {
    if (auto state = player->GetState()) {
      if (auto factory = state->GetFactory()) {
        player->SetDashing(true);
        state->ChangeBehavior(
            player, factory->CreateBehavior(BehaviorType::DashAttack));
      }
    }
  }
}
