#pragma once
#include <memory>

class IPlayerState;
class IPlayerAction;
class IPlayerBehavior;

enum class PlayerFormType {
    Normal,
};

enum class BehaviorType {
    Root,
    Jump,
    Attack,
    Slide,
};

// 抽象ファクトリー
class IPlayerFactory : public std::enable_shared_from_this<IPlayerFactory> {
public:
    virtual ~IPlayerFactory() = default;

    virtual std::unique_ptr<IPlayerState> CreateState() = 0;
    virtual std::unique_ptr<IPlayerAction> CreateMoveAction() = 0;
    virtual std::unique_ptr<IPlayerAction> CreateJumpAction() = 0;
    virtual std::unique_ptr<IPlayerAction> CreateAttackAction() = 0;
    virtual std::unique_ptr<IPlayerAction> CreateShootAction() = 0;

    virtual std::unique_ptr<IPlayerBehavior> CreateBehavior(BehaviorType type) = 0;
};

// --- NormalPlayerFactory ---
class NormalPlayerFactory : public IPlayerFactory {
public:
    std::unique_ptr<IPlayerState> CreateState() override;
    std::unique_ptr<IPlayerAction> CreateMoveAction() override;
    std::unique_ptr<IPlayerAction> CreateJumpAction() override;
    std::unique_ptr<IPlayerAction> CreateAttackAction() override;
    std::unique_ptr<IPlayerAction> CreateShootAction() override;
    std::unique_ptr<IPlayerBehavior> CreateBehavior(BehaviorType type) override;
};

class PlayerStateFactory {
public:
    static std::unique_ptr<IPlayerState> CreateState(PlayerFormType type);
};
