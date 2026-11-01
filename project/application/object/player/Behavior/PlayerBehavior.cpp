#include "PlayerBehavior.h"
#include "Command.h"
#include "IPlayerFactory.h"
#include "Player.h"
#include "PlayerAction.h"
#include "PlayerState.h"

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
    player->SetDashing(true);
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
    state->ChangeBehavior(player,
                          factory->CreateBehavior(BehaviorType::Attack));
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
  player->TriggerInvincibility(
      player->GetConfig().attackAction.invincibilityAfter);
}

void BehaviorAttack::HandleInput(Player *player, ICommand *command) {
  (void)player;
  (void)command;
}

// --- BehaviorJump ---
void BehaviorJump::Initialize(Player *player) {
  // Jump への遷移は Player::TryExecuteBufferedJump() 内でジャンプ初速設定後に行われるため、
  // Initialize での再度の Jump() 呼び出し（二重遷移・先行入力タイマー再チャージ）は行わない
  (void)player;
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
  }
}

void BehaviorJump::Finalize(Player *player) {}

void BehaviorJump::HandleInput(Player *player, ICommand *command) {
  if (!command)
    return;
  auto state = player->GetState();
  if (!state)
    return;

  CommandType type = command->GetType();

  if (type == CommandType::Dash) {
    player->SetDashing(true);
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

  // 空中滞空中でもジャンプボタンの先行入力を受け付ける
  if (type == CommandType::Jump) {
    player->Jump();
    return;
  }

  if (type == CommandType::Attack) {
    state->ChangeBehavior(player,
                          factory->CreateBehavior(BehaviorType::Attack));
    return;
  }
}

// --- BehaviorSlide ---
void BehaviorSlide::Initialize(Player *player) {
  timer_ = 0.0f;
  slideDir_ = player->GetMoveDirection();
  player->SetInvincible(true); // スライディング中は無敵判定
}

void BehaviorSlide::Update(Player *player) {
  const auto &config = player->GetConfig().slide;
  // 時間経過とともに滑らかに減速（終盤も余速を維持して自然な停止に）
  float progress = (config.duration > 0.0f) ? (timer_ / config.duration) : 1.0f;
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
  player->TriggerInvincibility(
      player->GetConfig().slide.invincibilityAfter); // 終了後わずかな無敵余韻
}

void BehaviorSlide::HandleInput(Player *player, ICommand *command) {
  if (!command)
    return;
  // スライディング中にジャンプ入力でキャンセル可能（スライディングジャンプ）
  if (command->GetType() == CommandType::Jump) {
    player->Jump();
  }
}
