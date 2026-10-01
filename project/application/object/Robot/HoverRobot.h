#pragma once
#include "Robot.h"
#include <memory>
#include <string>
#include "PlayerState.h"
#include "PlayerBehavior.h"

class Player;
class IStateRideOn;

/// <summary>
/// ホバーエネミー用ロボットクラス
/// プレイヤーが乗っ取った際に HoverPlayerFactory を提供
/// </summary>
class HoverRobot : public Robot {
public:
    HoverRobot();
    ~HoverRobot() override;

    std::shared_ptr<IPlayerFactory> CreatePlayerFactory() override;
    std::string GetName() const override {
        return "HoverRobot";
    }
};
