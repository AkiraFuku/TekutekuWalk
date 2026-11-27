#include "Input.h"
#include "assert.h"
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#include <memory>
#include "WinApp.h"

void Input::Finalize() {
    Singleton<Input>::Finalize();
}
void Input::Initialize() {

    // DirectInputの初期化
    HRESULT hr;
    Microsoft::WRL::ComPtr<IDirectInput8> directInput = nullptr;
    hr = DirectInput8Create(
        WinApp::GetInstance()->GetInstanceHandle(),
        DIRECTINPUT_VERSION,
        IID_IDirectInput8,
        (void**)&directInput,
        nullptr
    );
    assert(SUCCEEDED(hr));
    // キーボードデバイスの作成
    keyboard = nullptr;
    hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
    assert(SUCCEEDED(hr));
    hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
    assert(SUCCEEDED(hr));
    // キーボードの設定
    hr = keyboard->SetCooperativeLevel(
        WinApp::GetInstance()->GetHwnd(),
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
    );
    assert(SUCCEEDED(hr));
    // マウスデバイスの作成
    mouse = nullptr;
    hr = directInput->CreateDevice(GUID_SysMouse, &mouse, NULL);
    assert(SUCCEEDED(hr));
    hr = mouse->SetDataFormat(&c_dfDIMouse);
    assert(SUCCEEDED(hr));
    hr = mouse->SetCooperativeLevel(
        WinApp::GetInstance()->GetHwnd(),
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY
    );
    assert(SUCCEEDED(hr));

    // 配列の初期化
    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        deadZoneL_[i] = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE; // デフォルト値推奨
        deadZoneR_[i] = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
        ZeroMemory(&state_[i], sizeof(XINPUT_STATE));
        ZeroMemory(&preState_[i], sizeof(XINPUT_STATE));
    }

    if (keyboard) keyboard->GetDeviceState(sizeof(preKey), preKey);
    if (mouse) mouse->GetDeviceState(sizeof(DIMOUSESTATE), &preMouseState);
}
void Input::Update()
{
    // 例:

    std::memcpy(preKey, key, sizeof(key));
    keyboard->Acquire();

    keyboard->GetDeviceState(sizeof(key), key);


    // --- マウス更新  ---
    // 前回の状態を保存
    preMouseState = mouseState;

    // デバイス取得権限の要求
    mouse->Acquire();

    // 現在の状態を取得
    mouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState);

    // --- ゲームパッド更新 ---
    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        // 現在の状態を「過去」に保存
        preState_[i] = state_[i];

        // 最新の状態を取得
        ZeroMemory(&state_[i], sizeof(XINPUT_STATE));
        DWORD result_ = XInputGetState(i, &state_[i]);

        // 接続されていない場合はパケット番号を0にしておくなどの処理
        if (result_ != ERROR_SUCCESS) {
            ZeroMemory(&state_[i], sizeof(XINPUT_STATE));
        }
    }

    // --- 直近使用入力デバイスの判定 ---
    bool hasKeyMouseInput = false;
    for (int k = 0; k < 256; ++k) {
        if (key[k] & 0x80) {
            hasKeyMouseInput = true;
            break;
        }
    }
    if (!hasKeyMouseInput) {
        if ((mouseState.rgbButtons[0] & 0x80) ||
            (mouseState.rgbButtons[1] & 0x80) ||
            (mouseState.rgbButtons[2] & 0x80) ||
            std::abs(mouseState.lX) > 2 ||
            std::abs(mouseState.lY) > 2) {
            hasKeyMouseInput = true;
        }
    }

    bool hasPadInput = false;
    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        if (state_[i].dwPacketNumber == 0) {
            continue;
        }
        const auto& gp = state_[i].Gamepad;
        if (gp.wButtons != 0 ||
            gp.bLeftTrigger > 30 ||
            gp.bRightTrigger > 30 ||
            std::abs(gp.sThumbLX) > deadZoneL_[i] ||
            std::abs(gp.sThumbLY) > deadZoneL_[i] ||
            std::abs(gp.sThumbRX) > deadZoneR_[i] ||
            std::abs(gp.sThumbRY) > deadZoneR_[i]) {
            hasPadInput = true;
            break;
        }
    }

    if (hasPadInput) {
        lastInputDevice_ = InputDeviceType::Gamepad;
    } else if (hasKeyMouseInput) {
        lastInputDevice_ = InputDeviceType::KeyboardMouse;
    }
}

bool Input::PushedKeyDown(BYTE keys)
{

    return (key[keys] & 0x80) && (preKey[keys] & 0x80);
}

bool Input::PushedKeyUp(BYTE keys)
{

    return !(key[keys] & 0x80) && !(preKey[keys] & 0x80);

}

bool Input::TriggerKeyDown(BYTE keys)
{
    return (key[keys] & 0x80) && !(preKey[keys] & 0x80);
}

bool Input::TriggerKeyUp(BYTE keys)
{
     return !(key[keys] & 0x80) && (preKey[keys] & 0x80);

}
bool Input::PushMouseDown(int32_t button) {
    // マウスボタンが押されているか確認
    int btnIndex = static_cast<int>(button);
    return (mouseState.rgbButtons[btnIndex] & 0x80) != 0;
}

bool Input::PushMouseUP(int32_t button) {
    // マウスボタンが押されていないか確認
    int btnIndex = static_cast<int>(button);
    return (mouseState.rgbButtons[btnIndex] & 0x80) == 0;
}

bool Input::TriggerMouseDown(int32_t button) {
    // マウスボタンがトリガーされたか確認（押した瞬間）
    int btnIndex = static_cast<int>(button);
    return ((mouseState.rgbButtons[btnIndex] & 0x80) != 0 &&
        (preMouseState.rgbButtons[btnIndex] & 0x80) == 0);
}

bool Input::TriggerMouseUP(int32_t button) {
    // マウスボタンがトリガーされたか確認（離した瞬間）
    int btnIndex = static_cast<int>(button);
    return ((mouseState.rgbButtons[btnIndex] & 0x80) == 0 &&
        (preMouseState.rgbButtons[btnIndex] & 0x80) != 0);
}

Input::MoveMouse Input::GetMouseMove()
{
    return MoveMouse(
        mouseState.lX,
        mouseState.lY,
        mouseState.lZ // ホイール
    );
}

bool Input::GetJoyStick(int32_t stickNo, XINPUT_STATE& out) const
{

    if (stickNo < 0 || stickNo >= XUSER_MAX_COUNT) return false;

    // メンバ変数に保存されている最新の状態をコピー
    out = state_[stickNo];

    // 接続チェック (パケット番号が変わっていなければ切断とみなす等の判定も可能だが、API戻り値で判定済みとして扱う)
    // ここでは簡易的にパケット番号0なら未接続とする
    if (out.dwPacketNumber == 0) return false;

    // --- デッドゾーン処理 (Lスティック) ---
    // 単純な切捨て処理（円形デッドゾーンにする場合はベクトル計算が必要ですが、ここでは簡易実装とします）
    if (std::abs(out.Gamepad.sThumbLX) < deadZoneL_[stickNo]) out.Gamepad.sThumbLX = 0;
    if (std::abs(out.Gamepad.sThumbLY) < deadZoneL_[stickNo]) out.Gamepad.sThumbLY = 0;

    // --- デッドゾーン処理 (Rスティック) ---
    if (std::abs(out.Gamepad.sThumbRX) < deadZoneR_[stickNo]) out.Gamepad.sThumbRX = 0;
    if (std::abs(out.Gamepad.sThumbRY) < deadZoneR_[stickNo]) out.Gamepad.sThumbRY = 0;

    return true;
}

bool Input::GetPreJoyStick(int32_t stickNo, XINPUT_STATE& out) const
{
    if (stickNo < 0 || stickNo >= XUSER_MAX_COUNT) return false;

    out = preState_[stickNo];

    if (out.dwPacketNumber == 0) return false;

    return true;
}

void Input::SetDeadZone(int32_t stickNo, int32_t deadZoneL, int32_t deadZoneR)
{
    if (stickNo < 0 || stickNo >= XUSER_MAX_COUNT) return;

    deadZoneL_[stickNo] = deadZoneL;
    deadZoneR_[stickNo] = deadZoneR;
}

size_t Input::GetConnectedStickNum()
{
    size_t count = 0;
    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        if (state_[i].dwPacketNumber != 0) {
            count++;
        }
    }
    return count;
}

// -----------------------------------------------------------
// ボタンが押され続けているか (Press)
// -----------------------------------------------------------
bool Input::PushPadDown(int32_t stickNo, WORD button) const
{
    if (stickNo < 0 || stickNo >= XUSER_MAX_COUNT) return false;
    // 接続チェックが必要ならここに入れる (例: if (state_[stickNo].dwPacketNumber == 0) return false;)

    // 現在のフレームで押されているか
    return (state_[stickNo].Gamepad.wButtons & button) != 0;
}

// -----------------------------------------------------------
// ボタンが押されていないか (Release state)
// -----------------------------------------------------------
bool Input::PushPadUP(int32_t stickNo, WORD button) const
{
    if (stickNo < 0 || stickNo >= XUSER_MAX_COUNT) return false;

    // 現在のフレームで押されていないか
    return (state_[stickNo].Gamepad.wButtons & button) == 0;
}

// -----------------------------------------------------------
// ボタンを押した瞬間か (Trigger)
// -----------------------------------------------------------
bool Input::TriggerPadDown(int32_t stickNo, WORD button) const
{
    if (stickNo < 0 || stickNo >= XUSER_MAX_COUNT) return false;

    // 「現在は押されている」かつ「前は押されていなかった」
    return ((state_[stickNo].Gamepad.wButtons & button) != 0) &&
        ((preState_[stickNo].Gamepad.wButtons & button) == 0);
}

// -----------------------------------------------------------
// ボタンを離した瞬間か (Release Trigger)
// -----------------------------------------------------------
bool Input::TriggerPadUp(int32_t stickNo, WORD button) const
{
    if (stickNo < 0 || stickNo >= XUSER_MAX_COUNT) return false;

    // 「現在は押されていない」かつ「前は押されていた」
    return ((state_[stickNo].Gamepad.wButtons & button) == 0) &&
        ((preState_[stickNo].Gamepad.wButtons & button) != 0);
}


