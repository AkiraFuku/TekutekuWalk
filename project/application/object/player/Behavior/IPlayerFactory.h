#pragma once
#include <memory>

class IPlayerState;
class IPlayerAction;
class IPlayerBehavior;

enum class PlayerFormType {
    Normal,
    RideOnTest,
    Bound,
    Hover,
    Shield,
};

enum class BehaviorType {
    Root,
    Jump,
    Attack,
    Aim,
    Slide,
    Hover,
    Guard,
};

// 抽象ファクトリー
class IPlayerFactory : public std::enable_shared_from_this<IPlayerFactory> {
public:
    virtual ~IPlayerFactory() = default;

    virtual std::unique_ptr<IPlayerState> CreateState() = 0;
    virtual std::unique_ptr<IPlayerAction> CreateMoveAction() = 0;
    virtual std::unique_ptr<IPlayerAction> CreateJumpAction() = 0;
    virtual std::unique_ptr<IPlayerAction> CreateAttackAction() = 0;
    virtual std::unique_ptr<IPlayerAction> CreateShootAction();


    virtual std::unique_ptr<IPlayerBehavior> CreateBehavior(BehaviorType type) = 0;
};




// --- NormalPlayerFactory ---
class NormalPlayerFactory : public IPlayerFactory {
public:
    std::unique_ptr<IPlayerState> CreateState() override;
    std::unique_ptr<IPlayerAction> CreateMoveAction() override;
    std::unique_ptr<IPlayerAction> CreateJumpAction() override;
    std::unique_ptr<IPlayerAction> CreateAttackAction() override;
    std::unique_ptr<IPlayerBehavior> CreateBehavior(BehaviorType type) override;
};

// --- RideOnPlayerFactory ---
class RideOnPlayerFactory : public IPlayerFactory {
public:
   virtual std::unique_ptr<IPlayerState> CreateState() override;
   virtual std::unique_ptr<IPlayerAction> CreateMoveAction() override;
   virtual std::unique_ptr<IPlayerAction> CreateJumpAction() override;
   virtual std::unique_ptr<IPlayerAction> CreateAttackAction() override;
   virtual std::unique_ptr<IPlayerAction> CreateShootAction() override;
   virtual std::unique_ptr<IPlayerBehavior> CreateBehavior(BehaviorType type) override;
};

// --- BoundPlayerFactory ---
class BoundPlayerFactory : public RideOnPlayerFactory {
public:
    std::unique_ptr<IPlayerState> CreateState() override;
    std::unique_ptr<IPlayerAction> CreateMoveAction() override;
    std::unique_ptr<IPlayerBehavior> CreateBehavior(BehaviorType type) override;
};

// --- HoverPlayerFactory ---
class HoverPlayerFactory : public RideOnPlayerFactory {
public:
    std::unique_ptr<IPlayerState> CreateState() override;
    std::unique_ptr<IPlayerAction> CreateMoveAction() override;
    std::unique_ptr<IPlayerBehavior> CreateBehavior(BehaviorType type) override;
};

// --- ShieldPlayerFactory ---
class ShieldPlayerFactory : public RideOnPlayerFactory {
public:
    std::unique_ptr<IPlayerState> CreateState() override;
    std::unique_ptr<IPlayerAction> CreateMoveAction() override;
    std::unique_ptr<IPlayerBehavior> CreateBehavior(BehaviorType type) override;
};

class PlayerStateFactory {
public:
    static std::unique_ptr<IPlayerState> CreateState(PlayerFormType type);
};
