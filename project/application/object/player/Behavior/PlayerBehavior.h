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

private:
    float timer_ = 0.0f;
    const float kAttackDuration = 0.65f; // 0.65秒間持続
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
    const float kSlideDuration = 0.38f;      // スライディング持続時間 (秒)
    const float kSlideInitialSpeed = 22.0f;  // スライディング初速 (m/s)
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
    const float kMaxHoverTime = 2.5f;       // 最大ホバー時間 (秒)
    const float kHoverFallSpeed = -0.5f;    // 緩やかな微小降下 (m/s)
    const float kHoverMoveSpeed = 10.0f;    // ホバー中の水平移動速度 (m/s)
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
    const float kMaxGuardTime = 3.0f;       // ガード最大継続時間 (秒)
    const float kBashDuration = 0.28f;      // バッシュ突進時間 (秒)
    const float kBashSpeed = 24.0f;         // バッシュ突進速度 (m/s)
    int bashDir_ = 1;
};