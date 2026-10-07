#pragma once
#include "PlayerState.h"

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

protected:
	InputActionType nextInput;
	bool nextShiftReady = false;

private:
	// ダメージ処理の進行段階
	enum class Step
	{
		Start,       // ダメージ開始
		Reaction,    // 被弾アニメーション再生中
		AirLoop,     // 空中で着地待ち
		Landing,     // 着地アニメーション
		GetUp,       // 起き上がり
		LaunchApex,  // 打ち上げの最高到達点で停止
	};

	// 打ち上げ時の水平方向のノックバック量
	static constexpr float LAUNCH_HORIZONTAL_KNOCKBACK = -1.5f;

	// ダメージ方向をゼロベクトルとみなす長さ
	static constexpr float DAMAGE_DIRECTION_MIN_LENGTH = 0.001f;

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
	Step step = Step::Start;
	DirectX::XMFLOAT3 knockbackTargetPosition = { 0.0f, 0.0f, 0.0f };

	// ダメージタイプ判定
	DamageType GetCurrentDamageType()
	{
		if (player->IsKnockdownDamage())
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

	// ノックバック目標位置へ水平方向に寄せる
	void MoveTowardKnockbackTarget(float lerpRate);

	// 受付時間内の回避入力を先行入力として記録する
	void AcceptDodgeInput(float frame, const AnimationConfig* config);

	// 先行入力を消費する。回避ステートへ遷移した場合はtrueを返す
	bool TryConsumeAdvanceInput(float frame, const AnimationConfig* config);

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
			DirectX::XMFLOAT3 launchPos = player->CalculateKnockbackPosition(LAUNCH_HORIZONTAL_KNOCKBACK);
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
		if (length < DAMAGE_DIRECTION_MIN_LENGTH) return;

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
	Effekseer::Handle handle = -1;
};