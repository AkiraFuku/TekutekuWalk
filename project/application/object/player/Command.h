// Command.h
#pragma once
#include "Vector2.h"
class ICommand {
public:
    virtual ~ICommand() = default;
};

class MoveCommand : public ICommand {
public:
    MoveCommand(float speed) : speed_(speed) {}
    float GetSpeed() const { return speed_; }
private:
    float speed_;
};

class JumpCommand : public ICommand {};
class AttackCommand : public ICommand {};
class ShootCommand : public ICommand {};
class PreShootCommand : public ICommand {};
class DashCommand : public ICommand {};
class SlideCommand : public ICommand {};
class GuardCommand : public ICommand {};
// Command.h
class AimCommand : public ICommand {
public:
    // x, y は -1.0f ~ 1.0f の範囲
    AimCommand(Vector2 direction) : direction_(direction) {}
    float GetX() const { return direction_.x; }
    float GetY() const { return direction_.y; }
    Vector2 GetDirection() const {
        return direction_;
    }   
private:
   Vector2 direction_;
   
};