#pragma once
#include "PlayerState.h"

// ガード待機ステート
class PlayerGuardIdle : public PlayerState
{
public:
	PlayerGuardIdle(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 終了処理
	void Exit() override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int guardStartAnimationIndex = -1;
	int guardLoopAnimationIndex = -1;
	int guardEndAnimationIndex = -1;
	int index;
	float guardIdleAnimationSpeed = 1.0f;
	float frame = 0.0f;
	float timer = 0.0f;
	bool isLoop = false;
	AnimationConfig* config;
	InputActionType nextInput = InputActionType::None;
};

// ガードヒットステート
class PlayerGuardHit : public PlayerState
{
public:
	PlayerGuardHit(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 終了処理
	void Exit() override;

private:
	int guardHitAnimationIndex = -1;
	float timer = 0.0f;
	bool nextShiftReady = false;
	InputActionType nextInput = InputActionType::None;
};

// ガードカウンターステート
class PlayerGuardCounter : public PlayerState
{
public:
	PlayerGuardCounter(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 終了処理
	void Exit() override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int counterMiddleAnimationIndex = -1;
	int counterCloseAnimationIndex = -1;
	int counterRushAnimationIndex = -1;
	float guardParryAnimationSpeed = 1.0f;
	float timer = 0.0f;

	bool nextShiftReady = false;
	InputActionType nextInput = InputActionType::None;
	DirectX::XMFLOAT3 enemyPos;
};
