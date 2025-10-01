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
};

// ブレス攻撃行動
template <typename ActorType>
class BreathAction : public ActionBase<ActorType>
{
public:
	BreathAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
};

// 薙ぎ払いブレス攻撃行動
template <typename ActorType>
class BreathSweepingAction : public ActionBase<ActorType>
{
public:
	BreathSweepingAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
};

// 咆哮攻撃行動
template <typename ActorType>
class RoarAction : public ActionBase<ActorType>
{
public:
	RoarAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
};

// タックル攻撃行動
template <typename ActorType>
class TackleAction : public ActionBase<ActorType>
{
public:
	TackleAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
};

// サイドステップ行動
template <typename ActorType>
class SidestepAction : public ActionBase<ActorType>
{
public:
	SidestepAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
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

// 逃走行動
template <typename ActorType>
class LeaveAction : public ActionBase<ActorType>
{
public:
	LeaveAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
};

// 回復行動
template <typename ActorType>
class RecoverAction : public ActionBase<ActorType>
{
public:
	RecoverAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
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
		toTargetVec = { (targetPosition.x - position.x) / 3, (targetPosition.y - position.y) / 3, (targetPosition.z - position.z) / 3 };
		DirectX::XMStoreFloat3(&vec, DirectX::XMVector3Normalize(DirectX::XMVectorAdd(toTargetVec, sideVec)));
		owner->SetMovement(vec, 15.0f);
		owner->SetRunTimer(0.7f); // 切り返し時間
		step++;
		break;
	case 1: // 右へ切り返し
		runTimer = owner->GetRunTimer() - elapsedTime;
		owner->SetRunTimer(runTimer);
		if (runTimer <= 0.0f)
		{
			sideVec = { Node->worldTransform._11, Node->worldTransform._12, Node->worldTransform._13 };
			toTargetVec = { (targetPosition.x - position.x) / 3, (targetPosition.y - position.y) / 3, (targetPosition.z - position.z) / 3 };
			DirectX::XMStoreFloat3(&vec, DirectX::XMVector3Normalize(DirectX::XMVectorAdd(toTargetVec, sideVec)));
			owner->SetMovement(vec, 15.0f);
			owner->SetRunTimer(0.7f);
			step++;
		}
		break;
	case 2: // 最後にプレイヤー方向へ
		runTimer = owner->GetRunTimer() - elapsedTime;
		owner->SetRunTimer(runTimer);
		if (runTimer <= 0.0f)
		{
			toTargetVec = { (targetPosition.x - position.x), (targetPosition.y - position.y), (targetPosition.z - position.z) };
			DirectX::XMStoreFloat3(&vec, DirectX::XMVector3Normalize(toTargetVec));
			owner->SetMovement(vec, 15.0f);
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// ブレス攻撃行動
template <typename ActorType>
typename ActionBase<ActorType>::State BreathAction<ActorType>::Run(float elapsedTime)
{
	//static Effekseer::Handle handle = -1; // エフェクトのハンドルを保持

	switch (step)
	{
	case 0:
		// 攻撃対象設定
		owner->SetTargetPosition(Player::Instance().GetPosition());
		//owner->GetModel()->SetAnimationSpeed(0.45f);
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
		if (owner->GetHealth() <= 0)
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step++;
		}
		break;
	case 2:
		// アニメーション再生
		owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::Breath), false, owner->GetBlendSeconds());
		// ブレスエフェクト再生
		{
			Model::Node* modelNode = owner->GetModel()->FindNode("Bone052");
			DirectX::XMFLOAT3 pos = {
				modelNode->worldTransform._41,
				modelNode->worldTransform._42,
				modelNode->worldTransform._43
			};

			//handle = owner->breathEffect->Play(pos);
		}
		step++;
		break;
	case 3:
		if (owner->GetHealth() <= 0)
		{
			//owner->breathEffect->Stop(handle);
			//handle = -1;
			step = 0;
			return ActionBase<ActorType>::State::Complete;
			break;
		}
		{
			Model::Node* modelNode = owner->GetModel()->FindNode("Bone052");
			Model::Node* collisionNode = owner->GetModel()->FindNode("Bip001_Neck2");
			DirectX::XMFLOAT3 pos = {
				modelNode->worldTransform._41,
				modelNode->worldTransform._42,
				modelNode->worldTransform._43
			};
			DirectX::XMVECTOR forward = {
				-collisionNode->worldTransform._33,
				-collisionNode->worldTransform._23,
				-collisionNode->worldTransform._13
			};
			DirectX::XMFLOAT3 dir;
			forward = DirectX::XMVectorSetY(forward, 0.0f); // Y成分を0にしてXZ平面に投影
			DirectX::XMStoreFloat3(&dir, DirectX::XMVector3Normalize(forward));
			// ベクトルからヨー・ピッチを算出
			float yaw, pitch;
			yaw = std::atan2(dir.x, dir.z); // Y軸回転（ヨー）
			float xzLen = std::sqrt(dir.x * dir.x + dir.z * dir.z);
			pitch = std::atan2(-dir.y, xzLen); // X軸回転（ピッチ）
			DirectX::XMFLOAT3 angle = { pitch, yaw, 0.0f };

			//owner->breathEffect->SetRotation(handle, angle);
			//owner->breathEffect->SetPosition(handle, pos);

			// 当たり判定用ベクトル正規化
			DirectX::XMFLOAT3 direction;
			DirectX::XMStoreFloat3(&direction, DirectX::XMVector3Normalize(DirectX::XMVectorNegate(forward)));
			float length = 27.5f; // ブレスの長さ
			float sphereRadius = 1.5f; // 各球の半径
			int sphereCount = 15; // 球の数
			float invicibleTime = 0.5f;

			// アニメーション時間に基づく当たり判定 
			if (!owner->isPlayerInvincible)
			{
				owner->BreathEffectCollision(owner->attackFrameMin[3], owner->attackFrameMax[3], pos, direction, length, sphereRadius, sphereCount, owner->attackDamage[1], 0.5f);
			}
		}
		// エフェクト停止
		/*if (handle != -1 && owner->GetModel()->GetCurrentAnimationSeconds() > owner->attackFrameMax[3])
		{
			owner->breathEffect->Stop(handle);
			handle = -1;
		}*/
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
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 薙ぎ払いブレス攻撃行動
template <typename ActorType>
typename ActionBase<ActorType>::State BreathSweepingAction<ActorType>::Run(float elapsedTime)
{
	//static Effekseer::Handle handle = -1; // エフェクトのハンドルを保持

	switch (step)
	{
	case 0:
		// 攻撃対象設定
		owner->SetTargetPosition(Player::Instance().GetPosition());
		//owner->GetModel()->SetAnimationSpeed(0.45f);
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
		if (owner->GetHealth() <= 0)
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
			break;
		}
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step++;
		}
		break;
	case 2:
		// アニメーション再生
		owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::BreathSweeping), false, owner->GetBlendSeconds());
		// ブレスエフェクト再生
		{
			Model::Node* modelNode = owner->GetModel()->FindNode("Bone052");
			DirectX::XMFLOAT3 pos = {
				modelNode->worldTransform._41,
				modelNode->worldTransform._42,
				modelNode->worldTransform._43
			};
			//handle = owner->breathEffect->Play(pos);
		}
		step++;
		break;
	case 3:
		if (owner->GetHealth() <= 0)
		{
			//owner->breathEffect->Stop(handle);
			//handle = -1;
			step = 0;
			return ActionBase<ActorType>::State::Complete;
			break;
		}
		{
			Model::Node* modelNode = owner->GetModel()->FindNode("Bone052");
			Model::Node* collisionNode = owner->GetModel()->FindNode("Bip001_Neck2");
			DirectX::XMFLOAT3 pos = {
				modelNode->worldTransform._41,
				modelNode->worldTransform._42,
				modelNode->worldTransform._43
			};
			DirectX::XMVECTOR forward = {
				-collisionNode->worldTransform._33,
				-collisionNode->worldTransform._23,
				-collisionNode->worldTransform._13
			};
			DirectX::XMFLOAT3 dir;
			forward = DirectX::XMVectorSetY(forward, 0.0f); // Y成分を0にしてXZ平面に投影
			DirectX::XMStoreFloat3(&dir, DirectX::XMVector3Normalize(forward));
			// ベクトルからヨー・ピッチを算出
			float yaw, pitch;
			yaw = std::atan2(dir.x, dir.z); // Y軸回転（ヨー）
			float xzLen = std::sqrt(dir.x * dir.x + dir.z * dir.z);
			pitch = std::atan2(-dir.y, xzLen); // X軸回転（ピッチ）
			DirectX::XMFLOAT3 angle = { pitch, yaw, 0.0f };

			//owner->breathEffect->SetRotation(handle, angle);
			//owner->breathEffect->SetPosition(handle, pos);

			// 当たり判定用ベクトル正規化
			DirectX::XMFLOAT3 direction;
			DirectX::XMStoreFloat3(&direction, DirectX::XMVector3Normalize(DirectX::XMVectorNegate(forward)));
			float length = 27.5f; // ブレスの長さ
			float sphereRadius = 1.5f; // 各球の半径
			int sphereCount = 15; // 球の数
			float invicibleTime = 0.5f;

			// アニメーション時間に基づく当たり判定
			if (!owner->isPlayerInvincible)
			{
				owner->BreathEffectCollision(owner->attackFrameMin[4], owner->attackFrameMax[4], pos, direction, length, sphereRadius, sphereCount, owner->attackDamage[0], invicibleTime);
			}
		}
		// エフェクト停止
		/*if (handle != -1 && owner->GetModel()->GetCurrentAnimationSeconds() > owner->attackFrameMax[4])
		{
			owner->breathEffect->Stop(handle);
			handle = -1;
		}*/
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
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 咆哮攻撃行動
template <typename ActorType>
typename ActionBase<ActorType>::State RoarAction<ActorType>::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		// 攻撃対象設定
		owner->SetTargetPosition(Player::Instance().GetPosition());
		// 目的地点へ移動
		owner->MoveToTarget(elapsedTime, 1.0);		// アニメーション再生
		owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::Roar), false, owner->GetBlendSeconds());
		step++;
		break;
	case 1:
		if (!owner->IsRoarUsing && owner->GetModel()->GetCurrentAnimationSeconds() > 1.6f && owner->GetModel()->GetCurrentAnimationSeconds() < 1.61f)
		{
			owner->IsRoarUsing = true;
			//owner->RoarSE->Play(false, 0.7f);
		}
		if (owner->IsRoarUsing && owner->GetModel()->GetCurrentAnimationSeconds() > 4.67f)
		{
			owner->IsRoarUsing = false;
		}
		// アニメーションが終了しているとき
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step = 0;
			owner->RoarUsedFlag = true;
			owner->IsRoarUsing = false;
			if (owner->GetHealth() >= owner->GetMaxHealth() * 0.5f)
			{
				owner->AngryBGMStart = true;
			}
			// 攻撃成功を返す
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 体当たり攻撃行動
template <typename ActorType>
typename ActionBase<ActorType>::State TackleAction<ActorType>::Run(float elapsedTime)
{
	bool init = false;	// 初期化フラグ
	DirectX::XMFLOAT3 vec;
	switch (step)
	{
	case 0:
		// 攻撃対象設定
		owner->SetTargetPosition(Player::Instance().GetPosition());
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
		if (owner->GetHealth() <= 0)
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
			break;
		}
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step++;
		}
		break;
	case 2:
		// アニメーション再生
		owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::Tackle), false, owner->GetBlendSeconds());
		owner->SetAttackFlg(true);
		step++;
	case 3:
		if (owner->GetHealth() <= 0)
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
			break;
		}
		if (!owner->isPlayerInvincible)
		{
			owner->AttackAnimationCollision({ owner->tackleHitSpheres }, owner->attackFrameMin[5], owner->attackFrameMax[5], owner->attackDamage[1]);
		}
		{
			Model::Node* Node = owner->GetModel()->FindNode("root");
			// Rootノードから左前方向のベクトルを取る
			if (!init)
			{
				DirectX::XMVECTOR forwardVec = { Node->worldTransform._31, Node->worldTransform._32, Node->worldTransform._33 };
				//DirectX::XMVECTOR leftVec = { -Node->worldTransform._11, -Node->worldTransform._12, -Node->worldTransform._13 };
				//DirectX::XMStoreFloat3(&vec, DirectX::XMVector3Normalize(DirectX::XMVectorAdd(forwardVec, leftVec)));
				DirectX::XMStoreFloat3(&vec, DirectX::XMVector3Normalize(forwardVec));
				init = true;
			}

			if (owner->GetModel()->GetCurrentAnimationSeconds() > owner->attackFrameMin[5] && owner->GetModel()->GetCurrentAnimationSeconds() < owner->attackFrameMax[5])
			{
				// ベクトルの方向に移動
				owner->SetMovement(vec, 9.0f);
			}
		}
		// アニメーションが終了しているとき
		if (!owner->GetModel()->IsPlayAnimation())
		{
			owner->SetAttackFlg(false);
			owner->isPlayerInvincible = false;
			step = 0;
			init = false;
			// 攻撃成功を返す
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// サイドステップ行動
template <typename ActorType>
typename ActionBase<ActorType>::State SidestepAction<ActorType>::Run(float elapsedTime)
{
	bool init = false;	// 初期化フラグ
	DirectX::XMFLOAT3 vec;
	switch (step)
	{
	case 0:
		// 攻撃対象設定
		owner->SetTargetPosition(Player::Instance().GetPosition());
		// アニメーション再生
		owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::Sidestep), false, owner->GetBlendSeconds());
		step++;
	case 1:
		if (owner->GetHealth() <= 0)
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
			break;
		}
		{
			Model::Node* Node = owner->GetModel()->FindNode("Bip001");
			// Rootノードから左前方向のベクトルを取る
			if (!init)
			{
				DirectX::XMVECTOR backVec = { -Node->worldTransform._21, -Node->worldTransform._22, -Node->worldTransform._23 };
				DirectX::XMVECTOR rightVec = { Node->worldTransform._11, Node->worldTransform._12, Node->worldTransform._13 };
				DirectX::XMStoreFloat3(&vec, DirectX::XMVector3Normalize(DirectX::XMVectorAdd(backVec, rightVec)));
				init = true;
			}

			if (owner->GetModel()->GetCurrentAnimationSeconds() > owner->attackFrameMin[6] && owner->GetModel()->GetCurrentAnimationSeconds() < owner->attackFrameMax[6])
			{
				// ベクトルの方向に移動
				owner->SetMovement(vec, 6.0f);
			}
		}
		// アニメーションが終了しているとき
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step = 0;
			init = false;
			// 攻撃成功を返す
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