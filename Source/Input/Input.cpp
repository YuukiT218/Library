#include "Input/Input.h"
#include <Xinput.h>

Input* Input::instance = nullptr;

// コンストラクタ
Input::Input(HWND hWnd)
    : mouse(hWnd)
{
    instance = this;
}

// 更新処理
void Input::Update()
{
    gamePad.Update();
    IsThumbStickMoved();

    mouse.Update();
}

void Input::IsThumbStickMoved()
{
    XINPUT_STATE xinputState = {};
    DWORD result = XInputGetState(0, &xinputState); // プレイヤー1（コントローラー0）

    if (result == ERROR_SUCCESS) {
        // ゲームパッドの状態を取得できた
        XINPUT_GAMEPAD& pad = xinputState.Gamepad;

        // ボタン入力を判定
        if (pad.wButtons != 0) {
            pushGamePad = true; // 何かのボタンが押された場合
        }

        // LスティックとRスティックの入力値を取得
        short thumbLX = pad.sThumbLX;
        short thumbLY = pad.sThumbLY;
        short thumbRX = pad.sThumbRX;
        short thumbRY = pad.sThumbRY;

        // デッドゾーンを考慮してLスティックまたはRスティックの入力を判定
        if (abs(thumbLX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
            abs(thumbLY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
            abs(thumbRX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ||
            abs(thumbRY) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
            pushGamePad = true; // 入力がある場合はTrueに設定
        }
        // 入力がなくてもPushGamePadの状態は維持する
    }
    else {
        // ゲームパッドが接続されていない場合のみFalseにする
        pushGamePad = false;
    }

    if (mouse.GetButtonDown())
    {
        pushGamePad = false;
    }
}

