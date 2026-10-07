#pragma once

using GamePadButton = unsigned int;

// ゲームパッド
class GamePad
{
public:
	static const GamePadButton BTN_UP = (1 << 0);
	static const GamePadButton BTN_RIGHT = (1 << 1);
	static const GamePadButton BTN_DOWN = (1 << 2);
	static const GamePadButton BTN_LEFT = (1 << 3);
	static const GamePadButton BTN_A = (1 << 4);
	static const GamePadButton BTN_B = (1 << 5);
	static const GamePadButton BTN_X = (1 << 6);
	static const GamePadButton BTN_Y = (1 << 7);
	static const GamePadButton BTN_START = (1 << 8);
	static const GamePadButton BTN_BACK = (1 << 9);
	static const GamePadButton BTN_LEFT_THUMB = (1 << 10);
	static const GamePadButton BTN_RIGHT_THUMB = (1 << 11);
	static const GamePadButton BTN_LEFT_SHOULDER = (1 << 12);
	static const GamePadButton BTN_RIGHT_SHOULDER = (1 << 13);
	static const GamePadButton BTN_LEFT_TRIGGER = (1 << 14);
	static const GamePadButton BTN_RIGHT_TRIGGER = (1 << 15);

	static const GamePadButton BTN_A_EMU = (1 << 16);
public:
	GamePad() {}
	~GamePad() {}

	// 更新
	void Update();

	// 振動させる(0.0f ～ 1.0fの範囲で強さを指定)
	void Vibrate(float leftMotor, float rightMotor);

	// スロット設定
	void SetSlot(int slot) { this->slot = slot; }

	// ボタン入力状態の取得
	GamePadButton GetButton() const { return buttonState[CURRENT_STATE]; }

	// ボタン押下状態の取得
	GamePadButton GetButtonDown() const { return buttonDown; }

	// ボタン押上状態の取得
	GamePadButton GetButtonUp() const { return buttonUp; }

	// 左スティックX軸入力状態の取得
	float GetAxisLX() const { return axisLx; }

	// 左スティックY軸入力状態の取得
	float GetAxisLY() const { return axisLy; }

	// 右スティックX軸入力状態の取得
	float GetAxisRX() const { return axisRx; }

	// 右スティックY軸入力状態の取得
	float GetAxisRY() const { return axisRy; }

	// 左スティック入力状態の力取得
	float GetLAxisPower() const { return axisLPower; }

	// 右スティック入力状態の力取得
	float GetRAxisPower() const { return axisRPower; }

	// 左トリガー入力状態の取得
	float GetTriggerL() const { return triggerL; }

	// 右トリガー入力状態の取得
	float GetTriggerR() const { return triggerR; }

private:
	// ボタン状態の履歴（今フレームと前フレーム）
	static constexpr int CURRENT_STATE = 0;
	static constexpr int PREVIOUS_STATE = 1;
	static constexpr int STATE_HISTORY_COUNT = 2;
	GamePadButton		buttonState[STATE_HISTORY_COUNT] = { 0 };
	GamePadButton		buttonDown = 0;
	GamePadButton		buttonUp = 0;
	float				axisLx = 0.0f;
	float				axisLy = 0.0f;
	float				axisRx = 0.0f;
	float				axisRy = 0.0f;
	float				axisLPower = 0.0f;
	float				axisRPower = 0.0f;
	float				triggerL = 0.0f;
	float				triggerR = 0.0f;
	int					slot = 0;
};
