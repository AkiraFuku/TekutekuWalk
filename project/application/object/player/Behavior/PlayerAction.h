#pragma once
#include "Vector2.h"
class Player;
class GameScene;

enum PlayerActionType { kJump, kMove, kAttack, kShoot };

// --- 行動ビヘイビア (Action) ---
// 物理計算や移動ロジックを担当
class IPlayerAction {
public:
  virtual ~IPlayerAction() = default;
  virtual void Execute(Player *player) = 0;
};

class NormalMoveAction : public IPlayerAction {
public:
  NormalMoveAction(float speed) : speed_(speed) {}
  void Execute(Player *player) override;
  void SetSpeed(float speed) { speed_ = speed; }

private:
  float speed_;
  PlayerActionType Type = kMove;
};

// ジャンプアクション
class NormalJumpAction : public IPlayerAction {
public:
  void Execute(Player *player) override;

public:
  PlayerActionType Type = kJump;
};

// 近接格闘アクション（アームパンチ／パイルバンカー）
class NormalAttackAction : public IPlayerAction {
public:
  void Execute(Player *player) override;

public:
  PlayerActionType Type = kAttack;
};

// 連射アクション（ヘビィマシンガン連射）
class RapidFireAction : public IPlayerAction {
public:
  void Execute(Player *player) override;

public:
  PlayerActionType Type = kShoot;

private:
  int shotCount_ = 0;
};