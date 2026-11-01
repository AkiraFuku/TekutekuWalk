#include "TestRobot.h"
#include "IPlayerFactory.h"

TestRobot::TestRobot() {}
TestRobot::~TestRobot() = default;

std::shared_ptr<IPlayerFactory> TestRobot::CreatePlayerFactory()
{
    return nullptr;
}