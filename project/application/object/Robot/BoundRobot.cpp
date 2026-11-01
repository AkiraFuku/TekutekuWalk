#include "BoundRobot.h"
#include "Player.h"

BoundRobot::BoundRobot() {}
BoundRobot::~BoundRobot() = default;

std::shared_ptr<IPlayerFactory> BoundRobot::CreatePlayerFactory() {
    return nullptr;
}
