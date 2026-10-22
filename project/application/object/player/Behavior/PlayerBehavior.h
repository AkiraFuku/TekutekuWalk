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

class BehaviorAttack : public IPlayerBehavior {
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
    void Finalize(Player* player) override;
    void HandleInput(Player* player, ICommand* command) override;
    const char* GetName() const override { return "Attack"; }
    bool IsAttacking() const override { return true; }

private:
    float timer_ = 0.0f;
};

class BehaviorJump : public IPlayerBehavior {
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
    void Finalize(Player* player) override;
    void HandleInput(Player* player, ICommand* command) override;
    const char* GetName() const override { return "Jump"; }
};

class BehaviorAim : public IPlayerBehavior {
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
    void Finalize(Player* player) override;
    void HandleInput(Player* player, ICommand* command) override;
    const char* GetName() const override { return "Aim"; }

private:
    float aimX_ = 1.0f;
    float aimY_ = 0.0f;
    const float kAimFallSpeed = 0.02f;
};

// 常に跳ね続けるビヘイビア
class BehaviorBound : public IPlayerBehavior {
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
    void Finalize(Player* player) override;
    void HandleInput(Player* player, ICommand* command) override;
    const char* GetName() const override { return "Bound"; }
};

// スライディング・回避ステップビヘイビア
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

// 空中ホバージャンプビヘイビア
class BehaviorHover : public IPlayerBehavior {
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
    void Finalize(Player* player) override;
    void HandleInput(Player* player, ICommand* command) override;
    const char* GetName() const override { return "Hover"; }

private:
    float timer_ = 0.0f;
    bool isHovering_ = true;
};

// シールドガード＆バッシュビヘイビア
class BehaviorGuard : public IPlayerBehavior {
public:
    void Initialize(Player* player) override;
    void Update(Player* player) override;
    void Finalize(Player* player) override;
    void HandleInput(Player* player, ICommand* command) override;
    const char* GetName() const override { return "Guard"; }

private:
    float guardTimer_ = 0.0f;
    bool isBashing_ = false;
    float bashTimer_ = 0.0f;
    int bashDir_ = 1;
};