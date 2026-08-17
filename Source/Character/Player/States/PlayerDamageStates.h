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
			DirectX::XMFLOAT3 launchPos = player->CalculateKnockbackPosition(-1.5f);
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
	Effekseer::Handle handle = -1;
};