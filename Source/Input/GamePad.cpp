#include <windows.h>
#include <math.h>
#include <Xinput.h>
#include "Input/GamePad.h"

namespace
{
	// トリガーの最大値（BYTE）
	constexpr float TRIGGER_MAX = 255.0f;

	// スティックの最大値（SHORT の絶対値）
	constexpr float THUMB_MAX = static_cast<float>(0x8000);

	// 振動の強さの最大値（WORD）
	constexpr float VIBRATION_MAX = 65535.0f;

	// GetAsyncKeyState でキーが押されていることを表すビット
	constexpr SHORT KEY_DOWN_MASK = static_cast<SHORT>(0x8000);

	// キーが押されているか
	bool IsKeyDown(int virtualKey)
	{
		return (GetAsyncKeyState(virtualKey) & KEY_DOWN_MASK) != 0;
	}
}

// 更新
void GamePad::Update()
{
	axisLx = axisLy = 0.0f;
	axisRx = axisRy = 0.0f;
	triggerL = triggerR = 0.0f;

	GamePadButton newButtonState = 0;

	// ボタン情報取得
	XINPUT_STATE xinputState;
	if (XInputGetState(slot, &xinputState) == ERROR_SUCCESS)
	{
		//XINPUT_CAPABILITIES caps;
		//XInputGetCapabilities(m_slot, XINPUT_FLAG_GAMEPAD, &caps);
		XINPUT_GAMEPAD& pad = xinputState.Gamepad;

		if (pad.wButtons & XINPUT_GAMEPAD_DPAD_UP)					newButtonState |= BTN_UP;
		if (pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)				newButtonState |= BTN_RIGHT;
		if (pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)				newButtonState |= BTN_DOWN;
		if (pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)				newButtonState |= BTN_LEFT;
		if (pad.wButtons & XINPUT_GAMEPAD_A)						newButtonState |= BTN_A;
		if (pad.wButtons & XINPUT_GAMEPAD_A)						newButtonState |= BTN_A_EMU;
		if (pad.wButtons & XINPUT_GAMEPAD_B)						newButtonState |= BTN_B;
		if (pad.wButtons & XINPUT_GAMEPAD_X)						newButtonState |= BTN_X;
		if (pad.wButtons & XINPUT_GAMEPAD_Y)						newButtonState |= BTN_Y;
		if (pad.wButtons & XINPUT_GAMEPAD_START)					newButtonState |= BTN_START;
		if (pad.wButtons & XINPUT_GAMEPAD_BACK)						newButtonState |= BTN_BACK;
		if (pad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)				newButtonState |= BTN_LEFT_THUMB;
		if (pad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)				newButtonState |= BTN_RIGHT_THUMB;
		if (pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)			newButtonState |= BTN_LEFT_SHOULDER;
		if (pad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)			newButtonState |= BTN_RIGHT_SHOULDER;
		if (pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)	newButtonState |= BTN_LEFT_TRIGGER;
		if (pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)	newButtonState |= BTN_RIGHT_TRIGGER;

		if ((pad.sThumbLX <  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && pad.sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) &&
			(pad.sThumbLY <  XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && pad.sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE))
		{
			pad.sThumbLX = 0;
			pad.sThumbLY = 0;
		}

		if ((pad.sThumbRX <  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && pad.sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) &&
			(pad.sThumbRY <  XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && pad.sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE))
		{
			pad.sThumbRX = 0;
			pad.sThumbRY = 0;
		}

		triggerL = static_cast<float>(pad.bLeftTrigger) / TRIGGER_MAX;
		triggerR = static_cast<float>(pad.bRightTrigger) / TRIGGER_MAX;
		axisLx = static_cast<float>(pad.sThumbLX) / THUMB_MAX;
		axisLy = static_cast<float>(pad.sThumbLY) / THUMB_MAX;
		axisRx = static_cast<float>(pad.sThumbRX) / THUMB_MAX;
		axisRy = static_cast<float>(pad.sThumbRY) / THUMB_MAX;
	}
	else
	{
#if 0
		// XInputで入力情報が取得出来なかった場合はWindowsAPIで取得する
		JOYINFOEX joyInfo;
		joyInfo.dwSize = sizeof(JOYINFOEX);
		joyInfo.dwFlags = JOY_RETURNALL;	// 全ての情報を取得

		if (joyGetPosEx(slot, &joyInfo) == JOYERR_NOERROR)
		{
			// 製品IDをチェックしてPS4コントローラーだけ対応する
			static const WORD PS4_PID = 1476;

			JOYCAPS joyCaps;
			if (joyGetDevCaps(slot, &joyCaps, sizeof(JOYCAPS)) == JOYERR_NOERROR)
			{
				// 十字キー
				if (joyInfo.dwPOV != 0xFFFF)
				{
					static const int povBit[8] =
					{
						BTN_UP,					// 上
						BTN_RIGHT | BTN_UP,		// 右上
						BTN_RIGHT,				// 右
						BTN_RIGHT | BTN_DOWN,	// 右下
						BTN_DOWN,				// 下
						BTN_LEFT | BTN_DOWN,	// 左下
						BTN_LEFT,				// 左
						BTN_LEFT | BTN_UP		// 左上
					};
					int angle = joyInfo.dwPOV / 4500;
					newButtonState |= povBit[angle];
				}
				if (joyCaps.wPid == PS4_PID)
				{
					// ボタン情報
					if (joyInfo.dwButtons & JOY_BUTTON1)  newButtonState |= BTN_Y;
					if (joyInfo.dwButtons & JOY_BUTTON2)  newButtonState |= BTN_B;
					if (joyInfo.dwButtons & JOY_BUTTON3)  newButtonState |= BTN_A;
					if (joyInfo.dwButtons & JOY_BUTTON4)  newButtonState |= BTN_X;
					if (joyInfo.dwButtons & JOY_BUTTON5)  newButtonState |= BTN_LEFT_SHOULDER;
					if (joyInfo.dwButtons & JOY_BUTTON6)  newButtonState |= BTN_RIGHT_SHOULDER;
					if (joyInfo.dwButtons & JOY_BUTTON7)  newButtonState |= BTN_LEFT_TRIGGER;
					if (joyInfo.dwButtons & JOY_BUTTON8)  newButtonState |= BTN_RIGHT_TRIGGER;
					if (joyInfo.dwButtons & JOY_BUTTON9)  newButtonState |= BTN_BACK;
					if (joyInfo.dwButtons & JOY_BUTTON10) newButtonState |= BTN_START;
					if (joyInfo.dwButtons & JOY_BUTTON11) newButtonState |= BTN_LEFT_THUMB;
					if (joyInfo.dwButtons & JOY_BUTTON12) newButtonState |= BTN_RIGHT_THUMB;
					//if (joyInfo.dwButtons & JOY_BUTTON13) newButtonState |= BTN_?;	// PS
					//if (joyInfo.dwButtons & JOY_BUTTON14) newButtonState |= BTN_?;	// Touch

					// 左スティック
					axisLx = static_cast<int>(joyInfo.dwXpos - 0x7FFF) / static_cast<float>(0x8000);
					axisLy = -static_cast<int>(joyInfo.dwYpos - 0x7FFF) / static_cast<float>(0x8000);

					// 右スティック
					axisRx = static_cast<int>(joyInfo.dwZpos - 0x7FFF) / static_cast<float>(0x8000);
					axisRy = -static_cast<int>(joyInfo.dwRpos - 0x7FFF) / static_cast<float>(0x8000);

					// LRトリガー
					triggerL = static_cast<float>(joyInfo.dwVpos) / static_cast<float>(0xFFFF);
					triggerR = static_cast<float>(joyInfo.dwUpos) / static_cast<float>(0xFFFF);

					if (axisLx > -0.25f && axisLx < 0.25f) axisLx = 0.0f;
					if (axisRx > -0.25f && axisRx < 0.25f) axisRx = 0.0f;
				}
			}
		}
#endif
	}

	// キーボードでエミュレーション
	{
		float lx = 0.0f;
		float ly = 0.0f;
		float rx = 0.0f;
		float ry = 0.0f;
		if (IsKeyDown('W')) ly = 1.0f;
		if (IsKeyDown('A')) lx = -1.0f;
		if (IsKeyDown('S')) ly = -1.0f;
		if (IsKeyDown('D')) lx = 1.0f;
		if (IsKeyDown('I')) ry = 1.0f;
		if (IsKeyDown('J')) rx = -1.0f;
		if (IsKeyDown('K')) ry = -1.0f;
		if (IsKeyDown('L')) rx = 1.0f;
		if (IsKeyDown(VK_SPACE)) newButtonState |= BTN_A;
		if (IsKeyDown(VK_RETURN)) newButtonState |= BTN_A_EMU;
		//if (IsKeyDown('P')) newButtonState |= BTN_B;
		if (IsKeyDown(VK_SHIFT)) newButtonState |= BTN_X;
		//if (IsKeyDown(VK_CONTROL)) newButtonState |= BTN_Y;
		if (IsKeyDown(VK_CONTROL)) newButtonState |= BTN_LEFT_THUMB;
		if (IsKeyDown('Q')) newButtonState |= BTN_RIGHT_SHOULDER;

		//矢印キーにも対応させる
		// 上
		if (IsKeyDown('W') || IsKeyDown(VK_UP))
			newButtonState |= BTN_UP;

		// 右
		if (IsKeyDown('D') || IsKeyDown(VK_RIGHT))
			newButtonState |= BTN_RIGHT;

		// 下
		if (IsKeyDown('S') || IsKeyDown(VK_DOWN))
			newButtonState |= BTN_DOWN;

		// 左
		if (IsKeyDown('A') || IsKeyDown(VK_LEFT))
			newButtonState |= BTN_LEFT;


		if (IsKeyDown(VK_ESCAPE))	newButtonState |= BTN_START;

#if 0
		if (newButtonState & BTN_UP)    ly = 1.0f;
		if (newButtonState & BTN_RIGHT) lx = 1.0f;
		if (newButtonState & BTN_DOWN)  ly = -1.0f;
		if (newButtonState & BTN_LEFT)  lx = -1.0f;
#endif

		if (lx >= 1.0f || lx <= -1.0f || ly >= 1.0f || ly <= -1.0f)
		{
			float power = ::sqrtf(lx * lx + ly * ly);
			axisLx = lx / power;
			axisLy = ly / power;
		}

		if (rx >= 1.0f || rx <= -1.0f || ry >= 1.0f || ry <= -1.0f)
		{
			float power = ::sqrtf(rx * rx + ry * ry);
			axisRx = rx / power;
			axisRy = ry / power;
		}
	}

	// 軸の力を計算
	{
		axisLPower = sqrtf(axisLx * axisLx + axisLy * axisLy);
		axisRPower = sqrtf(axisRx * axisRx + axisRy * axisRy);
	}

	// ボタン情報の更新
	{
		buttonState[PREVIOUS_STATE] = buttonState[CURRENT_STATE];	// スイッチ履歴
		buttonState[CURRENT_STATE] = newButtonState;

		buttonDown = ~buttonState[PREVIOUS_STATE] & newButtonState;	// 押した瞬間
		buttonUp = ~newButtonState & buttonState[PREVIOUS_STATE];	// 離した瞬間
	}
}

// 振動させる(0.0f ～ 1.0fの範囲で強さを指定)
void GamePad::Vibrate(float leftMotor, float rightMotor)
{
	XINPUT_VIBRATION vibration = {};
	vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * VIBRATION_MAX);
	vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * VIBRATION_MAX);

	XInputSetState(slot, &vibration);	// slotはコントローラー番号(0～3)
}
