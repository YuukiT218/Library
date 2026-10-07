#pragma once
#include "PlayerState.h"
#include <DirectXMath.h>

// コンボテート
class PlayerComboState : public PlayerState
{
public:
	PlayerComboState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 終了処理
	void Exit() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// コンボ共通のデバッグ用GUI描画
	void DrawComboDebugGUI(const char* label);

protected:
	PlayerStateId nextStateId = PlayerStateId::EnumCount;
	InputActionType nextInput;
	int comboAnimationIndex = -1;
	int airComboAnimationIndex = -1;
	int dashAttackAnimationIndex = -1;
	int airDashAttackAnimationIndex = -1;
	float comboAttackSpeed = 1.0f;
	float comboPoseSpeed = 1.0f;
	float poseFrame = 0.0f;
	float endFrame = 0.0f;
	float nextShiftFrame = 0.0f;
	bool nextShiftReady = false;
	bool isBakeY = true;

	// 敵との距離の移動値と回転値
	float moveRate = 1.0f;
	float turnRate = 1.0f;

	float forwardPower = 0.0f;
	float forwardFrame = 0.0f;
	bool forwarded = false;

	// 攻撃判定必要変数
	float attackCollisionStartFrame = 0.0f;
	float attackCollisionEndFrame = 1.0f;
	int	  attackDamage = 1;
	float invincibleTime = 0.5f;

	// コントローラーの振動変数
	float attackLeftVibrate = 1.0f;
	float attackRightVibrate = 1.0f;

	// 攻撃時ヒットストップ変数
	float attackHitStopTime = 1.0f;
	float attackHitStopSpeed = 0.1f;

	DirectX::XMFLOAT3 enemyPos;
};

// コンボ1ステート
class PlayerCombo1State : public PlayerComboState
{
public:
	PlayerCombo1State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// コンボ2ステート
class PlayerCombo2State : public PlayerComboState
{
public:
	PlayerCombo2State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// コンボ3ステート
class PlayerCombo3State : public PlayerComboState
{
public:
	PlayerCombo3State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// コンボ4ステート
class PlayerCombo4State : public PlayerComboState
{
public:
	PlayerCombo4State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// コンボ5ステート
class PlayerCombo5State : public PlayerComboState
{
public:
	PlayerCombo5State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// 強攻撃1ステート
class PlayerHeavyAttack1State : public PlayerComboState
{
public:
	PlayerHeavyAttack1State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};
