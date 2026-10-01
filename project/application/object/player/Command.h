// Command.h
#pragma once
#include "Vector2.h"

enum class CommandType {
    Move,
    Jump,
    Attack,
    Shoot,
    PreShoot,
    Dash,
    Slide,
    Guard,
    Aim
};

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual CommandType GetType() const = 0;
};

class MoveCommand : public ICommand {
public:
    MoveCommand(float speed) : speed_(speed) {}
    CommandType GetType() const override { return CommandType::Move; }
    float GetSpeed() const { return speed_; }
private:
    float speed_;
};

class JumpCommand : public ICommand {
public:
    CommandType GetType() const override { return CommandType::Jump; }
};

class AttackCommand : public ICommand {
public:
    CommandType GetType() const override { return CommandType::Attack; }
};

class ShootCommand : public ICommand {
public:
    CommandType GetType() const override { return CommandType::Shoot; }
};

class PreShootCommand : public ICommand {
public:
    CommandType GetType() const override { return CommandType::PreShoot; }
};

class DashCommand : public ICommand {
public:
    CommandType GetType() const override { return CommandType::Dash; }
};

class SlideCommand : public ICommand {
public:
    CommandType GetType() const override { return CommandType::Slide; }
};

class GuardCommand : public ICommand {
public:
    CommandType GetType() const override { return CommandType::Guard; }
};

class AimCommand : public ICommand {
public:
    // x, y は -1.0f ~ 1.0f の範囲
    AimCommand(Vector2 direction) : direction_(direction) {}
    CommandType GetType() const override { return CommandType::Aim; }
    float GetX() const { return direction_.x; }
    float GetY() const { return direction_.y; }
    Vector2 GetDirection() const {
        return direction_;
    }   
private:
   Vector2 direction_;
};