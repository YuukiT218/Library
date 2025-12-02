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
	int animationIndexs[4] = { owner->GetModel()->GetAnimationIndex("Combo_Attack_01_01_Seq_0"),
							   owner->GetModel()->GetAnimationIndex("Combo_Attack_01_02_Seq_0"),
							   owner->GetModel()->GetAnimationIndex("Combo_Attack_01_03_Seq_0"),
							   owner->GetModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0"), };
	int AttackCount = 0;
};

// 突進斬り
template <typename ActorType>
class DashSlashAction : public ActionBase<ActorType>
{
public:
	DashSlashAction(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Run_Attack_01_Seq_0");
	bool init = false;
	float timer = 0.0f;
	float duration = 3.0f;
	DirectX::SimpleMath::Vector3 epsilon{ 4.0f, 1.0f, 4.0f, };
	DirectX::SimpleMath::Vector3 teleportPosition;
	std::vector<DirectX::SimpleMath::Vector3> availablePositions;
	int randomIndex;
	int currentIndex;
};

// 三連テレポート
template <typename ActorType>
class TripleTeleportAction : public ActionBase<ActorType>
{
public:
	TripleTeleportAction(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	DirectX::SimpleMath::Vector3 WarpPosition[3];
	bool init = false;
	float timer = 0.0f;
	float duration = 5.0f; // 移動にかける時間
	DirectX::SimpleMath::Vector3 warpEpsilon { 0.1f, 10.0f, 0.1f, };
};

// テレポート強襲
template <typename ActorType>
class TelePortAssault : public ActionBase<ActorType>
{
public:
	TelePortAssault(ActorType* actor): ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int teleportAnimationIndex = owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0");
	int attackAnimationIndex1 = owner->GetModel()->GetAnimationIndex("Combo_Attack_01_03_Seq_0");
	int attackAnimationIndex2 = owner->GetModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0");
	DirectX::SimpleMath::Vector3 teleportPosition;
	std::vector<DirectX::SimpleMath::Vector3> availablePositions;
	int randomIndex;
	int currentIndex;
};

// テレポートコンボ
template <typename ActorType>
class TeleportCombo : public ActionBase<ActorType>
{
public:
	TeleportCombo(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int AttackCount = 0;
	int teleportAnimationIndex = owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0");
	int comboAnimationIndex1 = owner->GetModel()->GetAnimationIndex("Combo_Attack_03_01_Seq_0");
	int comboAnimationIndex2 = owner->GetModel()->GetAnimationIndex("Combo_Attack_03_02_Seq_0");
	int comboAnimationIndex3 = owner->GetModel()->GetAnimationIndex("Attack_Up_01_Seq_0");
	int comboAnimationIndex4 = owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_Loop_02_Seq_0");
	int comboAnimationIndex5 = owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_End_02_Seq_0");
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
class UnifiedDamageAction : public ActionBase<ActorType>
{
public:
	UnifiedDamageAction(ActorType* actor) : ActionBase<ActorType>(actor) {}

	ActionBase<ActorType>::State Run(float elapsedTime) override;

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
	int index = owner->GetModel()->GetCurrentAnimationIndex();
	float frame = owner->GetModel()->GetCurrentAnimationSeconds();

	AnimationConfig* config = owner->GetModel()->GetAnimationConfig("EnemyBoss", index);
	owner->GetSword()->AttackAnimationCollision(owner->GetModel(), config, owner->GetCharacter());

	// 攻撃対象設定
	owner->SetTargetPosition(Player::Instance().GetPosition());
	
	switch (step)
	{
	case 0:
		// 目的地点へ移動
		{
			owner->GetModel()->PlayRootMotion(animationIndexs[AttackCount], false, true, owner->GetBlendSeconds(), "root");
			AttackCount++;
			step++;
		}
		break;
	case 1:
		owner->MoveToTarget(elapsedTime, 0.1f);
		// アニメーション再生
		if (frame >= config->advanceInputEndFrame)
		{
			owner->GetModel()->PlayRootMotion(animationIndexs[AttackCount], false, true, owner->GetBlendSeconds(), "root");
			AttackCount++;
		}
		if (AttackCount >= 4)
		{
			step++;
		}
		break;
	case 2:
		// アニメーションが終了しているとき
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step = 0;
			AttackCount = 0;
			// 攻撃成功を返す
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsAnyDamage())
	{
		step = 0;
		AttackCount = 0;
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
	owner->SetTargetPosition(Player::Instance().GetPosition());
    
    if (!init)
    {
        init = true;
        timer = 0.0f;
        Player::Instance().warpDist = 20.0f;
    }

    float frame = owner->GetModel()->GetCurrentAnimationSeconds();
    AnimationConfig* config = owner->GetModel()->GetAnimationConfig("EnemyBoss", animationIndex);
    
    if (!owner->isPlayerInvincible)
    {
        owner->GetSword()->AttackAnimationCollision(owner->GetModel(), config, owner->GetCharacter());
    }

    timer += elapsedTime;
	float t = std::clamp(timer / duration, 0.0f, 1.0f);

    switch (step)
    {
    case 0:
        // テレポート先を決定
        if (behaviorData->IsInSequence())
        {
            availablePositions = {
                Player::Instance().PlayerFront,
                Player::Instance().PlayerFrontLeft,
                Player::Instance().PlayerFrontRight
            };
			duration = 20.0f;
            do {
                randomIndex = Mathf::RandomRange(0, static_cast<int>(availablePositions.size()) - 1);
            } while (randomIndex == currentIndex && availablePositions.size() > 1);
            teleportPosition = availablePositions[randomIndex];
        }
        
        // アニメーション再生
        owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
        step++;
        break;
    case 1:
        // アニメーションが一定の時間に達したら一時停止
        if (frame >= config->advanceInputEndFrame)
        {
            owner->GetModel()->PauseAnimation(true);
        }
		owner->TurnToTarget(elapsedTime, 1000);
		if (!owner->IsTeleporting())
	    {
		    owner->SetPosition({ Mathf::Lerp(owner->GetPosition().x, Player::Instance().GetPosition().x, t),
			   Mathf::Lerp(owner->GetPosition().y, Player::Instance().GetPosition().y, t),
			   Mathf::Lerp(owner->GetPosition().z, Player::Instance().GetPosition().z, t) });
	    }

		if (DirectX::XMVector3NearEqual(DirectX::XMLoadFloat3(&owner->GetPosition()), DirectX::XMLoadFloat3(&Player::Instance().GetPosition()), epsilon))
		{
			// シーケンス内で最後の行動でなければ通常テレポート処理
			if (behaviorData->IsInSequenceAndNotLast() && !owner->IsTeleporting())
			{
				init = false;
				owner->StartTeleport(teleportPosition, 0.5f);
				step = 0;
				timer = 0;
				return ActionBase<ActorType>::State::Complete;
			}
			else if (!owner->IsTeleporting())
			{
				// プレイヤーの近くについたらアニメーション再開
				owner->GetModel()->PauseAnimation(false);
				step++;
			}
		}
    	break;
	case 2:
		if (!owner->GetModel()->IsPlayAnimation())
		{
			init = false;
			step = 0;
			timer = 0;
			duration = 3.0f;
			Player::Instance().warpDist = 4.0f;
			return ActionBase<ActorType>::State::Complete;
		}
	}
    if (owner->IsAnyDamage())
    {
        init = false;
        owner->GetModel()->PauseAnimation(false);
        step = 0;
        timer = 0;
        Player::Instance().warpDist = 4.0f;
        return ActionBase<ActorType>::State::Failed;
    }
    return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 三連テレポート
template <typename ActorType>
typename ActionBase<ActorType>::State TripleTeleportAction<ActorType>::Run(float elapsedTime)
{
	owner->SetTargetPosition(Player::Instance().GetPosition());
	owner->TurnToTarget(elapsedTime, DirectX::XMConvertToRadians(360) * 3.0f);
	if (!init)
	{
		WarpPosition[0] = owner->WarpPosition[0];
		WarpPosition[1] = owner->WarpPosition[1];
		WarpPosition[2] = owner->WarpPosition[2];
		init = true;
		timer = 0.0f;
	}
	timer += elapsedTime;
	float t = std::clamp(timer / duration, 0.0f, 1.0f);
	switch (step)
	{
	case 0:
		owner->SetPosition({Mathf::Lerp(owner->GetPosition().x, WarpPosition[0].x, t),
			Mathf::Lerp(owner->GetPosition().y, WarpPosition[0].y, t),
			Mathf::Lerp(owner->GetPosition().z, WarpPosition[0].z, t)});
		if (DirectX::XMVector3NearEqual(DirectX::XMLoadFloat3(&owner->GetPosition()), WarpPosition[0], warpEpsilon))
		{
			step++;
			timer = 0.0f;
		}
		break;
	case 1:
		owner->SetPosition({ Mathf::Lerp(owner->GetPosition().x, WarpPosition[1].x, t),
			Mathf::Lerp(owner->GetPosition().y, WarpPosition[1].y, t),
			Mathf::Lerp(owner->GetPosition().z, WarpPosition[1].z, t) });
		if (DirectX::XMVector3NearEqual(DirectX::XMLoadFloat3(&owner->GetPosition()), WarpPosition[1], warpEpsilon))
		{
			step++;
			timer = 0.0f;
		}
		break;
	case 2:
		owner->SetPosition({ Mathf::Lerp(owner->GetPosition().x, WarpPosition[2].x, t),
			Mathf::Lerp(owner->GetPosition().y, WarpPosition[2].y, t),
			Mathf::Lerp(owner->GetPosition().z, WarpPosition[2].z, t)});
		if (DirectX::XMVector3NearEqual(DirectX::XMLoadFloat3(&owner->GetPosition()), WarpPosition[2], warpEpsilon))
		{
			init = false;
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// テレポート強襲
template <typename ActorType>
typename ActionBase<ActorType>::State TelePortAssault<ActorType>::Run(float elapsedTime)
{
	float frame = owner->GetModel()->GetCurrentAnimationSeconds();
    int index = owner->GetModel()->GetCurrentAnimationIndex();
    AnimationConfig* config = owner->GetModel()->GetAnimationConfig("EnemyBoss", index);
    
    if (!owner->isPlayerInvincible)
    {
        owner->GetSword()->AttackAnimationCollision(owner->GetModel(), config, owner->GetCharacter());
    }
    
    Player::Instance().warpDist = 4.0f;
    owner->SetTargetPosition(Player::Instance().GetPosition());
    
    switch (step)
    {
    case 0:
        // テレポート先を決定
		availablePositions = {
			Player::Instance().PlayerBack,
			Player::Instance().PlayerFrontLeft,
			Player::Instance().PlayerFrontRight
		};

		do {
			randomIndex = Mathf::RandomRange(0, static_cast<int>(availablePositions.size()) - 1);
		} while (randomIndex == currentIndex && availablePositions.size() > 1);
		teleportPosition = availablePositions[randomIndex];

        // テレポートアニメーション再生
        owner->GetModel()->PlayRootMotion(teleportAnimationIndex, false, true, owner->GetBlendSeconds(), "root");
        step++;
        break;
    case 1:
        // アニメーションが進んだらテレポート開始
        if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
        {
            // スムーズテレポート開始（0.3秒かけて移動）
            owner->StartTeleport(teleportPosition, 0.35f);
        	step++;
        }
		break;

    case 2:
        // テレポート完了確認
        if (!owner->IsTeleporting())
        {
            step++;
        }
        break;
        
    case 3:
        // 攻撃アニメーション1
        owner->GetModel()->PlayRootMotion(attackAnimationIndex1, false, true, owner->GetBlendSeconds(), "root");
        step++;
        break;
        
    case 4:
        owner->TurnToTarget(elapsedTime, 1000);
        if (frame >= config->advanceInputEndFrame)
        {
            owner->GetModel()->PlayRootMotion(attackAnimationIndex2, false, true, owner->GetBlendSeconds(), "root");
            step++;
        }
        break;
        
    case 5:
        if (behaviorData->IsInSequenceAndNotLast())
        {
            if (frame >= config->advanceInputEndFrame)
            {
                step = 0;
                return ActionBase<ActorType>::State::Complete;
            }
        }
        
        if (!owner->GetModel()->IsPlayAnimation())
        {
            step = 0;
            return ActionBase<ActorType>::State::Complete;
        }
        break;
    }
	if (owner->IsAnyDamage())
	{
		step = 0;
		return ActionBase<ActorType>::State::Failed;
	}
    return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// テレポートコンボ
template <typename ActorType>
typename ActionBase<ActorType>::State TeleportCombo<ActorType>::Run(float elapsedTime)
{
	float frame = owner->GetModel()->GetCurrentAnimationSeconds();
	int index = owner->GetModel()->GetCurrentAnimationIndex();
	AnimationConfig* config = owner->GetModel()->GetAnimationConfig("EnemyBoss", index);
	Player::Instance().warpDist = 1.5f;
	owner->SetTargetPosition(Player::Instance().GetPosition());
	if (!owner->isPlayerInvincible)
	{
		owner->GetSword()->AttackAnimationCollision(owner->GetModel(), config, owner->GetCharacter());
	}

	switch (step)
	{
	case 0:
		owner->SetGravity(0.0f);
		owner->GetModel()->PlayRootMotion(teleportAnimationIndex, false, true, owner->GetBlendSeconds(), "root");
		step++;
		break;
	case 1:
		owner->TurnToTarget(elapsedTime, 1000);
		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
		{
			owner->StartTeleport(Player::Instance().PlayerBack, 0.7f);
			switch (AttackCount)
			{
			case 0:
				owner->GetModel()->PlayRootMotion(comboAnimationIndex1, false, true, owner->GetBlendSeconds(), "root");
				AttackCount++;
				break;
			case 1:
				owner->GetModel()->PlayRootMotion(comboAnimationIndex2, false, true, owner->GetBlendSeconds(), "root");
				AttackCount++;
				break;
			case 2:
				owner->GetModel()->PlayRootMotion(comboAnimationIndex3, false, true, owner->GetBlendSeconds(), "root");
				AttackCount++;
				break;
			}
		}
		if (AttackCount >= 3)
		{
			step++;
		}
		break;
	case 2:
		owner->TurnToTarget(elapsedTime, 1000);
		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
		{
			owner->SetGravity(0.0f);
			owner->StartTeleport(Player::Instance().launchKnockbackPosition, 0.4f);
			owner->GetModel()->PlayRootMotion(comboAnimationIndex4, true, true, owner->GetBlendSeconds(), "root");
			step++;
		}
		break;
	case 3:
		owner->TurnToTarget(elapsedTime, 1000);
		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
		{
			owner->SetGravity(-1.5f);
			owner->GetModel()->PlayRootMotion(comboAnimationIndex5, false, true, owner->GetBlendSeconds(), "root");
			step++;
		}
		break;
	case 4:
		if (frame >= config->advanceInputEndFrame)
		{
			step = 0;
			AttackCount = 0;
			owner->SetGravity(-0.3f);
			return ActionBase<ActorType>::State::Complete;
		}
	}
	if (owner->IsAnyDamage())
	{
		step = 0;
		AttackCount = 0;
		owner->SetGravity(-0.3f);
		return ActionBase<ActorType>::State::Failed;
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
// ダメージタイプ別の初期処理
template <typename ActorType>
void UnifiedDamageAction<ActorType>::HandleDamageStart(DamageType type, float elapsedTime)
{
	owner->SetTargetPosition(Player::Instance().GetPosition());
	owner->TurnToTarget(elapsedTime, 10000.0f);

	// ダメージフラグをリセット
	owner->SetDamage(false);
	owner->SetLightKbDamage(false);
	owner->SetHeavyKbDamage(false);
	owner->SetLaunchKbDamage(false);

	DirectX::XMFLOAT3 knockbackPos = GetKnockbackPosition(type);

	switch (type)
	{
	case DamageType::Normal:
		if (!owner->IsGround())
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
		if (!owner->IsGround())
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
		if (!owner->IsGround())
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
typename ActionBase<ActorType>::State UnifiedDamageAction<ActorType>::Run(float elapsedTime)
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
			return ActionBase<ActorType>::State::Complete;
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
			return ActionBase<ActorType>::State::Complete;
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
	if (owner->IsAnyDamage())
	{
		owner->SetGravity(-0.3f);
		owner->SetVerticalVelocity(0.0f);
		step = 0;
		return ActionBase<ActorType>::State::Complete;
	}

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