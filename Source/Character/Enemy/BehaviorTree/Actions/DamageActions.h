#pragma once
#include "EnemyActionBase.h"
#include "Character/Player/Player.h"
#include "Math\Mathf.h"

// ダメージの種類を定義
enum class DamageType
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

	DamageType currentDamageType = DamageType::Normal;
	bool animationsInitialized = false;
	float pauseTimer = 0.0f;

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
	DamageType GetCurrentDamageType()
	{
		if (owner->IsLaunchKbDamage())
			return DamageType::Launch;
		else if (owner->IsHeavyKbDamage())
			return DamageType::Heavy;
		else if (owner->IsLightKbDamage())
			return DamageType::Light;
		else
			return DamageType::Normal;
	}

	// ダメージタイプ別の初期処理
	void HandleDamageStart(DamageType type, float elapsedTime);

	// ノックバック位置の取得
	DirectX::XMFLOAT3 GetKnockbackPosition(DamageType type)
	{
		switch (type)
		{
		case DamageType::Normal:
			return Player::Instance().knockbackPosition;
		case DamageType::Light:
			return Player::Instance().lightKnockbackPosition;
		case DamageType::Heavy:
			return Player::Instance().heavyKnockbackPosition;
		case DamageType::Launch:
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
	Effekseer::Handle handle = -1;
};

//-------------------------------------------------------------
// ダメージ
// ダメージタイプ別の初期処理
template <typename ActorType>
void UnifiedDamageAction<ActorType>::HandleDamageStart(DamageType type, float elapsedTime)
{
	owner->SetTargetPosition(Player::Instance().GetPosition());
	owner->TurnToTarget(elapsedTime, TurnSpeed::INSTANT);

	// ダメージフラグをリセット
	owner->SetDamage(false);
	owner->SetLightKbDamage(false);
	owner->SetHeavyKbDamage(false);
	owner->SetLaunchKbDamage(false);

	DirectX::XMFLOAT3 knockbackPos = GetKnockbackPosition(type);

	switch (type)
	{
	case DamageType::Normal:
		if (!owner->IsGround() && owner->GetDistanceFromGround() > 0.5f)
		{
			// 空中ダメージ
			owner->SetGravity(-0.15f);
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, knockbackPos.x, 0.15f),
				owner->GetPosition().y,
				Mathf::Lerp(owner->GetPosition().z, knockbackPos.z, 0.15f)
				});
			owner->SetVerticalVelocity(2.0f);
			owner->GetModel()->PlayRootMotion(anims.airStart, false, true, 0.2f, "root");
		}
		else
		{
			// 地上ダメージ
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, knockbackPos.x, 0.25f),
				owner->GetPosition().y,
				Mathf::Lerp(owner->GetPosition().z, knockbackPos.z, 0.25f)
				});
			owner->GetModel()->PlayRootMotion(anims.normalGround, false, true, owner->GetBlendSeconds(), "root");
		}
		break;

	case DamageType::Light:
		if (!owner->IsGround() && owner->GetDistanceFromGround() > 0.5f)
		{
			owner->SetGravity(-0.15f);
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, knockbackPos.x, 0.4f),
				owner->GetPosition().y,
				Mathf::Lerp(owner->GetPosition().z, knockbackPos.z, 0.4f)
				});
			owner->SetVerticalVelocity(2.0f);
			owner->GetModel()->PlayRootMotion(anims.airStart, false, true, 0.2f, "root");
		}
		else
		{
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, knockbackPos.x, 0.25f),
				owner->GetPosition().y,
				Mathf::Lerp(owner->GetPosition().z, knockbackPos.z, 0.25f)
				});
			owner->GetModel()->PlayRootMotion(anims.lightGround, false, true, owner->GetBlendSeconds(), "root");
		}
		break;

	case DamageType::Heavy:
		if (!owner->IsGround() && owner->GetDistanceFromGround() > 0.5f)
		{
			owner->SetGravity(-0.2f);
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, knockbackPos.x, 0.3f),
				owner->GetPosition().y,
				Mathf::Lerp(owner->GetPosition().z, knockbackPos.z, 0.3f)
				});
		}
		else
		{
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, knockbackPos.x, 0.2f),
				owner->GetPosition().y,
				Mathf::Lerp(owner->GetPosition().z, knockbackPos.z, 0.2f)
				});
			owner->SetVerticalVelocity(1.5f);
		}
		owner->GetModel()->PlayRootMotion(anims.heavyGround, false, true, owner->GetBlendSeconds(), "root");
		break;

	case DamageType::Launch:
		// 打ち上げアニメーション再生
		owner->GetModel()->PlayRootMotion(anims.airStart, false, true, 0.1f, "root");

		// 水平方向の位置を補間で移動
		owner->SetPosition({
			Mathf::Lerp(owner->GetPosition().x, knockbackPos.x, 0.5f),
			owner->GetPosition().y,
			Mathf::Lerp(owner->GetPosition().z, knockbackPos.z, 0.5f)
			});

		// 目標高度までの距離を計算
		float targetHeight = knockbackPos.y;
		float currentHeight = owner->GetPosition().y;
		float heightDiff = targetHeight - currentHeight;

		// 打ち上げに必要な初速度を計算
		float launchVelocity = sqrtf(100.0f * fabsf(owner->GetGravity()) * heightDiff);
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
		if ((currentDamageType == DamageType::Normal || currentDamageType == DamageType::Light) &&
			owner->IsGround() &&
			!owner->GetModel()->IsPlayAnimation())
		{
			owner->SetGravity(-0.3f);
			step = 0;
			return State::Complete;
		}

		// Heavyダメージで地上着地した場合、起き上がりへ
		if (currentDamageType == DamageType::Heavy &&
			owner->IsGround() &&
			!owner->GetModel()->IsPlayAnimation())
		{
			owner->SetVerticalVelocity(0.0f);
			owner->GetModel()->PlayRootMotion(anims.getUp, false, true, owner->GetBlendSeconds(), "root");
			step = 4; // 起き上がりステップへ
			break;
		}

		// 空中からのダメージでアニメーションが終了
		if (!owner->IsGround() && !owner->GetModel()->IsPlayAnimation())
		{
			owner->GetModel()->PlayRootMotion(anims.airLoop, true, true, 0.2f, "root");
			step++;
		}

		// Launchダメージの場合の特別処理
		if (currentDamageType == DamageType::Launch)
		{
			// 水平方向の位置を継続的に補間
			DirectX::XMFLOAT3 launchPos = Player::Instance().launchKnockbackPosition;
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, launchPos.x, 0.3f),
				owner->GetPosition().y,
				Mathf::Lerp(owner->GetPosition().z, launchPos.z, 0.3f)
				});

			// 最高高度に到達したか確認
			if (owner->GetVelocity().y <= 0.0f)
			{
				owner->SetVerticalVelocity(0.0f);
				owner->SetGravity(0.0f);
				pauseTimer = 0.3f;
				owner->SetRunTimer(pauseTimer);
				step = 5; // 打ち上げ専用ステップへ
			}

			// アニメーション終了でループに切り替え
			if (!owner->GetModel()->IsPlayAnimation())
			{
				owner->GetModel()->PlayRootMotion(anims.airLoop, true, true, 0.1f, "root");
			}
		}
		break;

	case 2: // 空中ループ中（着地待ち）
		if (owner->IsGround())
		{
			owner->GetModel()->PlayRootMotion(anims.airEnd, false, true, 0.2f, "root");
			step++;
		}
		break;

	case 3: // 着地アニメーション
		if (!owner->GetModel()->IsPlayAnimation())
		{
			owner->GetModel()->PlayRootMotion(anims.getUp, false, true, 0.2f, "root");
			step++;
		}
		break;

	case 4: // 起き上がりアニメーション
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step = 0;
			owner->SetGravity(-0.3f);
			return State::Complete;
		}
		break;

	case 5: // 打ち上げ専用：最高高度で一時停止
		pauseTimer -= elapsedTime;

		if (pauseTimer >= 0.0f)
		{
			owner->SetGravity(-0.3f);
			owner->GetModel()->PlayRootMotion(anims.airLoop, true, true, 0.1f, "root");
			step = 2; // 落下処理へ
		}
		break;
	}

	// ダメージを受けた場合は即座に遷移
	if (owner->IsAnyDamage() || owner->GetRevengeState() || owner->IsDeathFlag())
	{
		owner->SetGravity(-0.3f);
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
	if (handle == -1)
	{
		handle = owner->deathEffect->Play({ owner->GetPosition().x, owner->GetPosition().y + 1.0f, owner->GetPosition().z }, 0.5f);
	}
	owner->deathEffect->SetPosition(handle, { owner->GetPosition().x, owner->GetPosition().y + 1.0f, owner->GetPosition().z });
	// 実行中を返す
	return State::Run;
}
//-------------------------------------------------------------