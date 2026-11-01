#include "PlayerState.h"
#include "Player.h"

void IPlayerState::Initialize(Player* player) {
    if (factory_) {
        // Factory からそれぞれの Action を組み立て
        moveAction_ = factory_->CreateMoveAction();
        jumpAction_ = factory_->CreateJumpAction();
        attackAction_ = factory_->CreateAttackAction();
        shootAction_ = factory_->CreateShootAction();

        // 初期 Behavior を Factory から作成してセット
        ChangeBehavior(player, factory_->CreateBehavior(BehaviorType::Root));
    }
}

void IPlayerState::Update(Player* player) {
    if (currentBehavior_) currentBehavior_->Update(player);
}

void IPlayerState::Finalize(Player* player) {
    if (currentBehavior_) {
        currentBehavior_->Finalize(player);
        currentBehavior_.reset();
    }
}

void IPlayerState::HandleInput(Player* player, ICommand* command) {
    if (currentBehavior_) {
        currentBehavior_->HandleInput(player, command);
    }
}

void IPlayerState::ChangeBehavior(Player* player, std::unique_ptr<IPlayerBehavior> next) {
    if (!factory_) return;
    
    if (currentBehavior_) {
        currentBehavior_->Finalize(player);
    }
    currentBehavior_ = std::move(next);
    if (currentBehavior_) {
        currentBehavior_->Initialize(player);
    }
}