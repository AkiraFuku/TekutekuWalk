#pragma once
#include "BehaviorState.h"

class Player;
class ICommand;

class IPlayerBehavior : public IBehavior {
public:
    virtual void Initialize(Player* player) = 0;
    virtual void Update(Player* player) = 0;
    virtual void Finalize(Player* player) = 0;
    virtual void HandleInput(Player* player, ICommand* command) = 0;
    virtual bool IsAttacking() const { return false; }
};

class BehaviorRoot : public IPlayerBehavior {
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
    void Finalize(Player* player) override;
    void HandleInput(Player* player, ICommand* command) override;
    const char* GetName() const override { return "Root"; }
};

// 近接格闘ビヘイビア（構え → 踏み込み打撃 → 硬直）
class BehaviorAttack : public IPlayerBehavior {
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
    void Finalize(Player* player) override;
    void HandleInput(Player* player, ICommand* command) override;
    const char* GetName() const override { return "Attack"; }
    bool IsAttacking() const override { return isStrikePhase_; }

private:
    float timer_ = 0.0f;
    int attackDir_ = 1;
    bool isStrikePhase_ = false;
};

class BehaviorJump : public IPlayerBehavior {
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
    void Finalize(Player* player) override;
    void HandleInput(Player* player, ICommand* command) override;
    const char* GetName() const override { return "Jump"; }
};

// スライディング・急制動ステップビヘイビア
class BehaviorSlide : public IPlayerBehavior {
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
    void Finalize(Player* player) override;
    void HandleInput(Player* player, ICommand* command) override;
    const char* GetName() const override { return "Slide"; }

private:
    float timer_ = 0.0f;
    int slideDir_ = 1;
};