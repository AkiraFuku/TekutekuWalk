#include "InputHandler.h"
#include "Input.h"
#include "Vector2.h"
std::vector<std::unique_ptr<ICommand>> InputHandler::HandleInput()
{
    std::vector<std::unique_ptr<ICommand>> commands;
    Input* input = Input::GetInstance();
    // 0. ダッシュ入力の処理 (Shiftキー、RB/LBボタン、Lスティック押し込み)
    if (input->PushedKeyDown(DIK_LSHIFT) || input->TriggerKeyDown(DIK_LSHIFT) ||
        input->PushedKeyDown(DIK_RSHIFT) || input->TriggerKeyDown(DIK_RSHIFT) ||
        input->PushPadDown(0, XINPUT_GAMEPAD_RIGHT_SHOULDER) ||
        input->PushPadDown(0, XINPUT_GAMEPAD_LEFT_SHOULDER) ||
        input->PushPadDown(0, XINPUT_GAMEPAD_LEFT_THUMB))
    {
        commands.push_back(std::make_unique<DashCommand>());
    }

    // 現在の横移動入力方向を事前取得（ダッシュトリガーとの同時押し判定用）
    int inputMoveDir = 0;
    XINPUT_STATE preState;
    if (input->GetJoyStick(0, preState)) {
        float rawX = (float)preState.Gamepad.sThumbLX / 32767.0f;
        if (rawX < -0.2f) {
            inputMoveDir = -1;
        } else if (rawX > 0.2f) {
            inputMoveDir = 1;
        }
    }
    if (input->PushedKeyDown(DIK_A) || input->PushedKeyDown(DIK_LEFT)) {
        inputMoveDir = -1;
    } else if (input->PushedKeyDown(DIK_D) || input->PushedKeyDown(DIK_RIGHT)) {
        inputMoveDir = 1;
    }

    // 高速ダッシュ（旧ダッシュアタックの攻撃判定なし版）入力の処理 (方向キー不要、Shift/RB/LB/Lスティック押し込みトリガー)
    if (input->TriggerKeyDown(DIK_LSHIFT) || input->TriggerKeyDown(DIK_RSHIFT) ||
        input->TriggerPadDown(0, XINPUT_GAMEPAD_RIGHT_SHOULDER) ||
        input->TriggerPadDown(0, XINPUT_GAMEPAD_LEFT_SHOULDER) ||
        input->TriggerPadDown(0, XINPUT_GAMEPAD_LEFT_THUMB))
    {
        commands.push_back(std::make_unique<SlideCommand>(inputMoveDir));
    }

    // 1. スティック移動の処理
    XINPUT_STATE state;
    if (input->GetJoyStick(0, state) || (input->PushedKeyDown(DIK_A) || input->PushedKeyDown(DIK_D) ||
        input->PushedKeyDown(DIK_LEFT) || input->PushedKeyDown(DIK_RIGHT)))
    {
        if (input->GetJoyStick(0, state)) {
            float rawX = (float)state.Gamepad.sThumbLX / 32767.0f;
            if (std::abs(rawX) > 0.2f) {
                commands.push_back(std::make_unique<MoveCommand>(rawX));
            }
        }
        //　キー入力の例（WASDで移動、スペースでジャンプなど）もここに追加可
        if (input->PushedKeyDown(DIK_A) || input->PushedKeyDown(DIK_D) ||
            input->PushedKeyDown(DIK_LEFT) || input->PushedKeyDown(DIK_RIGHT))
        {
            if (input->PushedKeyDown(DIK_A) || input->PushedKeyDown(DIK_LEFT))
            {
                commands.push_back(std::make_unique<MoveCommand>(-1.0f)); // 左移動
            }
            if (input->PushedKeyDown(DIK_D) || input->PushedKeyDown(DIK_RIGHT))
            {
                commands.push_back(std::make_unique<MoveCommand>(1.0f)); // 右移動
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

    // 2.5 ジャンプ長押し（ホバージャンプ判定用）
    if (input->PushPadDown(0, XINPUT_GAMEPAD_A) || input->PushedKeyDown(DIK_SPACE))
    {
        commands.push_back(std::make_unique<JumpHoldCommand>());
    }

    // 近接攻撃コマンド (マウス左クリック または ゲームパッド Bボタン)
    if (input->TriggerMouseDown(0) || input->TriggerPadDown(0, XINPUT_GAMEPAD_B)) {
        commands.push_back(std::make_unique<AttackCommand>());
    }

    // 連射コマンド (Zキー / マウス右クリック / ゲームパッド Xボタン 押しっぱなしで連射)
    if (input->PushedKeyDown(DIK_Z) || input->PushedKeyDown(DIK_J) || input->PushedKeyDown(DIK_RETURN) || input->PushMouseDown(1) || input->PushPadDown(0, XINPUT_GAMEPAD_X)) {
        commands.push_back(std::make_unique<ShootCommand>());
    }

    return commands;
}
