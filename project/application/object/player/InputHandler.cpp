#include "InputHandler.h"
#include "Input.h"
#include "Vector2.h"
std::vector<std::unique_ptr<ICommand>> InputHandler::HandleInput()
{
    std::vector<std::unique_ptr<ICommand>> commands;
    Input* input = Input::GetInstance();
    // 0. ダッシュ入力の処理 (Shiftキー、RB/LBボタン、Lスティック押し込み)
    if (input->PushedKeyDown(DIK_LSHIFT) || input->PushedKeyDown(DIK_RSHIFT) ||
        input->PushPadDown(0, XINPUT_GAMEPAD_RIGHT_SHOULDER) ||
        input->PushPadDown(0, XINPUT_GAMEPAD_LEFT_SHOULDER) ||
        input->PushPadDown(0, XINPUT_GAMEPAD_LEFT_THUMB))
    {
        commands.push_back(std::make_unique<DashCommand>());
    }

    // スライディング入力の処理 (下入力 + ダッシュボタン)
    bool isDownPressed = input->PushedKeyDown(DIK_S);
    XINPUT_STATE padState;
    if (input->GetJoyStick(0, padState)) {
        float stickY = (float)padState.Gamepad.sThumbLY / 32767.0f;
        if (stickY < -0.4f) {
            isDownPressed = true;
        }
    }
    if (isDownPressed && (input->TriggerKeyDown(DIK_LSHIFT) || input->TriggerKeyDown(DIK_RSHIFT) ||
        input->TriggerPadDown(0, XINPUT_GAMEPAD_RIGHT_SHOULDER) ||
        input->TriggerPadDown(0, XINPUT_GAMEPAD_LEFT_SHOULDER) ||
        input->TriggerPadDown(0, XINPUT_GAMEPAD_LEFT_THUMB)))
    {
        commands.push_back(std::make_unique<SlideCommand>());
    }

    // 1. スティック移動の処理
    XINPUT_STATE state;
    if (input->GetJoyStick(0, state) || (input->PushedKeyDown(DIK_A) || input->PushedKeyDown(DIK_D)))
    {
        if (input->GetJoyStick(0, state)) {
            float rawX = (float)state.Gamepad.sThumbLX / 32767.0f;
            if (std::abs(rawX) > 0.2f) {
                commands.push_back(std::make_unique<MoveCommand>(rawX));
            }
        }
        //　キー入力の例（WASDで移動、スペースでジャンプなど）もここに追加可
        if (input->PushedKeyDown(DIK_A) || input->PushedKeyDown(DIK_D))
        {
            if (input->PushedKeyDown(DIK_A))
            {
                commands.push_back(std::make_unique<MoveCommand>(-1.0f)); // 左移動
            }
            if (input->PushedKeyDown(DIK_D))
            {
                commands.push_back(std::make_unique<MoveCommand>(1.0f)); // 右移動
            }
        }
    }
    //狙いを定める
    // 1. スティック入力の処理
    if (input->GetJoyStick(0, state) || (input->PushedKeyDown(DIK_A) || input->PushedKeyDown(DIK_D) || input->PushedKeyDown(DIK_W) || input->PushedKeyDown(DIK_S)))
    {
        if (input->GetJoyStick(0, state)) {
            float rawX = (float)state.Gamepad.sThumbLX / 32767.0f;
            float rawY = (float)state.Gamepad.sThumbLY / 32767.0f;
            if (std::abs(rawX) > 0.2f || std::abs(rawY) > 0.2f) {
                commands.push_back(std::make_unique<AimCommand>(Vector2{ rawX, rawY }));
            }
        }
        if (input->PushedKeyDown(DIK_A) || input->PushedKeyDown(DIK_D) || input->PushedKeyDown(DIK_W) || input->PushedKeyDown(DIK_S))
        {
            if (input->PushedKeyDown(DIK_A))
            {
                commands.push_back(std::make_unique<AimCommand>(Vector2{ -1.0f, 0.0f })); // 左入力
            } else if (input->PushedKeyDown(DIK_D))
            {
                commands.push_back(std::make_unique<AimCommand>(Vector2{ 1.0f, 0.0f })); // 右入力
            } else if (input->PushedKeyDown(DIK_W))
            {
                commands.push_back(std::make_unique<AimCommand>(Vector2{ 0.0f, 1.0f })); // 上入力
            } else if (input->PushedKeyDown(DIK_S))
            {
                commands.push_back(std::make_unique<AimCommand>(Vector2{ 0.0f, -1.0f })); // 下入力
            }
        }
    }

    if (input->TriggerPadDown(0, XINPUT_GAMEPAD_A) || input->TriggerKeyDown(DIK_SPACE))
    {
        // 2. ジャンプの処理 (TriggerPadDownを使用)
        if (input->TriggerPadDown(0, XINPUT_GAMEPAD_A)) {
            commands.push_back(std::make_unique<JumpCommand>());
        }
        if (input->TriggerKeyDown(DIK_SPACE))
        {
            commands.push_back(std::make_unique<JumpCommand>());
        }
    }

    if (input->TriggerMouseDown(0) || input->TriggerPadDown(0, XINPUT_GAMEPAD_B)) { // 例としてスペース
        commands.push_back(std::make_unique<AttackCommand>());
    }

    // ガードコマンド (Cキー または ゲームパッド Yボタン)
    if (input->PushedKeyDown(DIK_C) || input->PushPadDown(0, XINPUT_GAMEPAD_Y)) {
        commands.push_back(std::make_unique<GuardCommand>());
    }

    // 射出コマンド (Zキーまたはゲームパッドの特定ボタン)
    if (input->PushedKeyDown(DIK_Z)|| input->PushMouseDown(1)|| input->PushPadDown(0, XINPUT_GAMEPAD_X)) {
        commands.push_back(std::make_unique<PreShootCommand>());
    }
    if (input->TriggerKeyUp(DIK_Z) || input->TriggerMouseUP(1)|| input->TriggerPadUp(0, XINPUT_GAMEPAD_X)) {
        commands.push_back(std::make_unique<ShootCommand>());
    }

    return commands;
}
