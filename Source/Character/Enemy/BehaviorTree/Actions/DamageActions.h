#pragma once
#include "EnemyActionBase.h"
#include "Character/Player/Player.h"
#include "Math/Mathf.h"

// ダメージの種類を定義
enum class EnemyDamageType
{
	Normal,      // 通常ダメージ
	Light,       // 軽いノックバック
	Heavy,       // 重いノックバック
	Launch       // 打ち上げ
};

// 統合ダメージアクション
template <typename ActorType>
class UnifiedDamageAction : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	UnifiedDamageAction(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	State Run(float elapsedTime) override;

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

	EnemyDamageType currentDamageType = EnemyDamageType::Normal;
	bool animationsInitialized = false;
	float pauseTimer = 0.0f;

	// 地面からこれ以上離れていれば空中で被弾したとみなす
	static constexpr float AIR_DAMAGE_MIN_HEIGHT = 0.5f;

	// 打ち上げ関連のアニメーションのブレンド時間
	static constexpr float LAUNCH_BLEND_SECONDS = 0.1f;

	// 空中で被弾した時の重力と浮き上がり速度
	static constexpr float AIR_DAMAGE_GRAVITY = -0.15f;
	static constexpr float AIR_DAMAGE_RISE_VELOCITY = 2.0f;

	// 重攻撃を空中で受けた時の重力
	static constexpr float AIR_HEAVY_DAMAGE_GRAVITY = -0.2f;

	// 重攻撃を地上で受けた時の浮き上がり速度
	static constexpr float GROUND_HEAVY_DAMAGE_RISE_VELOCITY = 1.5f;

	// 打ち上げ初速の計算係数（v = sqrt(係数 * |g| * h)）
	static constexpr float LAUNCH_VELOCITY_COEFFICIENT = 100.0f;

	// 打ち上げの最高到達点で停止する時間
	static constexpr float LAUNCH_APEX_PAUSE_SECONDS = 0.3f;

	// ノックバック位置へ寄せる補間率
	static constexpr float NORMAL_AIR_KNOCKBACK_LERP = 0.15f;
	static constexpr float LIGHT_AIR_KNOCKBACK_LERP = 0.4f;
	static constexpr float GROUND_KNOCKBACK_LERP = 0.25f;
	static constexpr float HEAVY_AIR_KNOCKBACK_LERP = 0.3f;
	static constexpr float HEAVY_GROUND_KNOCKBACK_LERP = 0.2f;
	static constexpr float LAUNCH_START_KNOCKBACK_LERP = 0.5f;
	static constexpr float LAUNCH_CONTINUOUS_KNOCKBACK_LERP = 0.3f;

	// 指定位置へ水平方向に寄せる
	void MoveTowardHorizontally(const DirectX::XMFLOAT3& target, float lerpRate)
	{
		const DirectX::XMFLOAT3& position = owner->GetPosition();
		owner->SetPosition({
			Mathf::Lerp(position.x, target.x, lerpRate),
			position.y,
			Mathf::Lerp(position.z, target.z, lerpRate)
			});
	}

	// 空中で被弾したか
	bool IsAirborneDamage()
	{
		return !owner->IsGround() && owner->GetDistanceFromGround() > AIR_DAMAGE_MIN_HEIGHT;
	}

	// 初期化処理
	void InitializeAnimations()
	{
		if (animationsInitialized) return;

		anims.normalGround = owner->GetModel()->GetAnimationIndex("Hit_Combat_F_Seq_0");
		anims.lightGround = owner->GetModel()->GetAnimationIndex("Hit_Large_Combat_F_Seq_0");
		anims.heavyGround = owner->GetModel()->GetAnimationIndex("Hit_Large_Combat_Death_Seq_0");
		anims.airStart = owner->GetModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_Start_Seq_0");
		anims.airLoop = owner->GetModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_Loop_Seq_0");
		anims.airEnd = owner->GetModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_End_Seq_0");
		anims.getUp = owner->GetModel()->GetAnimationIndex("Get_Up_Combat_Seq_0");

		animationsInitialized = true;
	}

	// ダメージタイプ判定
	EnemyDamageType GetCurrentDamageType()
	{
		if (owner->IsLaunchKnockbackDamage())
			return EnemyDamageType::Launch;
		else if (owner->IsHeavyKnockbackDamage())
			return EnemyDamageType::Heavy;
		else if (owner->IsLightKnockbackDamage())
			return EnemyDamageType::Light;
		else
			return EnemyDamageType::Normal;
	}

	// ダメージタイプ別の初期処理
	void HandleDamageStart(EnemyDamageType type, float elapsedTime);

	// ノックバック位置の取得
	DirectX::XMFLOAT3 GetKnockbackPosition(EnemyDamageType type)
	{
		switch (type)
		{
		case EnemyDamageType::Normal:
			return Player::Instance().knockbackPosition;
		case EnemyDamageType::Light:
			return Player::Instance().lightKnockbackPosition;
		case EnemyDamageType::Heavy:
			return Player::Instance().heavyKnockbackPosition;
		case EnemyDamageType::Launch:
			return Player::Instance().launchKnockbackPosition;
		default:
			return Player::Instance().knockbackPosition;
		}
	}
};

// 死亡
template <typename ActorType>
class DeadAction : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	DeadAction(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}
	State Run(float elapsedTime);
private:
	// 死亡エフェクトの表示位置の高さと大きさ
	static constexpr float DEATH_EFFECT_OFFSET_Y = 1.0f;
	static constexpr float DEATH_EFFECT_SCALE = 0.5f;

	Effekseer::Handle handle = -1;
};

//-------------------------------------------------------------
// ダメージ
// ダメージタイプ別の初期処理
template <typename ActorType>
void UnifiedDamageAction<ActorType>::HandleDamageStart(EnemyDamageType type, float elapsedTime)
{
	owner->SetTargetPosition(Player::Instance().GetPosition());
	owner->TurnToTarget(elapsedTime, TurnSpeed::INSTANT);

	// ダメージフラグをリセット
	owner->SetDamage(false);
	owner->SetLightKnockbackDamage(false);
	owner->SetHeavyKnockbackDamage(false);
	owner->SetLaunchKnockbackDamage(false);

	DirectX::XMFLOAT3 knockbackPos = GetKnockbackPosition(type);

	switch (type)
	{
	case EnemyDamageType::Normal:
		if (IsAirborneDamage())
		{
			// 空中ダメージ
			owner->SetGravity(AIR_DAMAGE_GRAVITY);
			MoveTowardHorizontally(knockbackPos, NORMAL_AIR_KNOCKBACK_LERP);
			owner->SetVerticalVelocity(AIR_DAMAGE_RISE_VELOCITY);
			this->PlayRootMotion(anims.airStart, false, this->ACTION_BLEND_SECONDS);
		}
		else
		{
			// 地上ダメージ
			MoveTowardHorizontally(knockbackPos, GROUND_KNOCKBACK_LERP);
			this->PlayRootMotion(anims.normalGround, false);
		}
		break;

	case EnemyDamageType::Light:
		if (IsAirborneDamage())
		{
			owner->SetGravity(AIR_DAMAGE_GRAVITY);
			MoveTowardHorizontally(knockbackPos, LIGHT_AIR_KNOCKBACK_LERP);
			owner->SetVerticalVelocity(AIR_DAMAGE_RISE_VELOCITY);
			this->PlayRootMotion(anims.airStart, false, this->ACTION_BLEND_SECONDS);
		}
		else
		{
			MoveTowardHorizontally(knockbackPos, GROUND_KNOCKBACK_LERP);
			this->PlayRootMotion(anims.lightGround, false);
		}
		break;

	case EnemyDamageType::Heavy:
		if (IsAirborneDamage())
		{
			owner->SetGravity(AIR_HEAVY_DAMAGE_GRAVITY);
			MoveTowardHorizontally(knockbackPos, HEAVY_AIR_KNOCKBACK_LERP);
		}
		else
		{
			MoveTowardHorizontally(knockbackPos, HEAVY_GROUND_KNOCKBACK_LERP);
			owner->SetVerticalVelocity(GROUND_HEAVY_DAMAGE_RISE_VELOCITY);
		}
		this->PlayRootMotion(anims.heavyGround, false);
		break;

	case EnemyDamageType::Launch:
		// 打ち上げアニメーション再生
		this->PlayRootMotion(anims.airStart, false, LAUNCH_BLEND_SECONDS);

		// 水平方向の位置を補間で移動
		MoveTowardHorizontally(knockbackPos, LAUNCH_START_KNOCKBACK_LERP);

		// 目標高度までの距離を計算
		float targetHeight = knockbackPos.y;
		float currentHeight = owner->GetPosition().y;
		float heightDiff = targetHeight - currentHeight;

		// 打ち上げに必要な初速度を計算
		float launchVelocity = sqrtf(LAUNCH_VELOCITY_COEFFICIENT * fabsf(owner->GetGravity()) * heightDiff);
		owner->SetVerticalVelocity(launchVelocity);
		break;
	}
}

// メイン処理
template <typename ActorType>
typename EnemyActionBase<ActorType>::State UnifiedDamageAction<ActorType>::Run(float elapsedTime)
{
	// アニメーション初期化
	InitializeAnimations();

	switch (step)
	{
	case 0: // ダメージ開始
		currentDamageType = GetCurrentDamageType();
		HandleDamageStart(currentDamageType, elapsedTime);
		step++;
		break;

	case 1: // アニメーション再生中
		// Normal/Lightダメージで地上アニメーションが終了した場合
		if ((currentDamageType == EnemyDamageType::Normal || currentDamageType == EnemyDamageType::Light) &&
			owner->IsGround() &&
			!owner->GetModel()->IsPlayAnimation())
		{
			owner->SetGravity(Character::DEFAULT_GRAVITY);
			step = 0;
			return State::Complete;
		}

		// Heavyダメージで地上着地した場合、起き上がりへ
		if (currentDamageType == EnemyDamageType::Heavy &&
			owner->IsGround() &&
			!owner->GetModel()->IsPlayAnimation())
		{
			owner->SetVerticalVelocity(0.0f);
			this->PlayRootMotion(anims.getUp, false);
			step = 4; // 起き上がりステップへ
			break;
		}

		// 空中からのダメージでアニメーションが終了
		if (!owner->IsGround() && !owner->GetModel()->IsPlayAnimation())
		{
			this->PlayRootMotion(anims.airLoop, true, this->ACTION_BLEND_SECONDS);
			step++;
		}

		// Launchダメージの場合の特別処理
		if (currentDamageType == EnemyDamageType::Launch)
		{
			// 水平方向の位置を継続的に補間
			MoveTowardHorizontally(Player::Instance().launchKnockbackPosition, LAUNCH_CONTINUOUS_KNOCKBACK_LERP);

			// 最高高度に到達したか確認
			if (owner->GetVelocity().y <= 0.0f)
			{
				owner->SetVerticalVelocity(0.0f);
				owner->SetGravity(0.0f);
				pauseTimer = LAUNCH_APEX_PAUSE_SECONDS;
				owner->SetRunTimer(pauseTimer);
				step = 5; // 打ち上げ専用ステップへ
			}

			// アニメーション終了でループに切り替え
			if (!owner->GetModel()->IsPlayAnimation())
			{
				this->PlayRootMotion(anims.airLoop, true, LAUNCH_BLEND_SECONDS);
			}
		}
		break;

	case 2: // 空中ループ中（着地待ち）
		if (owner->IsGround())
		{
			this->PlayRootMotion(anims.airEnd, false, this->ACTION_BLEND_SECONDS);
			step++;
		}
		break;

	case 3: // 着地アニメーション
		if (!owner->GetModel()->IsPlayAnimation())
		{
			this->PlayRootMotion(anims.getUp, false, this->ACTION_BLEND_SECONDS);
			step++;
		}
		break;

	case 4: // 起き上がりアニメーション
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step = 0;
			owner->SetGravity(Character::DEFAULT_GRAVITY);
			return State::Complete;
		}
		break;

	case 5: // 打ち上げ専用：最高高度で一時停止
		pauseTimer -= elapsedTime;

		if (pauseTimer >= 0.0f)
		{
			owner->SetGravity(Character::DEFAULT_GRAVITY);
			this->PlayRootMotion(anims.airLoop, true, LAUNCH_BLEND_SECONDS);
			step = 2; // 落下処理へ
		}
		break;
	}

	// ダメージを受けた場合は即座に遷移
	if (owner->IsAnyDamage() || owner->IsRevenge() || owner->IsDeathFlag())
	{
		owner->SetGravity(Character::DEFAULT_GRAVITY);
		owner->SetVerticalVelocity(0.0f);
		step = 0;
		return State::Complete;
	}

	return State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 死亡
template <typename ActorType>
typename EnemyActionBase<ActorType>::State DeadAction<ActorType>::Run(float elapsedTime)
{
	const DirectX::XMFLOAT3& position = owner->GetPosition();
	const DirectX::XMFLOAT3 effectPosition = { position.x, position.y + DEATH_EFFECT_OFFSET_Y, position.z };
	if (handle == -1)
	{
		handle = owner->deathEffect->Play(effectPosition, DEATH_EFFECT_SCALE);
	}
	owner->deathEffect->SetPosition(handle, effectPosition);
	// 実行中を返す
	return State::Run;
}
//-------------------------------------------------------------