#pragma once
#include "ActionBase.h"

#include "Character\Player.h"
#include "Character\Projectile/ProjectileManager.h"
#include "Math\Mathf.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

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
							   owner->GetModel()->GetAnimationIndex("Combo_Attack_02_03_Seq_0"),
							   owner->GetModel()->GetAnimationIndex("Combo_Attack_02_04_Seq_0"), };
	int AttackCount = 0;
};

// 斬撃波
template <typename ActorType>
class SlashWave : public ActionBase<ActorType>
{
public:
	SlashWave(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndexes[4] = { owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_01_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_02_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_03_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_04_Seq_0") };
	int AttackCount = 0;
private:
	bool hasShot = false;
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
	bool playedEffect = false;
	float timer = 0.0f;
	float duration = 7.0f;
	DirectX::SimpleMath::Vector3 epsilon{ 4.0f, 1.0f, 4.0f, };
	DirectX::SimpleMath::Vector3 teleportPosition;
};

// 光柱円形拡散
template <typename ActorType>
class PillarSpiral : public ActionBase<ActorType>
{
public:
	PillarSpiral(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndexes[3] = { owner->GetModel()->GetAnimationIndex("Power_Attack_Start_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Power_Attack_Loop_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_02_Seq_0") };
	std::vector<Projectile*> spawnedProjectiles;
	float waitTimer = 0.0f;
	const float waitDuration = 1.5f;
	float recoveryTimer = 0.0f;
	const float recoveryDuration = 2.0f; // 後隙の時間
};

// 通常テレポート
template <typename ActorType>
class NormalTeleport : public ActionBase<ActorType>
{
public:
	NormalTeleport(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0");
	DirectX::SimpleMath::Vector3 teleportPosition;
};

// 三連テレポート
template <typename ActorType>
class TripleTeleportAction : public ActionBase<ActorType>
{
public:
	TripleTeleportAction(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	DirectX::SimpleMath::Vector3 WarpPosition[3];
	DirectX::SimpleMath::Vector3 currentStartPosition;
	bool init = false;
	float timer = 0.0f;
	float duration = 0.4f; // 移動にかける時間
	float arcHeight = 1.5f;
	DirectX::SimpleMath::Vector3 warpEpsilon { 0.1f, 10.0f, 0.1f, };
	int count = 0;
	Effekseer::Handle handle = -1;
	float intervalTimer = 0.0f;
	const float intervalDuration = 0.2f; // 移動間のわずかな間
	float recoveryTimer = 0.0f;
	const float recoveryDuration = 1.3f; // 終了後の後隙
};

// テレポート強襲
template <typename ActorType>
class TelePortAssault : public ActionBase<ActorType>
{
public:
	TelePortAssault(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndexes[3] = { owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Combo_Attack_01_03_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0") };
	DirectX::SimpleMath::Vector3 teleportPosition;
	std::vector<Projectile*> spawnedProjectiles;
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
	int comboAnimationIndexes[5] = {owner->GetModel()->GetAnimationIndex("Combo_Attack_03_01_Seq_0"),
								  owner->GetModel()->GetAnimationIndex("Combo_Attack_03_02_Seq_0"),
								  owner->GetModel()->GetAnimationIndex("Attack_Up_01_Seq_0"),
								  owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_Loop_02_Seq_0"),
								  owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_End_02_Seq_0") };
	DirectX::SimpleMath::Vector3 teleportPosition;
};

// 必殺技
template <typename ActorType>
class SpecialAttack : public ActionBase<ActorType>
{
public:
	SpecialAttack(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndexes[5] = { owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Power_Attack_Start_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Power_Attack_Loop_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Knock_Down_Combat_Loop_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Knock_Down_Combat_End_Seq_0")};
	std::vector<Projectile*> groupSpiral1;
	std::vector<Projectile*> groupSpiral2;
	std::vector<Projectile*> groupHoming;
	float waitTimer = 0.0f;
	const float waitDuration = 1.0f;
	float recoveryTimer = 0.0f;
	const float recoveryDuration = 6.0f; // 後隙の時間
	bool playedEffect = false;
	Effekseer::Handle handle = -1;
	int shotCount = 0;              // 3連射の現在の発射数
	float shotIntervalTimer = 0.0f; // 連射の間隔タイマー
	const float shotInterval = 1.0f;// 連射の間隔時間

	int waveCount = 0;              // 円形攻撃の現在の回数
	const float circleWaitTime = 1.5f; // 円形展開してから発射するまでの溜め時間
	float lockOnTime = 0.3f; // 発射直前に追従を止める時間
	float currentRotationAngle = 0.0f; // 円全体を回すための角度
};

// 強制反撃1
template <typename ActorType>
class RevengeDive : public ActionBase<ActorType>
{
public:
	RevengeDive(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	float intervalTimer = 0.0f;
	const float intervalDuration = 0.7f; // 移動間のわずかな間
	int animationIndexes[2] = { owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_Loop_02_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_End_02_Seq_0") };
};

// 強制反撃2
template <typename ActorType>
class RevengeAssault : public ActionBase<ActorType>
{
public:
	RevengeAssault(ActorType* actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	float intervalTimer = 0.0f;
	const float intervalDuration = 0.7f; // 移動間のわずかな間
	int animationIndexes[3] = { owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Combo_Attack_01_03_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0") };
	DirectX::SimpleMath::Vector3 teleportPosition;
	std::vector<Projectile*> spawnedProjectiles;
};

// 様子見歩き
template <typename ActorType>
class CautiousWalkAction : public ActionBase<ActorType>
{
public:
	CautiousWalkAction(ActorType*actor) : ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndexes[2] = { owner->GetModel()->GetAnimationIndex("Walk_Combat_Loop_F_L_90_RM_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Walk_Combat_Loop_F_R_90_RM_Seq_0") };
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

// 落下モーション
template <typename ActorType>
class FallAction : public ActionBase<ActorType>
{
public:
	FallAction(ActorType* actor) :ActionBase(actor) {}
	ActionBase::State Run(float elapsedTime);
	int animationIndex1 = owner->GetModel()->GetAnimationIndex("Jump_Combat_Loop_0_Seq_0");
	int animationIndex2 = owner->GetModel()->GetAnimationIndex("Jump_Combat_End_0_Seq_0");
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
private:
	Effekseer::Handle handle = -1;
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
		owner->TurnToTarget(elapsedTime, 10000);
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
	if (owner->IsAnyDamage() || owner->IsDeathFlag())
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
// 斬撃波
template <typename ActorType>
typename ActionBase<ActorType>::State SlashWave<ActorType>::Run(float elapsedTime)
{
	owner->SetTargetPosition(Player::Instance().GetPosition());
	owner->TurnToTarget(elapsedTime, 10000);

	int index = owner->GetModel()->GetCurrentAnimationIndex();
	float frame = owner->GetModel()->GetCurrentAnimationSeconds();

	AnimationConfig* config = owner->GetModel()->GetAnimationConfig("EnemyBoss", index);

	if (!owner->isPlayerInvincible)
	{
		owner->GetSword()->AttackAnimationCollision(owner->GetModel(), config, owner->GetCharacter());
	}

	switch (step)
	{
	case 0: // 初期化
	{
		// アニメーション再生
		int animIndex = animationIndexes[AttackCount % 4];
		owner->GetModel()->PlayAnimation(animIndex, false);

		// 発射フラグをリセット
		hasShot = false;
		step++;
		break;
	}
	case 1: // 発射待機
		if (!hasShot && frame >= config->advanceInputStartFrame)
		{
			// 1. プリセットから基本情報を取得
			ProjectileInfo info = ProjectileManager::GetSlashWaveInfo();

			// 2. 発射者(オーナー)を設定
			info.owner = owner;

			// 3. 発射位置を設定 (ボスの位置 + 少し上 + 少し前)
			DirectX::XMFLOAT3 bossPos = owner->GetPosition();
			DirectX::XMFLOAT3 bossForward = owner->CharacterForward(owner->GetAngle());

			info.spawnPosition = bossPos;
			info.spawnPosition.x += bossForward.x * 1.5f;
			info.spawnPosition.z += bossForward.z * 1.5f;

			// 方向計算は不要なので初期値(0,0,1など)のままでOK、またはボスの向きを入れておく
			info.direction = bossForward;

			// 3. 生成 (ポインタを受け取る)
			Projectile* p = ProjectileManager::Instance().Launch(info);

			// 4. 計算処理を委譲して発射
			if (p)
			{
				// プレイヤーの位置を取得
				DirectX::XMFLOAT3 targetPos = Player::Instance().GetPosition();

				// ★ここでベクトル計算を Projectile 側に任せる
				// 第二引数は弾の速度 (infoで設定された速度をそのまま使う)
				p->FireAt(targetPos, info.speed, false);
			}

			hasShot = true;
		}

		// アニメーション終了チェック
		if (frame >= config->advanceInputEndFrame)
		{
			// 次のコンボへ移行可能
			AttackCount++;
			step = 0;
		}
		if (AttackCount >= 4)
		{
			step = 2;
		}
		break;
	case 2:
		// アニメーションが終了しているとき
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step = 0;
			AttackCount = 0;
			hasShot = false;
			// 攻撃成功を返す
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}

	// ダメージを受けた場合は中断
	if (owner->IsAnyDamage() || owner->IsDeathFlag())
	{
		step = 0;
		AttackCount = 0;
		hasShot = false;
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
    }

    float frame = owner->GetModel()->GetCurrentAnimationSeconds();
    AnimationConfig* config = owner->GetModel()->GetAnimationConfig("EnemyBoss", animationIndex);
    
    if (!owner->isPlayerInvincible)
    {
        owner->GetSword()->AttackAnimationCollision(owner->GetModel(), config, owner->GetCharacter());
    }

    timer += elapsedTime;
	float t = std::clamp(timer / duration, 0.0f, 1.0f);

	if (!owner->GetPlayedEffect())
	{
		owner->attackSign->Play({ owner->GetPosition().x, owner->GetPosition().y + 1.0f, owner->GetPosition().z });
		owner->SetPlayedEffect(true);
	}

    switch (step)
    {
    case 0:
        // テレポート先を決定
        if (behaviorData->IsInSequence())
        {
			teleportPosition = owner->CalculateVisibleTeleportPos(15.0f);
			duration = 15.0f;
        }
		
        
        // アニメーション再生
        owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
		owner->SetGravity(-0.001f);
        step++;
        break;
    case 1:
		// アニメーションが一定の時間に達したら一時停止
		if (frame >= config->advanceInputStartFrame)
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
				owner->StartTeleport(
					teleportPosition,
					0.1f  // 0.4秒で消失
				);
				step = 0;
				timer = 0;
				return ActionBase<ActorType>::State::Complete;
			}
			if ((!behaviorData->IsInSequence() || behaviorData->IsLastNodeInSequence()) && !owner->IsTeleporting())
			{
				owner->attackSign->Play({ owner->GetPosition().x, owner->GetPosition().y + 1.0f, owner->GetPosition().z });
				// プレイヤーの近くについたらアニメーション再開
				owner->GetModel()->PauseAnimation(false);
				step++;
			}
		}
    	break;
	case 2:
		if (!owner->GetModel()->IsPlayAnimation() || (!owner->IsGround() && frame >= config->advanceInputEndFrame))
		{
			owner->SetGravity(-0.3f);
			init = false;
			owner->SetPlayedEffect(false);
			step = 0;
			timer = 0;
			duration = 3.0f;
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
    if (owner->IsAnyDamage() || owner->IsDeathFlag())
    {
		owner->SetGravity(-0.3f);
        init = false;
		owner->SetPlayedEffect(false);
    	owner->GetModel()->PauseAnimation(false);
        step = 0;
        timer = 0;
        return ActionBase<ActorType>::State::Failed;
    }
    return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 光柱円形拡散
template <typename ActorType>
typename ActionBase<ActorType>::State PillarSpiral<ActorType>::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		owner->GetModel()->PlayRootMotion(animationIndexes[0], false, true, 0.2f, "root");
		step++;
		break;
	case 1: // 円形配置
		owner->GetModel()->PlayRootMotion(animationIndexes[1], true, true, 0.2f, "root");
		if (spawnedProjectiles.empty())
		{
			int count = 8;
			float initialRadius = 2.0f;
			DirectX::XMFLOAT3 center = owner->GetPosition();

			for (int i = 0; i < count; ++i)
			{
				ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
				info.owner = owner;
				info.moveType = MovementType::Stationary;
				info.scale = 0.5f;
				info.radius = 0.7f;
				info.lifeTime = 5.0f;
				info.centerPosition = center; // 回転中心を記憶
				info.currentRadius = initialRadius;
				info.invincibleTime = 1.0f;

				float angle = DirectX::XM_2PI * (float)i / (float)count;
				info.currentAngle = angle;

				info.spawnPosition = {
					center.x + cosf(angle) * initialRadius,
					center.y,
					center.z + sinf(angle) * initialRadius
				};

				Projectile* p = ProjectileManager::Instance().Launch(info);
				if (p) spawnedProjectiles.push_back(p);
			}
		}

		waitTimer += elapsedTime;
		if (waitTimer >= waitDuration)
		{
			step++;
		}
		break;

	case 2: // 回転拡散開始
		for (auto* p : spawnedProjectiles)
		{
			if (p && p->IsActive())
			{
				owner->GetModel()->PlayRootMotion(animationIndexes[3], false, true, 0.2f, "root");
				// Projectile側でタイプをSpiralに変更
				p->StartSpiral(
					3.0f,  // 回転速度 (ラジアン/秒)
					5.0f   // 拡散速度 (メートル/秒)
				);
			}
		}
		spawnedProjectiles.clear();
		step++;
		break;

	case 3: // アニメーション終了待ち
		if (!owner->GetModel()->IsPlayAnimation())
		{
			// 即終了せず、後隙ステップへ移行
			step++;
		}
		break;

	case 4: // 追加: 後隙（Recovery）
		owner->GetModel()->PlayAnimation(owner->GetModel()->GetAnimationIndex("Idle_Combat_Seq_0"), true, 0.2f);
		recoveryTimer += elapsedTime;
		if (recoveryTimer >= recoveryDuration)
		{
			step = 0;
			waitTimer = 0.0f;
			recoveryTimer = 0.0f;
			spawnedProjectiles.clear();
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsAnyDamage() || owner->IsDeathFlag())
	{
		step = 0;
		waitTimer = 0.0f;
		recoveryTimer = 0.0f;
		spawnedProjectiles.clear();
		return ActionBase<ActorType>::State::Failed;
	}
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 通常テレポート
template <typename ActorType>
typename ActionBase<ActorType>::State NormalTeleport<ActorType>::Run(float elapsedTime)
{
	owner->SetTargetPosition(Player::Instance().GetPosition());

	float frame = owner->GetModel()->GetCurrentAnimationSeconds();
	AnimationConfig* config = owner->GetModel()->GetAnimationConfig("EnemyBoss", animationIndex);

	switch (step)
	{
	case 0:
		teleportPosition = owner->CalculateVisibleTeleportPos(12.5f, true);

		// アニメーション再生
		owner->GetModel()->PlayRootMotion(animationIndex, false, true, owner->GetBlendSeconds(), "root");
		step++;
		break;
	case 1:
		owner->TurnToTarget(elapsedTime, 1000);

		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
		{
			owner->StartTeleport(
				teleportPosition,
				0.3f  // 0.4秒で消失
			);
			step++;
		}
		break;
	case 2:
		if (!owner->IsTeleporting())
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
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

	// 初期化
	if (!init)
	{
		WarpPosition[0] = owner->WarpPosition[0];
		WarpPosition[1] = owner->WarpPosition[1];

		// 最初の始点を保存
		currentStartPosition = owner->GetPosition();

		// 向き調整
		owner->TurnToTarget(elapsedTime, DirectX::XMConvertToRadians(360) * 10.0f);

		init = true;
		timer = 0.0f;
		intervalTimer = 0.0f;
		recoveryTimer = 0.0f;
		count = 0;
	}

	switch (step)
	{
	case 0: // 開始演出（弾発射 + 姿を消す）
	{
		owner->SetInvincible(true);
		// (前回のコードと同じ弾発射処理)
		ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
		info.owner = owner;

		DirectX::XMFLOAT3 bossPos = owner->GetPosition();
		DirectX::XMFLOAT3 bossForward = owner->CharacterForward(owner->GetAngle());
		info.spawnPosition = bossPos;
		info.spawnPosition.x += bossForward.x * 1.5f;
		info.spawnPosition.z += bossForward.z * 1.5f;

		DirectX::XMFLOAT3 targetPos = Player::Instance().GetPosition();
		DirectX::XMFLOAT3 toTarget = {
			targetPos.x - info.spawnPosition.x,
			0,
			targetPos.z - info.spawnPosition.z
		};
		float len = sqrtf(toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z);
		if (len > 0.0f) {
			toTarget.x /= len; toTarget.z /= len;
			info.direction = toTarget;
		}
		else {
			info.direction = bossForward;
		}
		ProjectileManager::Instance().Launch(info);
	}

	if (handle == -1)
	{
		handle = owner->lightBall->Play(owner->GetPosition(), 0.5f);
		owner->SetScale({ 0,0,0 }); // 姿を消す（光の玉だけが跳ねるように見せる）
	}
	step++;
	break;

	case 1: // 放物線移動（ホッピング動作）
	{
		timer += elapsedTime;
		// 進行度 (0.0 -> 1.0)
		float t = std::clamp(timer / duration, 0.0f, 1.0f);

		// 線形補間（始点と終点の間を直線移動）
		float x = Mathf::Lerp(currentStartPosition.x, WarpPosition[count].x, t);
		float z = Mathf::Lerp(currentStartPosition.z, WarpPosition[count].z, t);
		float base_y = Mathf::Lerp(currentStartPosition.y, WarpPosition[count].y, t);

		// 放物線の高さを計算 (sinカーブを利用)
		// t=0で0, t=0.5で1, t=1.0で0 になる動き * 高さ
		float arc = sinf(t * DirectX::XM_PI) * arcHeight;

		// 座標適用 (Y座標に高さを足す)
		owner->SetPosition({ x, base_y + arc, z });

		// エフェクト追従
		owner->lightBall->SetPosition(handle, owner->GetPosition());

		// 着地判定
		if (t >= 1.0f)
		{
			// 位置を目的地に強制補正
			owner->SetPosition(WarpPosition[count]);

			// 次のジャンプのために現在の場所を「始点」として保存
			currentStartPosition = WarpPosition[count];

			timer = 0.0f;
			count++;

			// 次のステップへ
			if (count >= 3) {
				step = 3; // 全移動終了
			}
			else {
				step = 2; // 次のジャンプ前の着地タメ
			}
		}
		break;
	}

	case 2: // インターバル（着地時のバウンド感/タメ）
		intervalTimer += elapsedTime;
		owner->lightBall->SetPosition(handle, owner->GetPosition());

		if (count == 2)
		{
			WarpPosition[2] = owner->CalculateVisibleTeleportPos(2.0f, true);
		}

		if (intervalTimer >= intervalDuration)
		{
			intervalTimer = 0.0f;
			step = 0; // 次の放物線ジャンプへ
		}
		break;

	case 3: // 終了処理＆後隙
		owner->TurnToTarget(elapsedTime, 10.0f);
		owner->GetModel()->PlayAnimation(owner->GetModel()->GetAnimationIndex("Idle_Combat_Seq_0"), true, 0.2f);

		recoveryTimer += elapsedTime;
		if (recoveryTimer >= recoveryDuration)
		{
			owner->SetInvincible(false);
			// 姿を現す
			owner->SetScale({ 1,1,1 });
			handle = -1;
			init = false;
			step = 0;
			count = 0;
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
    
    owner->SetTargetPosition(Player::Instance().GetPosition());
    
    switch (step)
    {
    case 0:
        // テレポート先を決定
		teleportPosition = owner->CalculateVisibleTeleportPos(3.0f);

        // テレポートアニメーション再生
		owner->SetGravity(-0.001f);
        owner->GetModel()->PlayRootMotion(animationIndexes[0], false, true, owner->GetBlendSeconds(), "root");
        step++;
        break;
    case 1:
        // アニメーションが進んだらテレポート開始
        if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
        {
            // スムーズテレポート開始（0.3秒かけて移動）
			owner->StartTeleport(
				teleportPosition,
				0.2f  
			);
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
		owner->TurnToTarget(elapsedTime, 10000);
        // 攻撃アニメーション1
        owner->GetModel()->PlayRootMotion(animationIndexes[1], false, true, owner->GetBlendSeconds(), "root");
		if (owner->GetHealth() <= (owner->GetMaxHealth() * 0.5) && behaviorData->IsLastNodeInSequence() && spawnedProjectiles.empty())
		{
			for (int i = 0; i < 2; i++)
			{
				ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
				info.owner = owner;
				info.moveType = MovementType::Stationary;
				info.scale = 0.5f;
				info.radius = 0.7f;
				info.lifeTime = 5.0f;
				info.speed = 40.0f;

				if (i == 0)
				{
					info.spawnPosition = {
						owner->GetPosition().x + (owner->CharacterBack(owner->GetAngle()).x - owner->CharacterLeft(owner->GetAngle()).x) * 3.0f,
						-1,
						owner->GetPosition().z + (owner->CharacterBack(owner->GetAngle()).z  - owner->CharacterLeft(owner->GetAngle()).z) * 2.0f 
					};
				}
				else
				{
					info.spawnPosition = {
						owner->GetPosition().x + (owner->CharacterBack(owner->GetAngle()).x - owner->CharacterRight(owner->GetAngle()).x) * 3.0f,
						-1,
						owner->GetPosition().z + (owner->CharacterBack(owner->GetAngle()).z - owner->CharacterRight(owner->GetAngle()).z) * 2.0f
					};
				}
				Projectile* p = ProjectileManager::Instance().Launch(info);
				if (p) spawnedProjectiles.push_back(p);
			}
		}
        step++;
        break;
        
    case 4:
		owner->TurnToTarget(elapsedTime, 1000);
		if (frame >= config->advanceInputEndFrame)
        {
			owner->SetGravity(-0.3f);
            owner->GetModel()->PlayRootMotion(animationIndexes[2], false, true, owner->GetBlendSeconds(), "root");
            step++;
        }
        break;
        
    case 5:
		if (frame >= config->advanceInputStartFrame)
		{
			if (behaviorData->IsLastNodeInSequence())
			{
				for (auto* p : spawnedProjectiles)
				{
					if (p && p->IsActive())
					{
						// Projectile側でタイプをSpiralに変更
						p->FireAt(
							Player::Instance().GetPosition(),  // 回転速度 (ラジアン/秒)
							20.0f,   // 拡散速度 (メートル/秒)
							true
						);
					}
				}
				spawnedProjectiles.clear();
			}
			step++;
		}
		break;

    case 6:
        if (behaviorData->IsInSequenceAndNotLast())
        {
            if (frame >= config->advanceInputStartFrame)
            {
                step = 0;
                return ActionBase<ActorType>::State::Complete;
            }
        }
        
        if (!owner->GetModel()->IsPlayAnimation() || (frame >= config->advanceInputEndFrame && !owner->IsGround()))
        {
			owner->SetGravity(-0.3f);
            step = 0;
            return ActionBase<ActorType>::State::Complete;
        }
        break;
    }
	if (owner->IsAnyDamage() || owner->IsDeathFlag())
	{
		owner->SetGravity(-0.3f);
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
	owner->SetTargetPosition(Player::Instance().GetPosition());
	if (!owner->isPlayerInvincible)
	{
		owner->GetSword()->AttackAnimationCollision(owner->GetModel(), config, owner->GetCharacter());
	}

	switch (step)
	{
	case 0:
		owner->GetModel()->PlayRootMotion(teleportAnimationIndex, false, true, owner->GetBlendSeconds(), "root");
		step++;
		break;
	case 1:
		owner->TurnToTarget(elapsedTime, 1000);
		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting() && AttackCount < 3)
		{
			teleportPosition = owner->CalculateVisibleTeleportPos(2.0f);
			owner->SetGravity(0.0f);
			owner->StartTeleport(
				teleportPosition,
				0.1f 
			);
			step++;
		}
		else if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting() && AttackCount >= 3)
		{
			owner->StartTeleport(
				Player::Instance().launchKnockbackPosition,
				0.1f 
			);
			step++;
		}
		break;
	case 2:
		owner->TurnToTarget(elapsedTime, 1000);
		if (!owner->IsTeleporting())
		{
			step++;
		}
		break;
	case 3:
		owner->TurnToTarget(elapsedTime, 1000);
		if (!owner->IsTeleporting())
		{
			owner->GetModel()->PlayRootMotion(comboAnimationIndexes[AttackCount], false, true, owner->GetBlendSeconds(), "root");
			AttackCount++;
			if (frame >= config->advanceInputEndFrame && AttackCount < 4)
			{
				step = 1;
			}
			else if (frame >= config->advanceInputEndFrame && AttackCount >= 4)
			{
				step++;
			}
		}
		break;
	case 4:
		owner->TurnToTarget(elapsedTime, 1000);
		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
		{
			owner->SetGravity(-1.5f);
			owner->GetModel()->PlayRootMotion(comboAnimationIndexes[AttackCount], false, true, owner->GetBlendSeconds(), "root");
			step++;
		}
		break;
	case 5:
		if (frame >= config->advanceInputEndFrame)
		{
			step = 0;
			AttackCount = 0;
			owner->SetGravity(-0.3f);
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsAnyDamage() || owner->IsDeathFlag())
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
// 必殺技
template <typename ActorType>
typename ActionBase<ActorType>::State SpecialAttack<ActorType>::Run(float elapsedTime)
{
	float frame = owner->GetModel()->GetCurrentAnimationSeconds();
	int index = owner->GetModel()->GetCurrentAnimationIndex();
	AnimationConfig* config = owner->GetModel()->GetAnimationConfig("EnemyBoss", index);

	switch (step)
	{
	case 0:
		owner->GetModel()->PlayRootMotion(animationIndexes[0], false, true, owner->GetBlendSeconds(), "root");
		step++;
		break;
	case 1:
		owner->TurnToTarget(elapsedTime, 1000);
		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
		{
			owner->SetGravity(0.0f);
			owner->StartTeleport(
				{-0.37f, -1.460f, 0.35f},
				0.1f
			);
			step++;
		}
		break;
	case 2:
		if (!owner->IsTeleporting())
		{
			if (!playedEffect)
			{
				handle = owner->magicCircle->Play(owner->GetPosition(), 5.0f);
				owner->GetModel()->PlayRootMotion(animationIndexes[2], true, true, 0.2f, "root");
				playedEffect = true;
			}
		}
		waitTimer += elapsedTime;
		if (waitTimer >= waitDuration)
		{
			waitTimer = 0;
			step++;
		}
		break;
	case 3: // 円形配置
		if (!owner->IsTeleporting())
		{
			if (groupSpiral1.empty())
			{
				int count = 6;

				DirectX::XMFLOAT3 center = owner->GetPosition();

				for (int i = 0; i < count; ++i)
				{
					ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
					info.owner = owner;
					info.moveType = MovementType::Stationary;
					info.scale = 0.4f;
					info.radius = 0.5f;
					info.lifeTime = 10.0f;
					info.centerPosition = center; // 回転中心を記憶
					float initialRadius = 2.0f;
					info.currentRadius = initialRadius;
					info.invincibleTime = 1.0f;

					float angle = DirectX::XM_2PI * (float)i / (float)count;
					info.currentAngle = angle;

					info.spawnPosition = {
						center.x + cosf(angle) * initialRadius,
						center.y,
						center.z + sinf(angle) * initialRadius
					};

					Projectile* p = ProjectileManager::Instance().Launch(info);
					if (p) groupSpiral1.push_back(p);

					initialRadius = 6.0f;
					info.currentRadius = initialRadius;
					info.spawnPosition = {
						center.x + cosf(angle) * initialRadius,
						center.y,
						center.z + sinf(angle) * initialRadius
					};

					p = ProjectileManager::Instance().Launch(info);
					if (p) groupSpiral2.push_back(p);
				}
			}
			waitTimer += elapsedTime;
			if (waitTimer >= waitDuration)
			{
				step++;
			}
		}
		break;

	case 4: // 回転拡散開始
		for (auto* p : groupSpiral1)
		{
			if (p && p->IsActive())
			{
				// Projectile側でタイプをSpiralに変更
				p->StartSpiral(
					1.0f,  // 回転速度 (ラジアン/秒)
					2.0f   // 拡散速度 (メートル/秒)
				);
			}
		}
		for (auto* p : groupSpiral2)
		{
			if (p && p->IsActive())
			{
				// Projectile側でタイプをSpiralに変更
				p->StartSpiral(
					-1.0f,  // 回転速度 (ラジアン/秒)
					2.0f   // 拡散速度 (メートル/秒)
				);
			}
		}
		groupSpiral1.clear();
		groupSpiral2.clear();
		step++;
		break;

	case 5: // 3連射の初期化
		shotCount = 0;
		shotIntervalTimer = shotInterval; // 最初は即撃ちか、少し待つか調整
		groupHoming.clear(); // 安全のためクリア
		step++;
		break;

	case 6: // 時間差で3発発射
		shotIntervalTimer += elapsedTime;
		if (shotIntervalTimer >= shotInterval)
		{
			shotIntervalTimer = 0.0f;

			// 弾の設定
			ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
			info.owner = owner;
			info.scale = 0.4f;
			info.radius = 0.5f;
			info.lifeTime = 5.0f;

			// ボスの位置から計算（少しランダム性を持たせても良い）
			DirectX::XMFLOAT3 spawnPos = owner->CalculateVisibleTeleportPos(20.0f); // 画面外から飛んでくる演出ならこれでもOK
			// あるいはボスの正面から出すなら
			spawnPos.y = owner->GetPosition().y;
			info.spawnPosition = spawnPos;

			Projectile* p = ProjectileManager::Instance().Launch(info);
			if (p)
			{
				// プレイヤーに向けて発射
				p->FireAt(Player::Instance().GetPosition(), 25.0f, true); // 弾速25.0f
			}

			shotCount++;
			if (shotCount >= 3)
			{
				step++; // 次のフェーズへ
			}
		}
		break;

	case 7: // 円形攻撃ループの初期化
		waitTimer = 0.0f;
		step++;
		break;

	case 8: // プレイヤーの周囲に円形展開 (Wave開始)
	{
		groupHoming.clear();
		int count = 8; // 柱の本数
		float radius = 3.0f; // プレイヤーからの距離
		DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();

		for (int i = 0; i < count; ++i)
		{
			ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
			info.owner = owner;
			info.moveType = MovementType::Stationary; // その場で待機
			info.scale = 0.4f;
			info.radius = 0.5f;
			info.lifeTime = 8.0f; // 待機時間があるので長めに
			info.invincibleTime = 0.5f;

			float angle = DirectX::XM_2PI * (float)i / (float)count;

			// プレイヤー中心に配置
			info.spawnPosition = {
				playerPos.x + cosf(angle) * radius,
				owner->GetPosition().y, // 上空に出現させる演出
				playerPos.z + sinf(angle) * radius
			};

			Projectile* p = ProjectileManager::Instance().Launch(info);
			if (p)
			{
				// 地面に降りる演出を入れるならここでVelocity.yを設定するか、
				// あるいは最初から地面(playerPos.y)に出す
				groupHoming.push_back(p);
			}
		}

		waitTimer = 0.0f;
		step++;
		break;
	}

	case 9: // 溜め時間（プレイヤーへの警告時間）
		waitTimer += elapsedTime;

		// 「待機時間の終わり - ロックオン時間」までは追従する
		// 例: 1.5秒待機のうち、最初の1.0秒は追いかけてくる
		if (waitTimer < circleWaitTime - lockOnTime)
		{
			// 円全体を少し回転させる（演出）
			currentRotationAngle += 1.0f * elapsedTime;

			DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
			float radius = 3.0f;
			int count = (int)groupHoming.size();

			for (int i = 0; i < count; ++i)
			{
				Projectile* p = groupHoming[i];
				if (p && p->IsActive())
				{
					// 角度計算：(i / count) * 2PI + 全体の回転
					float angle = (DirectX::XM_2PI * (float)i / (float)count) + currentRotationAngle;

					DirectX::XMFLOAT3 newPos;
					newPos.x = playerPos.x + cosf(angle) * radius;
					newPos.y = owner->GetPosition().y; // プレイヤーの頭上5m
					newPos.z = playerPos.z + sinf(angle) * radius;

					// 座標を上書き
					p->SetPosition(newPos);
				}
			}
		}
		// ロックオン時間に入ったら座標更新を止める（これでプレイヤーは回避猶予ができる）
		if (waitTimer >= circleWaitTime)
		{
			step++;
		}
		break;

	case 10: // 一斉発射
	{
		for (auto* p : groupHoming)
		{
			if (p && p->IsActive())
			{
				// プレイヤーへ向かって一斉発射
				p->FireAt(Player::Instance().GetPosition(), 20.0f, true);
			}
		}
		groupHoming.clear();

		waveCount++;

		// 指定回数(2回)繰り返す判定
		if (waveCount < 2)
		{
			waitTimer = 0.0f;
			// 少し間隔を空けて次のWaveへ (Case 8に戻る前に少し待つ用)
			// ここでは簡易的に即座にCase 8の準備へ戻るが、
			// ステップを追加してインターバルを作っても良い
			step = 11;
		}
		else
		{
			step = 12; // 全Wave終了、後隙へ
		}
		break;
	}

	case 11: // Wave間のインターバル
		waitTimer += elapsedTime;
		if (waitTimer >= 1.0f) // 1秒空けて次の円形展開
		{
			step = 5; // 再び展開へ戻る
		}
		break;

	case 12: // 後隙（Recovery）
		owner->GetModel()->PlayRootMotion(animationIndexes[3], true, true, 0.2f, "root");
		recoveryTimer += elapsedTime;
		if (recoveryTimer >= recoveryDuration)
		{
			owner->GetModel()->PlayRootMotion(animationIndexes[4], true, true, 0.2f, "root");
			playedEffect = false;
			owner->magicCircle->Stop(handle);
			handle = -1;
			step = 0;
			waitTimer = 0.0f;
			recoveryTimer = 0.0f;
			shotCount = 0;
			waveCount = 0;
			groupHoming.clear();
			owner->SetSpecialReady(false);
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsAnyDamage() || owner->IsDeathFlag())
	{
		step = 0;
		playedEffect = false;
		owner->magicCircle->Stop(handle);
		handle = -1;
		step = 0;
		waitTimer = 0.0f;
		recoveryTimer = 0.0f;
		shotCount = 0;
		waveCount = 0;
		groupHoming.clear();
		groupSpiral1.clear();
		groupSpiral2.clear();
		owner->SetSpecialReady(false);
		return ActionBase<ActorType>::State::Failed;
	}

	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 強制反撃1
template <typename ActorType>
typename ActionBase<ActorType>::State RevengeDive<ActorType>::Run(float elapsedTime)
{
	float frame = owner->GetModel()->GetCurrentAnimationSeconds();
	int index = owner->GetModel()->GetCurrentAnimationIndex();
	AnimationConfig* config = owner->GetModel()->GetAnimationConfig("EnemyBoss", index);
	owner->SetSuperArmor(true);
	owner->SetTargetPosition(Player::Instance().GetPosition());
	if (!owner->isPlayerInvincible)
	{
		owner->GetSword()->AttackAnimationCollision(owner->GetModel(), config, owner->GetCharacter());
	}

	switch (step)
	{
	case 0:
		owner->SetGravity(0.0f);
		owner->StartTeleport(
			Player::Instance().launchKnockbackPosition,
			0.1f
		);
		step++;
		break;
	case 1:
		intervalTimer += elapsedTime;
		owner->TurnToTarget(elapsedTime, 1000);
		if (!owner->IsTeleporting())
		{
			owner->SetPosition(Player::Instance().launchKnockbackPosition);
			owner->GetModel()->PlayRootMotion(animationIndexes[0], true, true, owner->GetBlendSeconds(), "root");
		}
		if (intervalTimer >= intervalDuration)
		{
			intervalTimer = 0.0f;
			step++; // 次の放物線ジャンプへ
		}
		break;
	case 2:
		owner->TurnToTarget(elapsedTime, 1000);
		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
		{
			owner->SetGravity(-1.5f);
			owner->GetModel()->PlayRootMotion(animationIndexes[1], false, true, owner->GetBlendSeconds(), "root");
			step++;
		}
		break;
	case 3:
		if (frame >= config->advanceInputEndFrame)
		{
			step = 0;
			owner->SetGravity(-0.3f);
			owner->SetSuperArmor(false);
			owner->SetRevengeState(false);
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsAnyDamage() || owner->IsDeathFlag())
	{
		step = 0;
		owner->SetGravity(-0.3f);
		return ActionBase<ActorType>::State::Failed;
	}

	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 強制反撃2
template <typename ActorType>
typename ActionBase<ActorType>::State RevengeAssault<ActorType>::Run(float elapsedTime)
{
	float frame = owner->GetModel()->GetCurrentAnimationSeconds();
	int index = owner->GetModel()->GetCurrentAnimationIndex();
	AnimationConfig* config = owner->GetModel()->GetAnimationConfig("EnemyBoss", index);
	owner->SetSuperArmor(true);
	owner->SetTargetPosition(Player::Instance().GetPosition());
	if (!owner->isPlayerInvincible)
	{
		owner->GetSword()->AttackAnimationCollision(owner->GetModel(), config, owner->GetCharacter());
	}

	switch (step)
	{
	case 0:
		// テレポート先を決定
		teleportPosition = owner->CalculateVisibleTeleportPos(3.0f);

		owner->SetGravity(0.0f);
		owner->GetModel()->PlayRootMotion(animationIndexes[0], false, true, owner->GetBlendSeconds(), "root");
		step++;
		break;
	case 1:
		// アニメーションが進んだらテレポート開始
		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
		{
			// スムーズテレポート開始（0.3秒かけて移動）
			owner->StartTeleport(
				teleportPosition,
				0.2f
			);
			step++;
		}
		break;
	case 2:
		owner->MoveToTarget(elapsedTime, 0.1f);
		if (!owner->IsTeleporting())
		{
			owner->GetModel()->PlayRootMotion(animationIndexes[1], false, true, owner->GetBlendSeconds(), "root");
			if (owner->GetHealth() <= (owner->GetMaxHealth() * 0.5) && spawnedProjectiles.empty())
			{
				for (int i = 0; i < 2; i++)
				{
					ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
					info.owner = owner;
					info.moveType = MovementType::Stationary;
					info.scale = 0.5f;
					info.radius = 0.7f;
					info.lifeTime = 5.0f;
					info.speed = 40.0f;

					if (i == 0)
					{
						info.spawnPosition = {
							owner->GetPosition().x + (owner->CharacterBack(owner->GetAngle()).x - owner->CharacterLeft(owner->GetAngle()).x) * 3.0f,
							-1,
							owner->GetPosition().z + (owner->CharacterBack(owner->GetAngle()).z - owner->CharacterLeft(owner->GetAngle()).z) * 2.0f
						};
					}
					else
					{
						info.spawnPosition = {
							owner->GetPosition().x + (owner->CharacterBack(owner->GetAngle()).x - owner->CharacterRight(owner->GetAngle()).x) * 3.0f,
							-1,
							owner->GetPosition().z + (owner->CharacterBack(owner->GetAngle()).z - owner->CharacterRight(owner->GetAngle()).z) * 2.0f
						};
					}
					Projectile* p = ProjectileManager::Instance().Launch(info);
					if (p) spawnedProjectiles.push_back(p);
				}
			}
			step++;
		}
		break;
	case 3:
		owner->TurnToTarget(elapsedTime, 1000);
		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
		{
			owner->SetGravity(-1.5f);
			owner->GetModel()->PlayRootMotion(animationIndexes[2], false, true, owner->GetBlendSeconds(), "root");
			step++;
		}
		break;
	case 4:
		if (frame >= config->advanceInputStartFrame)
		{
			for (auto* p : spawnedProjectiles)
			{
				if (p && p->IsActive())
				{
					// Projectile側でタイプをSpiralに変更
					p->FireAt(
						Player::Instance().GetPosition(),  // 回転速度 (ラジアン/秒)
						20.0f,   // 拡散速度 (メートル/秒)
						true
					);
				}
			}
			step++;
			spawnedProjectiles.clear();
		}
		
		break;
	case 5:
		if (frame >= config->advanceInputEndFrame)
		{
			spawnedProjectiles.clear();
			step = 0;
			owner->SetGravity(-0.3f);
			owner->SetSuperArmor(false);
			owner->SetRevengeState(false);
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsAnyDamage() || owner->IsDeathFlag())
	{
		step = 0;
		owner->SetGravity(-0.3f);
		owner->SetRevengeState(false);
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
		int randomIndex = Mathf::RandomRange(0, 1);
		owner->GetModel()->PlayRootMotion(animationIndexes[randomIndex], true, true, owner->GetBlendSeconds(), "root");
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
	if (owner->IsAnyDamage() || owner->IsDeathFlag())
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
		owner->GetModel()->PlayRootMotion(animationIndex, true, true, owner->GetBlendSeconds(), "root");
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
	if (owner->IsAnyDamage() || owner->IsDeathFlag())
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
	if (owner->IsDamage() || owner->IsDeathFlag())
	{
		step = 0;
		return ActionBase<ActorType>::State::Failed;
	}
	// 実行中を返す
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 落下モーション
template <typename ActorType>
typename ActionBase<ActorType>::State FallAction<ActorType>::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		if (!owner->IsGround())
		{
			owner->GetModel()->PlayRootMotion(animationIndex1, true, true, owner->GetBlendSeconds(), "root");
		}
		if (owner->IsGround())
		{
			owner->GetModel()->PlayRootMotion(animationIndex2, false, true, owner->GetBlendSeconds(), "root");
			step++;
		}
		break;
	case 1:
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step = 0;
			return ActionBase<ActorType>::State::Complete;
		}
		break;
	}
	if (owner->IsDamage() || owner->IsDeathFlag())
	{
		step = 0;
		return ActionBase<ActorType>::State::Complete;
	}
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
	if (owner->IsAnyDamage() || owner->GetRevengeState() || owner->IsDeathFlag())
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
	if (handle == -1)
	{
		handle = owner->deathEffect->Play({ owner->GetPosition().x, owner->GetPosition().y + 1.0f, owner->GetPosition().z }, 0.5f);
	}
	owner->deathEffect->SetPosition(handle, { owner->GetPosition().x, owner->GetPosition().y + 1.0f, owner->GetPosition().z });
	// 実行中を返す
	return ActionBase<ActorType>::State::Run;
}
//-------------------------------------------------------------

