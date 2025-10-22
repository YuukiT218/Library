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
	int animationIndex = owner->GetModel()->GetAnimationIndex("Hit_Large_Combat_F_Seq_0");
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
	if (owner->IsDamage())
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
		owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
		step++;
	case 1:
		if (!owner->GetModel()->IsPlayAnimation() || owner->IsDamage())
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
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