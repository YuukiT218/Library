#pragma once
#include "ActionBase.h"

#include "Character\Player.h"
#include "Math\Mathf.h"

//-------------------------------------------------------------
// 行動処理クラス宣言
//-------------------------------------------------------------
// 斬撃コンボ1
template <typename ActorType>
class SlashCombo1Action : public ActionBase<ActorType>
{
public:
	SlashCombo1Action(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Combo_Attack_03_All_Seq_0");
};

// 斬撃コンボ2
template <typename ActorType>
class SlashCombo2Action : public ActionBase<ActorType>
{
public:
	SlashCombo2Action(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Combo_Attack_01_All_Seq_0");
};

// 突進斬り
template <typename ActorType>
class DashSlashAction : public ActionBase<ActorType>
{
public:
	DashSlashAction(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Run_Attack_01_Seq_0");
};

// 三連テレポート
template <typename ActorType>
class TripleTeleportAction : public ActionBase<ActorType>
{
public:
	TripleTeleportAction(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0");
};

// 様子見歩き
template <typename ActorType>
class CautiousWalkAction : public ActionBase<ActorType>
{
public:
	CautiousWalkAction(ActorType*actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Walk_Combat_Loop_F_L_90_RM_Seq_0");
};

// 追跡行動
template <typename ActorType>
class PursuitAction : public ActionBase<ActorType>
{
public:
	PursuitAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Run_Combat_Loop_F_0_Seq_0");
};

// 徘徊行動
template <typename ActorType>
class WanderAction : public ActionBase<ActorType>
{
public:
	WanderAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Walk_Combat_Loop_F_0_RM_Seq_0");
};

// 待機行動
template <typename ActorType>
class IdleAction : public ActionBase<ActorType>
{
public:
	IdleAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Idle_Combat_Seq_0");
};

// ダメージ
template <typename ActorType>
class DamageAction : public ActionBase<ActorType>
{
public:
	DamageAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Hit_Combat_F_Seq_0");
	int airAnimationIndex = owner->GetModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_Start_Seq_0");
	int airLoopAnimationIndex = owner->GetModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_Loop_Seq_0");
};

template <typename ActorType>
class LightDamageAction : public ActionBase<ActorType>
{
public:
	LightDamageAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Hit_Large_Combat_F_Seq_0");
	int airAnimationIndex = owner->GetModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_Start_Seq_0");
	int airLoopAnimationIndex = owner->GetModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_Loop_Seq_0");
};

template <typename ActorType>
class HeavyDamageAction : public ActionBase<ActorType>
{
public:
	HeavyDamageAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Hit_Large_Combat_Death_Seq_0");
	int getupAnimationIndex = owner->GetModel()->GetAnimationIndex("Get_Up_Combat_Seq_0");
};

template <typename ActorType>
class LaunchDamageAction : public ActionBase<ActorType>
{
public:
	LaunchDamageAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_Start_Seq_0");
	int fallAnimationIndex = owner->GetModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_Loop_Seq_0");
};

// 死亡
template <typename ActorType>
class DeadAction : public ActionBase<ActorType>
{
public:
	DeadAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
};

// ステートマシン実行用
template <typename ActorType>
class StateMachineAction : public ActionBase<ActorType>
{
public:
	StateMachineAction(ActorType* actor, StateBase<ActorType>* stateMachine)
		: ActionBase<ActorType>(actor), stateMachine(stateMachine) {}

	ActionBase<ActorType>::State Run(float elapsedTime) override {
		if (!entered) {
			stateMachine->Enter();
			entered = true;
		}
		stateMachine->Execute(elapsedTime);
		if (stateMachine->GetState()->IsComplete())
		{
			entered = false;
			return ActionBase<ActorType>::State::Complete;
		}
		if (stateMachine->GetState()->IsFailed())
		{
			entered = false;
			return ActionBase<ActorType>::State::Failed;
		}
		return ActionBase<ActorType>::State::Run;
	}
private:
	StateBase<ActorType>* stateMachine;
	bool entered = false;
};
//-------------------------------------------------------------

//-------------------------------------------------------------
//  行動処理クラス処理部分
//-------------------------------------------------------------
//-------------------------------------------------------------
// 斬撃コンボ1
template <typename ActorType>
typename ActionBase<ActorType>::State SlashCombo1Action<ActorType>::Run(float elapsedTime)
{
	// 攻撃対象設定
	owner->SetTargetPosition(Player::Instance().GetPosition());
	switch (step)
	{
	case 0:
		// 目的地点へ移動
		{
			float vx = owner->GetTargetPosition().x - owner->GetPosition().x;
			float vz = owner->GetTargetPosition().z - owner->GetPosition().z;
			owner->TurnToTarget(elapsedTime, vx, vz, DirectX::XMConvertToRadians(360) * 3.0f);
			if (owner->IsTurnToTarget(vx, vz))
			{
				owner->isTurnAnimation = false;
				step++;
			}
		}
		break;
	case 1:
		// アニメーション再生
		owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
		owner->SetAttackFlg(true);
		step++;
		break;
	case 2:
		owner->MoveToTarget(elapsedTime, 0.1f);
		if (!owner->isPlayerInvincible)
		{
			owner->AttackAnimationCollision({ owner->rightArmNodeHitSpheres }, owner->attackFrameMin[0], owner->attackFrameMax[0], owner->attackDamage[0]);
			owner->GetSword()->AttackAnimationCollision(owner->GetModel(), owner->attackFrameMin[0], owner->attackFrameMax[0], owner->attackDamage[0], 0, 0, 0, 0, 0);
		}
		// アニメーションが終了しているとき
		if (!owner->GetModel()->IsPlayAnimation())
		{
			owner->isPlayerInvincible = false;
			owner->SetAttackFlg(false);

			step = 0;
			// 攻撃成功を返す
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsAnyDamage())
	{
		step = 0;
		return ActionBase<ActorType>::State::Failed;
	}
	if (owner->GetHealth() <= 0)
	{
		return ActionBase<ActorType>::State::Failed;
	}
	// 実行中を返す
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 斬撃コンボ2
template <typename ActorType>
typename ActionBase<ActorType>::State SlashCombo2Action<ActorType>::Run(float elapsedTime)
{
	// 攻撃対象設定
	owner->SetTargetPosition(Player::Instance().GetPosition());
	switch (step)
	{
	case 0:
		// 目的地点へ移動
		{
			float vx = owner->GetTargetPosition().x - owner->GetPosition().x;
			float vz = owner->GetTargetPosition().z - owner->GetPosition().z;
			owner->TurnToTarget(elapsedTime, vx, vz, DirectX::XMConvertToRadians(360) * 3.0f);
			if (owner->IsTurnToTarget(vx, vz))
			{
				owner->isTurnAnimation = false;
				step++;
			}
		}
		break;
	case 1:
		// アニメーション再生
		owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
		owner->SetAttackFlg(true);
		step++;
		break;
	case 2:
		owner->MoveToTarget(elapsedTime, 0.1f);
		if (!owner->isPlayerInvincible)
		{
			owner->AttackAnimationCollision({ owner->rightArmNodeHitSpheres }, owner->attackFrameMin[0], owner->attackFrameMax[0], owner->attackDamage[0]);
		}
		// アニメーションが終了しているとき
		if (!owner->GetModel()->IsPlayAnimation())
		{
			owner->isPlayerInvincible = false;
			step = 0;
			// 攻撃成功を返す
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsAnyDamage())
	{
		step = 0;
		return ActionBase<ActorType>::State::Failed;
	}
	// 実行中を返す
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 突進斬り
template<typename ActorType>
typename ActionBase<ActorType>::State DashSlashAction<ActorType>::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		// 攻撃対象設定
		owner->SetTargetPosition(Player::Instance().GetPosition());
		// アニメーション再生
		owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
		owner->SetAttackFlg(true);
		step++;
		break;
	case 1:
		owner->MoveToTarget(elapsedTime, 0.1f);
		if (!owner->isPlayerInvincible)
		{
			owner->AttackAnimationCollision({ owner->rightArmNodeHitSpheres }, owner->attackFrameMin[0], owner->attackFrameMax[0], owner->attackDamage[0]);
		}
		// アニメーションが終了しているとき
		if (!owner->GetModel()->IsPlayAnimation())
		{
			owner->isPlayerInvincible = false;
			step = 0;
			// 攻撃成功を返す
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsAnyDamage())
	{
		step = 0;
		return ActionBase<ActorType>::State::Failed;
	}
	// 実行中を返す
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 三連テレポート
template <typename ActorType>
typename ActionBase<ActorType>::State TripleTeleportAction<ActorType>::Run(float elapsedTime)
{
	float runTimer = owner->GetRunTimer();
	DirectX::XMFLOAT3 vec;
	DirectX::XMVECTOR toTargetVec = {};
	DirectX::XMVECTOR sideVec = {};
	Model::Node* Node = owner->GetModel()->FindNode("pelvis");
	DirectX::XMFLOAT3 targetPosition = Player::Instance().GetPosition();
	DirectX::XMFLOAT3 position = owner->GetPosition();

	switch (step)
	{
	case 0: // 左へ切り返し
		// 左方向ベクトル
		sideVec = { -Node->worldTransform._11, -Node->worldTransform._12, -Node->worldTransform._13 };
		// プレイヤー方向ベクトル
		toTargetVec = { (targetPosition.x - position.x) / 3, position.y, (targetPosition.z - position.z) / 3 };
		DirectX::XMStoreFloat3(&vec, DirectX::XMVectorAdd(toTargetVec, sideVec));
		owner->SetPosition(vec); 
		owner->SetRunTimer(0.7f); // 切り返し時間
		step++;
		break;
	case 1: // 右へ切り返し
		runTimer = owner->GetRunTimer() - elapsedTime;
		owner->SetRunTimer(runTimer);
		if (runTimer <= 0.0f)
		{
			sideVec = { Node->worldTransform._11, Node->worldTransform._12, Node->worldTransform._13 };
			toTargetVec = { (targetPosition.x - position.x) / 3, position.y, (targetPosition.z - position.z) / 3 };
			DirectX::XMStoreFloat3(&vec, DirectX::XMVectorAdd(toTargetVec, sideVec));
			//owner->SetMovement(vec, 15.0f);
			owner->SetPosition(vec);
			owner->SetRunTimer(0.7f);
			step++;
		}
		break;
	case 2: // 最後にプレイヤー方向へ
		runTimer = owner->GetRunTimer() - elapsedTime;
		owner->SetRunTimer(runTimer);
		if (runTimer <= 0.0f)
		{
			toTargetVec = { (targetPosition.x - position.x), position.y, (targetPosition.z - position.z) };
			DirectX::XMStoreFloat3(&vec, toTargetVec);
			//owner->SetMovement(vec, 15.0f);
			owner->SetPosition(vec);
			owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
			step++;
		}
		break;
	case 3:
		if (!owner->GetModel()->IsPlayAnimation())
		{
			owner->isPlayerInvincible = false;
			step = 0;
			// 攻撃成功を返す
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 様子見歩き
template <typename ActorType>
typename ActionBase<ActorType>::State CautiousWalkAction<ActorType>::Run(float elapsedTime)
{
	float runTimer;
	switch (step)
	{
	case 0:
		owner->GetModel()->PlayRootMotion(animationIndex, true, true, owner->GetBlendSeconds(), "root");
		owner->SetRunTimer(Mathf::RandomRange(2.0f, 4.0f));
		step++;
		break;
	case 1:
		runTimer = owner->GetRunTimer() - elapsedTime;
		owner->SetRunTimer(runTimer);
		if (runTimer <= 0.0f)
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 徘徊行動
template <typename ActorType>
typename ActionBase<ActorType>::State WanderAction<ActorType>::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		// 徘徊モーション設定
		owner->GetModel()->PlayRootMotion(animationIndex, true, true, owner->GetBlendSeconds(), "root");
		step++;
		break;
	case 1:
		if (owner->GetHealth() <= 0)
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
		// 目的地点までのXZ平面での距離判定
		DirectX::XMFLOAT3 position = owner->GetPosition();
		DirectX::XMFLOAT3 targetPosition = owner->GetTargetPosition();
		float vx = targetPosition.x - position.x;
		float vz = targetPosition.z - position.z;
		float distSq = vx * vx + vz * vz;

		// 目的地へ着いた
		float radius = owner->GetRadius();
		if (distSq < radius * radius)
		{
			step = 0;
			// 徘徊成功を返す
			return ActionBase<ActorType>::State::Complete;
		}

		// 目的地点へ移動
		owner->MoveToTarget(elapsedTime, 0.1f);

		// プレイヤー索敵成功したら
		if (owner->SearchPlayer())
		{
			step = 0;
			// 徘徊成功を返す
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsAnyDamage())
	{
		step = 0;
		return ActionBase<ActorType>::State::Failed;
	}
	// 実行中を返す
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 追跡行動
template <typename ActorType>
typename ActionBase<ActorType>::State PursuitAction<ActorType>::Run(float elapsedTime)
{
	float runTimer = owner->GetRunTimer();
	switch (step)
	{
	case 0:
		// 目標地点をプレイヤー位置に設定
		owner->SetTargetPosition(Player::Instance().GetPosition());
		owner->SetRunTimer(Mathf::RandomRange(2.0f, 3.0f));
		owner->GetModel()->PlayAnimation(animationIndex, true, owner->GetBlendSeconds());
		step++;
		break;
	case 1:
		if (owner->GetHealth() <= 0)
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
		runTimer -= elapsedTime;
		// タイマー更新
		owner->SetRunTimer(runTimer);
		// 目標地点をプレイヤー位置に設定
		owner->SetTargetPosition(Player::Instance().GetPosition());
		// 目的地点へ移動
		owner->MoveToTarget(elapsedTime, owner->GetMoveSpeed());

		// プレイヤーとの距離を計算
		DirectX::XMFLOAT3 position = owner->GetPosition();
		DirectX::XMFLOAT3 targetPosition = owner->GetTargetPosition();

		float vx = targetPosition.x - position.x;
		float vy = targetPosition.y - position.y;
		float vz = targetPosition.z - position.z;
		float dist = sqrtf(vx * vx + vy * vy + vz * vz);
		// 攻撃範囲にいるとき
		if (dist < owner->GetAttackRange())
		{
			step = 0;
			// 追跡成功を返す
			return ActionBase<ActorType>::State::Complete;
		}
		// 行動時間が過ぎた時
		if (runTimer <= 0.0f)
		{
			step = 0;
			// 追跡失敗を返す
			return ActionBase<ActorType>::State::Failed;
		}
		break;
	}
	if (owner->IsAnyDamage())
	{
		step = 0;
		return ActionBase<ActorType>::State::Failed;
	}
	// 実行中を返す
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 待機行動
template <typename ActorType>
typename ActionBase<ActorType>::State IdleAction<ActorType>::Run(float elapsedTime)
{
	float runTimer = owner->GetRunTimer();
	switch (step)
	{
	case 0:
		owner->SetRunTimer(Mathf::RandomRange(3.0f, 5.0f));
		if (owner->GetHealth() < (owner->GetMaxHealth() * 0.3))
		{
			owner->GetModel()->PlayAnimation(animationIndex, true, owner->GetBlendSeconds());
		}
		else
		{
			owner->GetModel()->PlayAnimation(animationIndex, true, owner->GetBlendSeconds());
		}
		step++;
		break;
	case 1:
		runTimer -= elapsedTime;
		// タイマー更新
		owner->SetRunTimer(runTimer);

		// 待機時間が過ぎた時
		if (runTimer <= 0.0f)
		{
			owner->SetRandomTargetPosition();
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}

		// プレイヤーを見つけた時
		if (owner->SearchPlayer())
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsDamage())
	{
		step = 0;
		return ActionBase<ActorType>::State::Failed;
	}
	// 実行中を返す
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// ダメージ
template <typename ActorType>
typename ActionBase<ActorType>::State DamageAction<ActorType>::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		owner->SetTargetPosition(Player::Instance().GetPosition());
		owner->MoveToTarget(elapsedTime, 0);
		owner->SetDamage(false);

		// 空中で攻撃を受けた場合の処理
		if (!owner->IsGround())
		{
			owner->SetGravity(-0.15f);

			// 空中ヒット用の軽い吹き飛び
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, Player::Instance().knockbackPosition.x, 0.15f),
				owner->GetPosition().y,
				Mathf::Lerp(owner->GetPosition().z, Player::Instance().knockbackPosition.z, 0.15f)
				});

			owner->SetVerticalVelocity(2.0f);

			// 空中ダメージアニメーション（あれば）
			owner->GetModel()->PlayRootMotion(airAnimationIndex, false, true, 0.2f, "root");
		}
		else
		{
			// 地上ヒット時の通常処理
			owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, Player::Instance().knockbackPosition.x, 0.25f),
				Mathf::Lerp(owner->GetPosition().y, Player::Instance().knockbackPosition.y, 0.25f),
				Mathf::Lerp(owner->GetPosition().z, Player::Instance().knockbackPosition.z, 0.25f)
				});
		}
		step++;
		break;

	case 1:
		// 空中コンボ判定：より強力な攻撃を受けたら即座に遷移
		if (owner->IsLightKbDamage() || owner->IsHeavyKbDamage() || owner->IsLaunchKbDamage())
		{
			owner->SetGravity(-0.3f);
			owner->SetVerticalVelocity(0.0f);
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}

		// 通常ダメージを再度受けた場合
		if (owner->IsDamage())
		{
			owner->SetGravity(-0.3f);
			owner->SetVerticalVelocity(0.0f);
			step = 0;
			return ActionBase<ActorType>::State::Run;
		}

		// アニメーション終了で完了
		if (owner->IsGround() && !owner->GetModel()->IsPlayAnimation())
		{
			owner->SetGravity(-0.3f);
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	return ActionBase<ActorType>::State::Run;
}

template <typename ActorType>
typename ActionBase<ActorType>::State LightDamageAction<ActorType>::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		owner->SetTargetPosition(Player::Instance().GetPosition());
		owner->MoveToTarget(elapsedTime, 0);
		owner->SetLightKbDamage(false);

		// 空中で攻撃を受けた場合の処理
		if (!owner->IsGround())
		{
			owner->SetGravity(-0.15f);

			// 空中ヒット：横方向の吹き飛びを強化
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, Player::Instance().lightKnockbackPosition.x, 0.4f),
				owner->GetPosition().y,
				Mathf::Lerp(owner->GetPosition().z, Player::Instance().lightKnockbackPosition.z, 0.4f)
				});

			owner->SetVerticalVelocity(2.0f);

			owner->GetModel()->PlayRootMotion(airAnimationIndex, false, true, 0.2f, "root");
		}
		else
		{
			// 地上ヒット：通常の吹き飛び
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, Player::Instance().lightKnockbackPosition.x, 0.25f),
				Mathf::Lerp(owner->GetPosition().y, Player::Instance().lightKnockbackPosition.y, 0.25f),
				Mathf::Lerp(owner->GetPosition().z, Player::Instance().lightKnockbackPosition.z, 0.25f)
				});
			owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
		}

		step++;
		break;
	case 1:
		// より強力な攻撃を受けたら即座に遷移
		if (owner->IsDamage() || owner->IsHeavyKbDamage() || owner->IsLaunchKbDamage())
		{
			owner->SetGravity(-0.3f);
			owner->SetVerticalVelocity(0.0f);
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}

		if (owner->IsLightKbDamage())
		{
			owner->SetGravity(-0.3f);
			owner->SetVerticalVelocity(0.0f);
			step = 0;
			return ActionBase<ActorType>::State::Run;
		}

		// アニメーション終了 or 着地で完了
		if (!owner->GetModel()->IsPlayAnimation() && owner->IsGround())
		{
			owner->SetGravity(-0.3f);
			step = 0;
			if (owner->IsGround())
			{
				owner->SetVerticalVelocity(0.0f);
			}
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	return ActionBase<ActorType>::State::Run;
}

template <typename ActorType>
typename ActionBase<ActorType>::State HeavyDamageAction<ActorType>::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		owner->SetTargetPosition(Player::Instance().GetPosition());
		owner->MoveToTarget(elapsedTime, 0);
		owner->SetHeavyKbDamage(false);

		// 空中で攻撃を受けた場合の処理
		if (!owner->IsGround())
		{
			owner->SetGravity(-0.2f);

			// 空中ヒット：大きく横方向に吹き飛ばす
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, Player::Instance().heavyKnockbackPosition.x, 0.6f),
				owner->GetPosition().y,
				Mathf::Lerp(owner->GetPosition().z, Player::Instance().heavyKnockbackPosition.z, 0.6f)
				});
		}
		else
		{
			// 地上ヒット：大きく吹き飛ばす
			owner->SetPosition({
				Mathf::Lerp(owner->GetPosition().x, Player::Instance().heavyKnockbackPosition.x, 0.4f),
				Mathf::Lerp(owner->GetPosition().y, Player::Instance().heavyKnockbackPosition.y, 0.4f),
				Mathf::Lerp(owner->GetPosition().z, Player::Instance().heavyKnockbackPosition.z, 0.4f)
				});

			// 地上から中程度に浮かせる
			owner->SetVerticalVelocity(1.5f);
		}

		owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
		step++;
		break;

	case 1:
		// Launch攻撃を受けたら即座に遷移
		if (owner->IsAnyDamage())
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}

		// アニメーション終了 or 着地で完了
		if (!owner->GetModel()->IsPlayAnimation() && owner->IsGround())
		{
			owner->SetGravity(-0.3f);
			step = 0;
			if (owner->IsGround())
			{
				owner->SetVerticalVelocity(0.0f);
			}
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	return ActionBase<ActorType>::State::Run;
}

template <typename ActorType>
typename ActionBase<ActorType>::State LaunchDamageAction<ActorType>::Run(float elapsedTime)
{
	float targetHeight;
	float currentHeight;
	float heightDiff;
	float launchVelocity;
	float pauseTimer = owner->GetRunTimer();

	switch (step)
	{
	case 0: // 打ち上げ開始
		owner->SetTargetPosition(Player::Instance().GetPosition());
		owner->MoveToTarget(elapsedTime, 0);

		// 打ち上げアニメーション再生
		owner->GetModel()->PlayRootMotion(animationIndex, false, true, 0.1f, "root");

		// 水平方向の位置を補間で移動
		owner->SetPosition({
			Mathf::Lerp(owner->GetPosition().x, Player::Instance().launchKnockbackPosition.x, 0.5f),
			owner->GetPosition().y,
			Mathf::Lerp(owner->GetPosition().z, Player::Instance().launchKnockbackPosition.z, 0.5f)
			});

		// 目標高度までの距離を計算
		targetHeight = Player::Instance().launchKnockbackPosition.y;
		currentHeight = owner->GetPosition().y;
		heightDiff = targetHeight - currentHeight;

		// 打ち上げに必要な初速度を計算（重力を考慮）
		// v = sqrt(2 * g * h)の式を使用
		launchVelocity = sqrtf(100.0f * fabsf(owner->GetGravity()) * heightDiff);
		owner->SetVerticalVelocity(launchVelocity);

		step++;
		break;

	case 1: // 上昇中
		// 水平方向の位置を継続的に補間
		owner->SetPosition({
			Mathf::Lerp(owner->GetPosition().x, Player::Instance().launchKnockbackPosition.x, 0.3f),
			owner->GetPosition().y,
			Mathf::Lerp(owner->GetPosition().z, Player::Instance().launchKnockbackPosition.z, 0.3f)
			});

		// 最高高度に到達したか確認（速度が0以下になったとき）
		if (owner->GetVelocity().y <= 0.0f)
		{
			// 最高高度で一瞬停止
			owner->SetVerticalVelocity(0.0f);
			owner->SetGravity(0.0f);  // 重力を一時的に無効化
			owner->SetRunTimer(0.3f);  // 0.3秒間停止
			step++;
		}

		// アニメーションが終わったらループアニメーションに切り替え
		if (!owner->GetModel()->IsPlayAnimation())
		{
			owner->GetModel()->PlayRootMotion(fallAnimationIndex, true, true, 0.1f, "root");
		}
		break;

	case 2: // 最高高度で一時停止
		pauseTimer -= elapsedTime;
		owner->SetRunTimer(pauseTimer);

		if (pauseTimer <= 0.0f)
		{
			// 重力を再度有効化して落下開始
			owner->SetGravity(-0.3f);  // 元の重力値に戻す
			owner->GetModel()->PlayRootMotion(fallAnimationIndex, true, true, 0.1f, "root");
			step++;
		}
		break;

	case 3: // 落下中
		// 地面に着地したか、他のダメージを受けたら終了
		if (owner->IsGround() || owner->IsDamage() || owner->IsLightKbDamage() || owner->IsHeavyKbDamage())
		{
			step = 0;
			owner->SetLaunchKbDamage(false);
			owner->SetVerticalVelocity(0.0f);
			owner->SetGravity(-0.3f);  // 重力を確実に元に戻す
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsDamage() || owner->IsLightKbDamage() || owner->IsHeavyKbDamage())
	{
		step = 0;
		owner->SetLaunchKbDamage(false);
		owner->SetVerticalVelocity(0.0f);
		return ActionBase<ActorType>::State::Complete;
	}
	// 実行中を返す
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 死亡
template <typename ActorType>
typename ActionBase<ActorType>::State DeadAction<ActorType>::Run(float elapsedTime)
{
	if (owner->GetModel()->GetCurrentAnimationSeconds() > 4.45f && owner->GetModel()->GetCurrentAnimationSeconds() < 4.46f)
	{
		//owner->DefeatedSE->Play(false, 0.2f);
	}
	// 実行中を返す
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------