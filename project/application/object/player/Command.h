// Command.h
#pragma once
#include "Vector2.h"

enum class CommandType {
    Move,
    Jump,
    Attack,
    Shoot,
    Dash,
    Slide,
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

class DashCommand : public ICommand {
public:
    CommandType GetType() const override { return CommandType::Dash; }
};

class SlideCommand : public ICommand {
public:
    CommandType GetType() const override { return CommandType::Slide; }
};