#pragma once

#include "Input/GamePad.h"
#include "Input/Mouse.h"

// インプット
class Input
{
public:
	Input(HWND hWnd);
	~Input() {}

public:
	// インスタンス取得
	static Input& Instance() { return *instance; }

	// 更新処理
	void Update();

	// ゲームパッド取得
	GamePad& GetGamePad() { return gamePad; }

	// マウス取得
	Mouse& GetMouse() { return mouse; }

	bool GetAnyButton() { return anyKeyPressed; }

	void OnKeyDown() { anyKeyPressed = true; }
	void OnKeyUp() { anyKeyPressed = false; }

	void SetIsLastGamePad(bool button) { PushGamePad = button; }
	bool GetIsLastGamePad() { return PushGamePad; }


	void IsThumbStickMoved();
private:
	static Input* instance;
	GamePad				gamePad;
	Mouse				mouse;

private:
	bool anyKeyPressed = false; // 何かのキーが押されたかを示すフラグ
	bool PushGamePad = false;
};
