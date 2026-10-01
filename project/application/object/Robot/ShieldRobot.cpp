#include "ShieldRobot.h"
#include "IPlayerFactory.h"
#include "Player.h"

ShieldRobot::ShieldRobot() {}
ShieldRobot::~ShieldRobot() = default;

std::shared_ptr<IPlayerFactory> ShieldRobot::CreatePlayerFactory() {
    return std::make_shared<ShieldPlayerFactory>();
}
