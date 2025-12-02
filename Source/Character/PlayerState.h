#pragma once
#include <unordered_map>

#include "Player.h"

// 前方宣言
class Player;
enum class PlayerStateId;

// ステート基底
class PlayerState
{
public:
	PlayerState(Player* player);
	virtual ~PlayerState() = default;

	// 開始処理
	virtual void Enter() {}

	// 終了処理
	virtual void Exit() {}

	// 更新処理
	virtual void Update(float elapsedTime) {}

	// デバッグ用GUI描画
	virtual void DrawDebugGUI() {}

protected:
	// ステート変更
	void ChangeState(PlayerStateId stateId);

	// 歩き移動入力
	bool InputWalkMove() const;

	// 走り移動入力
	bool InputRunMove() const;

	// ロックオンしている場合はストレイフ
	void LockOnStrafe(int rightIndex, int leftIndex, int frontIndex, int backIndex);

	enum class InputActionType
	{
		None,
		LightAttack,
		HeavyAttack,
		Dodge,
		Guard,
		Jump
	};

	InputActionType InputAction();
	InputActionType currentInput;

protected:
	Player* player = nullptr;
	std::unordered_map<InputActionType, PlayerStateId> inputToNextState;
};

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
	int dodgeBackMovePow = 2;
	int airDodgeAnimationIndex = -1;
	float rollingFrontAnimationSpeed = 1.0f;
	float dodgeAnimationTime = 1.333f;
	float airDodgeAnimationTime = 0.583f;
	int rollingFrontMovePow = 3;
	float timer = 0.0f;
	bool isDodgeBack = false;
	InputActionType nextInput;
	bool nextShiftReady = false;
};

// コンボステート
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
	int	  attackDamage = 1.0f;
	float invisibleTime = 0.5f;

	// コントローラーの振動変数
	float attackLeftVibrate = 1.0f;
	float attackRightVibrate = 1.0f;

	// 攻撃時ヒットストップ変数
	float attackHitStopTime = 1.0f;
	float attackHitStopSpeed = 0.1f;
};

// コンボス1ステート
class PlayerCombo1State : public PlayerComboState
{
public:
	PlayerCombo1State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// コンボス2ステート
class PlayerCombo2State : public PlayerComboState
{
public:
	PlayerCombo2State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// コンボス3ステート
class PlayerCombo3State : public PlayerComboState
{
public:
	PlayerCombo3State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// コンボス4ステート
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
	InputActionType nextInput;
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
	bool nextShiftReady;
	InputActionType nextInput;
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
	int guardParryAnimationIndex = -1;
	float guardParryAnimationSpeed = 1.0f;
	float timer = 0.0f;

	bool nextShiftReady;
	InputActionType nextInput;
};

// ダメージの種類を定義
enum class DamageType
{
	Normal,      // 通常ダメージ
	Light,       // 軽いノックバック
	Heavy,       // 重いノックバック
	Launch,      // 打ち上げ
	Knockdown    // 打ち落とし
};
// ダメージステート
class PlayerDamageState : public PlayerState
{
public:
	PlayerDamageState(Player* player);

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
	// アニメーションインデックス
	struct DamageAnimations
	{
		int normalGround;
		int lightGround;
		int heavyGround;
		int airStart;
		int airLoop;
		int airEnd;
		int getUp;
	} anims;

	DamageType currentDamageType = DamageType::Normal;
	float pauseTimer = 0.0f;
	int step = 0;
	DirectX::XMFLOAT3 knockbackTargetPosition = { 0.0f, 0.0f, 0.0f };

	// ダメージタイプ判定
	DamageType GetCurrentDamageType()
	{
		if (player->IsKnockDownDamage())
			return DamageType::Knockdown;
		else if (player->IsLaunchDamage())
			return DamageType::Launch;
		else if (player->IsHeavyDamage())
			return DamageType::Heavy;
		else if (player->IsLightDamage())
			return DamageType::Light;
		else
			return DamageType::Normal;
	}

	// ダメージタイプ別の初期処理
	void HandleDamageStart(DamageType type, float elapsedTime);

	DirectX::XMFLOAT3 GetKnockbackPosition(DamageType type)
	{
		switch (type)
		{
		case DamageType::Normal:
			return player->CalculateKnockbackPosition(player->normalKnockbackPower);
		case DamageType::Light:
			return player->CalculateKnockbackPosition(player->lightKnockbackPower);
		case DamageType::Heavy:
			return player->CalculateKnockbackPosition(player->heavyKnockbackPower);
		case DamageType::Launch:
		{
			DirectX::XMFLOAT3 launchPos = player->CalculateKnockbackPosition(1.5f);
			launchPos.y = player->GetPosition().y + player->launchKnockbackHeight;
			return launchPos;
		}
		case DamageType::Knockdown:
			return player->CalculateKnockbackPosition(player->knockdownKnockbackPower);
		default:
			return player->CalculateKnockbackPosition(player->normalKnockbackPower);
		}
	}

	void TurnToDamageDirection(float elapsedTime)
	{
		DirectX::XMFLOAT3 damageDir = player->GetDamageDirection();

		// ダメージ方向がゼロベクトルの場合は処理しない
		float length = sqrtf(damageDir.x * damageDir.x + damageDir.z * damageDir.z);
		if (length < 0.001f) return;

		// ダメージ方向を向く角度を計算
		float targetAngle = atan2f(damageDir.x, damageDir.z);

		// 現在の角度を取得
		DirectX::XMFLOAT3 angle = player->GetAngle();

		// 即座に向きを変更
		angle.y = targetAngle;
		player->SetAngle(angle);
	}
};

// 死亡ステート
class PlayerDeadState : public PlayerState
{
public:
	PlayerDeadState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

private:
	int deadAnimationIndex = -1;
	bool notificated = false;
};