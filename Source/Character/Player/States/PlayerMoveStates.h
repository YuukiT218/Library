#pragma once
#include "PlayerState.h"
#include <DirectXMath.h>

// 待機ステート
class PlayerIdleState : public PlayerState
{
public:
	PlayerIdleState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int idleAnimationIndex = -1;
	float idleAnimationSpeed = 1.0f;
};

// 歩きステート
class PlayerWalkState : public PlayerState
{
public:
	PlayerWalkState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int walkFrontAnimationIndex = -1;
	int walkBackAnimationIndex = -1;
	int walkRightAnimationIndex = -1;
	int walkLeftAnimationIndex = -1;
	float walkAnimationSpeed = 1.0f;
	float walkAnimationMoveRate = 0.45f;
};

// 走りステート
class PlayerRunState : public PlayerState
{
public:
	PlayerRunState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int runStartAnimationIndex = -1;
	int runLoopAnimationIndex = -1;
	int runEndAnimationIndex = -1;
	int runRightAnimationIndex = -1;
	int runLeftAnimationIndex = -1;
	float runAnimationSpeed = 1.0f;
	float runAnimationMoveRate = 0.9f;
};

// ジャンプステート
class PlayerJumpState : public PlayerState
{
public:
	PlayerJumpState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int jumpAnimationIndex = -1;
	int fallAnimationIndex = -1;
	float jumpAnimationSpeed = 1.0f;
	float jumpAnimationMoveRate = 0.7f;
	float jumpPower = 10.0f;
};

// 落下ステート
class PlayerFallState : public PlayerState
{
public:
	PlayerFallState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int fallAnimationIndex = -1;
	float fallAnimationSpeed = 1.0f;
	float fallAnimationMoveRate = 0.7f;
};

// 回避ステート
class PlayerDodgeState : public PlayerState
{
public:
	PlayerDodgeState(Player* player);

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
	int dodgeAnimationIndex = -1;
	float dodgeBackAnimationSpeed = 1.0f;
	float dodgeBackAnimationTime = 0.7f;
	int dodgeBackMovePower = 2;
	int airDodgeAnimationIndex = -1;
	float rollingFrontAnimationSpeed = 1.0f;
	float dodgeAnimationTime = 1.333f;
	float airDodgeAnimationTime = 0.583f;
	int rollingFrontMovePower = 3;
	float timer = 0.0f;
	bool isDodgeBack = false;
	InputActionType nextInput;
	bool nextShiftReady = false;
};
