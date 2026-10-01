#include "HoverRobot.h"
#include "IPlayerFactory.h"
#include "Player.h"

HoverRobot::HoverRobot() {}
HoverRobot::~HoverRobot() = default;

std::shared_ptr<IPlayerFactory> HoverRobot::CreatePlayerFactory() {
    return std::make_shared<HoverPlayerFactory>();
}
