#pragma once
#include <memory>
#include "PlayerBehavior.h"
#include "PlayerAction.h"
#include "IPlayerFactory.h"

class Player;
class ICommand;

class IPlayerState {
public:
    explicit IPlayerState(std::shared_ptr<IPlayerFactory> factory) 
        : factory_(std::move(factory)) {}
    virtual ~IPlayerState() = default;

    virtual void Initialize(Player* player);
    virtual void Update(Player* player);
    virtual void Finalize(Player* player);
    virtual void HandleInput(Player* player, ICommand* command);
    virtual const char* GetName() const = 0;

    IPlayerFactory* GetFactory() const { return factory_.get(); }

    virtual IPlayerAction* GetMoveAction() { return moveAction_.get(); }
    virtual IPlayerAction* GetJumpAction() { return jumpAction_.get(); }
    virtual IPlayerAction* GetAttackAction_() { return attackAction_.get(); }

    void ChangeBehavior(Player* player, std::unique_ptr<IPlayerBehavior> next);
    IPlayerBehavior* GetBehavior() { return currentBehavior_.get(); }



protected:
    std::shared_ptr<IPlayerFactory> factory_;
    std::unique_ptr<IPlayerBehavior> currentBehavior_ = nullptr;

    std::unique_ptr<IPlayerAction> moveAction_ = nullptr;
    std::unique_ptr<IPlayerAction> jumpAction_ = nullptr;
    std::unique_ptr<IPlayerAction> attackAction_ = nullptr;
};

// --- 通常状態 ---
class StateNormal : public IPlayerState {
public:
    explicit StateNormal(std::shared_ptr<IPlayerFactory> factory) : IPlayerState(std::move(factory)) {}
    const char* GetName() const override { return "Normal"; }
};

// --- 搭乗抽象基底 ---
class IStateRideOn : public IPlayerState {
public:
    explicit IStateRideOn(std::shared_ptr<IPlayerFactory> factory) : IPlayerState(std::move(factory)) {}

    void Initialize(Player* player) override;
    void DoMove(Player* player);
    void DoShoot(Player* player);

    IPlayerAction* GetShootAction() { return shootAction_.get(); }

protected:
    std::unique_ptr<IPlayerAction> shootAction_ = nullptr;
};

// --- ライドオンテスト状態 ---
class StateRideOnTest : public IStateRideOn {
public:
    explicit StateRideOnTest(std::shared_ptr<IPlayerFactory> factory) : IStateRideOn(std::move(factory)) {}
    const char* GetName() const override { return "RideOnTest"; }
};
// --- ライドオンバウンドロボ状態 ---
class StateBound: public IStateRideOn {
public:
    explicit StateBound(std::shared_ptr<IPlayerFactory> factory) : IStateRideOn(std::move(factory)) {}
    const char* GetName() const override { return "Bound"; }
};

// --- ライドオンホバーロボ状態 ---
class StateHover : public IStateRideOn {
public:
    explicit StateHover(std::shared_ptr<IPlayerFactory> factory) : IStateRideOn(std::move(factory)) {}
    const char* GetName() const override { return "Hover"; }
};

// --- ライドオンシールドロボ状態 ---
class StateShield : public IStateRideOn {
public:
    explicit StateShield(std::shared_ptr<IPlayerFactory> factory) : IStateRideOn(std::move(factory)) {}
    const char* GetName() const override { return "Shield"; }
};


// --- 死亡状態 ---
class StateDead : public IPlayerState {
public:
    StateDead() : IPlayerState(nullptr) {}
    void Initialize(Player* player) override {}
    const char* GetName() const override { return "Dead"; }
};