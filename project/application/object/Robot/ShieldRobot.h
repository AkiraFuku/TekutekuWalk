#pragma once
#include "Robot.h"
#include <memory>
#include <string>
#include "PlayerState.h"
#include "PlayerBehavior.h"

class Player;
class IStateRideOn;

/// <summary>
/// シールドエネミー用ロボットクラス
/// プレイヤーが乗っ取った際に ShieldPlayerFactory を提供
/// </summary>
class ShieldRobot : public Robot {
public:
    ShieldRobot();
    ~ShieldRobot() override;

    std::shared_ptr<IPlayerFactory> CreatePlayerFactory() override;
    std::string GetName() const override {
        return "ShieldRobot";
    }
};
