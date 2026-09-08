// AttackActions.h
#pragma once
#include "EnemyActionBase.h"
#include "Character/Player/Player.h"
#include "Character/Projectile/ProjectileManager.h"
#include "Math/Mathf.h"
#include "Effect/EffectManager.h"
#include <algorithm>

using namespace DirectX;
using namespace DirectX::SimpleMath;

// 斬撃コンボ1
template <typename ActorType>
class SlashCombo1Action : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	SlashCombo1Action(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	int animationIndexes[4] = { 
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_01_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_02_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_02_03_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_02_04_Seq_0") 
	};
	int attackCount = 0;
	bool init = false;
	float timer = 0.0f;
	float duration = 1.0f;
	// 4 段目を振り抜く間合い。
	float finisherReach = EnemyActionBase<ActorType>::MELEE_REACH;

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();
		float frame = this->GetAnimationFrame();
		AnimationConfig* config = this->GetCurrentAnimationConfig();

		this->UpdateAttackCollision();
		this->owner->SetTargetPosition(this->PlayerPosition());

		switch (this->step)
		{
		case 0:
			this->PlayRootMotion(animationIndexes[attackCount++], false);
			this->step++;
			break;

		case 1:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::INSTANT);
			if (frame >= config->advanceInputEndFrame)
			{
				this->PlayRootMotion(animationIndexes[attackCount++], false);
			}
			if (attackCount >= 4)
			{
				init = false;
				this->step++;
			}
			break;

		case 2:
		{
			if (!init) { init = true; timer = 0.0f; }
			timer += elapsedTime;
			float t = std::clamp(timer / duration, 0.0f, 1.0f);

			this->owner->TurnToTarget(elapsedTime, TurnSpeed::INSTANT);

			if (frame >= config->advanceInputStartFrame)
			{
				model->PauseAnimation(true);

				if (this->IsWithinReach(finisherReach))
				{
					model->PauseAnimation(false);
					this->step++;
				}
				else
				{
					this->LerpTowardPlayer(t);
				}
			}
			break;
		}

		case 3:
			if (!model->IsPlayAnimation()) return this->ResetState(State::Complete);
			break;
		}

		if (this->IsInterrupted())
		{
			// 止めたまま抜けると、以降アニメーションが動かなくなる
			model->PauseAnimation(false);
			return this->ResetState(State::Failed);
		}
		return State::Run;
	}

private:
	void OnReset() override
	{
		attackCount = 0;
		init = false;
		timer = 0.0f;
	}
};

// 斬撃波
template <typename ActorType>
class SlashWave : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	SlashWave(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	int animationIndexes[4] = { 
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_01_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_02_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_03_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_04_Seq_0") 
	};
	int attackCount = 0;

private:
	bool hasShot = false;

public:
	State Run(float elapsedTime) override
	{
		this->owner->SetTargetPosition(this->PlayerPosition());
		this->owner->TurnToTarget(elapsedTime, TurnSpeed::INSTANT);

		auto* model = this->GetModel();
		float frame = this->GetAnimationFrame();
		AnimationConfig* config = this->GetCurrentAnimationConfig();

		this->UpdateAttackCollision();

		switch (this->step)
		{
		case 0:
			model->PlayAnimation(animationIndexes[attackCount % 4], false);
			hasShot = false;
			this->step++;
			break;

		case 1:
			if (!hasShot && frame >= config->advanceInputStartFrame)
			{
				ProjectileInfo info = ProjectileManager::GetSlashWaveInfo();
				info.owner = this->owner;
				XMFLOAT3 bossPos = this->owner->GetPosition();
				XMFLOAT3 bossForward = this->owner->CharacterForward(this->owner->GetAngle());
				
				info.spawnPosition = { bossPos.x + bossForward.x * 1.5f, bossPos.y, bossPos.z + bossForward.z * 1.5f };
				info.direction = bossForward;

				if (Projectile* p = ProjectileManager::Instance().Launch(info))
					p->FireAt(this->PlayerPosition(), info.speed, false);

				hasShot = true;
			}

			if (frame >= config->advanceInputEndFrame)
			{
				attackCount++;
				this->step = (attackCount >= 4) ? 2 : 0;
			}
			break;

		case 2:
			if (!model->IsPlayAnimation()) return this->ResetState(State::Complete);
			break;
		}

		if (this->IsInterrupted()) return this->ResetState(State::Failed);
		return State::Run;
	}

private:
	void OnReset() override
	{
		attackCount = 0;
		hasShot = false;
	}
};

// 突進斬り
template <typename ActorType>
class DashSlashAction : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	DashSlashAction(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	int animationIndex = this->owner->GetModel()->GetAnimationIndex("Run_Attack_01_Seq_0");
	bool init = false;
	float timer = 0.0f;
	float duration = 7.0f;

	float approachReach = EnemyActionBase<ActorType>::MELEE_REACH;

	Vector3 teleportPosition;

	State Run(float elapsedTime) override
	{
		this->owner->SetTargetPosition(this->PlayerPosition());
		if (!init) { init = true; timer = 0.0f; }

		auto* model = this->GetModel();
		float frame = this->GetAnimationFrame();
		AnimationConfig* config = this->GetAnimationConfig(animationIndex);

		this->UpdateAttackCollision();

		timer += elapsedTime;
		float t = std::clamp(timer / duration, 0.0f, 1.0f);

		if (!this->owner->GetPlayedEffect())
		{
			this->owner->attackSign->Play({ this->owner->GetPosition().x, this->owner->GetPosition().y + 1.0f, this->owner->GetPosition().z });
			this->owner->SetPlayedEffect(true);
		}

		switch (this->step)
		{
		case 0:
			if (this->behaviorData->IsInSequence())
			{
				teleportPosition = this->owner->CalculateVisibleTeleportPos(15.0f);
				duration = 15.0f;
			}
			this->PlayRootMotion(animationIndex, false);
			this->owner->SetGravity(-0.001f);
			this->step++;
			break;

		case 1:
			if (frame >= config->advanceInputStartFrame) model->PauseAnimation(true);
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);

			if (!this->owner->IsTeleporting())
			{
				this->LerpTowardPlayer(t);
			}

			if (this->IsWithinReach(approachReach))
			{
				if (this->behaviorData->IsInSequenceAndNotLast() && !this->owner->IsTeleporting())
				{
					this->owner->StartTeleport(teleportPosition, 0.1f);
					return this->ResetState(State::Complete);
				}
				if ((!this->behaviorData->IsInSequence() || this->behaviorData->IsLastNodeInSequence()) && !this->owner->IsTeleporting())
				{
					this->owner->attackSign->Play({ this->owner->GetPosition().x, this->owner->GetPosition().y + 1.0f, this->owner->GetPosition().z });
					model->PauseAnimation(false);
					this->step++;
				}
			}
			break;

		case 2:
			if (!model->IsPlayAnimation() || (!this->owner->IsGround() && frame >= config->advanceInputEndFrame))
			{
				duration = 3.0f;
				return this->ResetState(State::Complete);
			}
			break;
		}

		if (this->IsInterrupted())
		{
			model->PauseAnimation(false);
			return this->ResetState(State::Failed);
		}
		return State::Run;
	}

private:
	void OnReset() override
	{
		this->owner->SetGravity(-0.3f);
		init = false;
		this->owner->SetPlayedEffect(false);
		timer = 0;
	}
};

// 光柱円形収束
template <typename ActorType>
class PillarSpiralConv : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	PillarSpiralConv(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	int animationIndexes[3] = { 
		this->owner->GetModel()->GetAnimationIndex("Power_Attack_Start_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Power_Attack_Loop_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_02_Seq_0") 
	};
	// 円状に並べる光柱の本数と半径
	static constexpr int PILLAR_COUNT = 8;
	static constexpr float CIRCLE_RADIUS = 3.0f;

	std::vector<Projectile*> spawnedProjectiles;
	float waitTimer = 0.0f;
	const float waitDuration = 1.5f;
	float recoveryTimer = 0.0f;
	const float recoveryDuration = 0.75f;
	const float circleWaitTime = 2.5f;
	float lockOnTime = 0.3f;
	float currentRotationAngle = 0.0f;

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();

		switch (this->step)
		{
		case 0:
			this->PlayRootMotion(animationIndexes[0], false, 0.2f);
			this->step++;
			break;

		case 1:
		{
			this->PlayRootMotion(animationIndexes[1], true, 0.2f);
			spawnedProjectiles.clear();
			
			// プレイヤーを囲むように光柱を円状に配置する
			XMFLOAT3 playerPos = this->PlayerPosition();
			XMFLOAT3 center = { playerPos.x, this->owner->GetPosition().y, playerPos.z };
			this->SpawnLightPillarCircle(spawnedProjectiles, center,
				PILLAR_COUNT, CIRCLE_RADIUS, 0.4f, 0.5f, 8.0f, 0.5f, false);
			waitTimer = 0.0f;
			this->step++;
			break;
		}

		case 2:
		{
			waitTimer += elapsedTime;
			if (waitTimer < circleWaitTime - lockOnTime)
			{
				currentRotationAngle += 1.0f * elapsedTime;
				XMFLOAT3 playerPos = Player::Instance().GetPosition();
				int count = (int)spawnedProjectiles.size();

				for (int i = 0; i < count; ++i)
				{
					if (Projectile* p = spawnedProjectiles[i]; p && p->IsActive())
					{
						float angle = (XM_2PI * (float)i / count) + currentRotationAngle;
						p->SetPosition({ playerPos.x + cosf(angle) * CIRCLE_RADIUS, this->owner->GetPosition().y, playerPos.z + sinf(angle) * CIRCLE_RADIUS });
					}
				}
			}
			if (waitTimer >= circleWaitTime) this->step++;
			break;
		}

		case 3:
			this->PlayRootMotion(animationIndexes[2], false, 0.2f);
			this->FireAndClear(spawnedProjectiles, 20.0f, true);
			
			waitTimer += elapsedTime;
			if (waitTimer >= waitDuration) this->step++;
			break;

		case 4:
			if (!model->IsPlayAnimation()) this->step++;
			break;

		case 5:
			model->PlayAnimation(model->GetAnimationIndex("Idle_Combat_Seq_0"), true, 0.2f);
			recoveryTimer += elapsedTime;
			if (recoveryTimer >= recoveryDuration) return this->ResetState(State::Complete);
			break;
		}

		if (this->IsInterrupted()) return this->ResetState(State::Failed);
		return State::Run;
	}

private:
	void OnReset() override
	{
		waitTimer = 0.0f;
		recoveryTimer = 0.0f;
		spawnedProjectiles.clear();
	}
};

// 光柱円形拡散
template <typename ActorType>
class PillarSpiralDiff : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	PillarSpiralDiff(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	int animationIndexes[3] = { 
		this->owner->GetModel()->GetAnimationIndex("Power_Attack_Start_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Power_Attack_Loop_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_02_Seq_0") 
	};
	// 円状に並べる光柱の本数と初期半径
	static constexpr int PILLAR_COUNT = 8;
	static constexpr float INITIAL_RADIUS = 2.0f;

	std::vector<Projectile*> spawnedProjectiles;
	float waitTimer = 0.0f;
	const float waitDuration = 1.5f;
	float recoveryTimer = 0.0f;
	const float recoveryDuration = 2.0f;

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();

		switch (this->step)
		{
		case 0:
			this->PlayRootMotion(animationIndexes[0], false, 0.2f);
			this->step++;
			break;

		case 1:
			this->PlayRootMotion(animationIndexes[1], true, 0.2f);
			if (spawnedProjectiles.empty())
			{
				// 自分を中心に光柱を円状に配置し、外へ広がる軌道情報を持たせる
				this->SpawnLightPillarCircle(spawnedProjectiles, this->owner->GetPosition(),
					PILLAR_COUNT, INITIAL_RADIUS, 0.5f, 0.7f, 5.0f, 1.0f, true);
			}

			waitTimer += elapsedTime;
			if (waitTimer >= waitDuration) this->step++;
			break;

		case 2:
			for (auto* p : spawnedProjectiles)
			{
				if (p && p->IsActive())
				{
					this->PlayRootMotion(animationIndexes[3], false, 0.2f); // NOTE: index bounds check needed if out of range, original code used 3.
					p->StartSpiral(3.0f, 5.0f);
				}
			}
			spawnedProjectiles.clear();
			this->step++;
			break;

		case 3:
			if (!model->IsPlayAnimation()) this->step++;
			break;

		case 4:
			model->PlayAnimation(model->GetAnimationIndex("Idle_Combat_Seq_0"), true, 0.2f);
			recoveryTimer += elapsedTime;
			if (recoveryTimer >= recoveryDuration) return this->ResetState(State::Complete);
			break;
		}

		if (this->IsInterrupted()) return this->ResetState(State::Failed);
		return State::Run;
	}

private:
	void OnReset() override
	{
		waitTimer = 0.0f;
		recoveryTimer = 0.0f;
		spawnedProjectiles.clear();
	}
};

// テレポート強襲
template <typename ActorType>
class TelePortAssault : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	TelePortAssault(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	int animationIndexes[3] = { 
		this->owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_03_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0") 
	};
	Vector3 teleportPosition;
	std::vector<Projectile*> spawnedProjectiles;

	float teleportDistance = EnemyActionBase<ActorType>::MELEE_REACH;

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();
		float frame = this->GetAnimationFrame();
		AnimationConfig* config = this->GetCurrentAnimationConfig();

		this->UpdateAttackCollision();

		this->owner->SetTargetPosition(this->PlayerPosition());

		switch (this->step)
		{
		case 0:
			teleportPosition = this->owner->CalculateVisibleTeleportPos(teleportDistance);
			this->owner->SetGravity(-0.001f);
			this->PlayRootMotion(animationIndexes[0], false);
			this->step++;
			break;

		case 1:
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->StartTeleport(teleportPosition, 0.1f);
				this->step++;
			}
			break;

		case 2:
			if (!this->owner->IsTeleporting()) this->step++;
			break;

		case 3:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::INSTANT);
			this->PlayRootMotion(animationIndexes[1], false);
			
			if (this->IsHealthBelowRate(0.5f) && this->behaviorData->IsLastNodeInSequence() && spawnedProjectiles.empty())
			{
				this->SpawnSideLightPillars(spawnedProjectiles);
			}
			this->step++;
			break;

		case 4:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::INSTANT);
			if (frame >= config->advanceInputEndFrame)
			{
				this->owner->SetGravity(-0.3f);
				this->PlayRootMotion(animationIndexes[2], false);
				this->step++;
			}
			break;

		case 5:
			if (frame >= config->advanceInputStartFrame)
			{
				if (this->behaviorData->IsLastNodeInSequence())
				{
					this->FireAndClear(spawnedProjectiles, 20.0f, true);
				}
				this->step++;
			}
			else this->owner->MoveToTarget(elapsedTime, 2);
			break;

		case 6:
			if (this->behaviorData->IsInSequenceAndNotLast() && frame >= config->advanceInputStartFrame) return this->ResetState(State::Complete);

			if (!model->IsPlayAnimation() || (frame >= config->advanceInputEndFrame && !this->owner->IsGround()))
			{
				return this->ResetState(State::Complete);
			}
			break;
		}

		if (this->IsInterrupted()) return this->ResetState(State::Failed);
		return State::Run;
	}

private:
	void OnReset() override
	{
		this->owner->SetGravity(-0.3f);
	}
};

// テレポートコンボ
template <typename ActorType>
class TeleportCombo : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	TeleportCombo(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	int attackCount = 0;
	int teleportAnimationIndex = this->owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0");
	int comboAnimationIndexes[5] = { 
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_03_01_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_03_02_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Attack_Up_01_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_Loop_02_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_End_02_Seq_0") 
	};
	Vector3 teleportPosition;

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();
		float frame = this->GetAnimationFrame();
		AnimationConfig* config = this->GetCurrentAnimationConfig();
		
		this->owner->SetTargetPosition(this->PlayerPosition());
		this->UpdateAttackCollision();

		switch (this->step)
		{
		case 0:
			this->PlayRootMotion(teleportAnimationIndex, false);
			this->step++;
			break;

		case 1:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				teleportPosition = this->owner->CalculateVisibleTeleportPos((attackCount < 3) ? 2.0f : 1.5f);
				if (attackCount >= 3) teleportPosition.y += 3.0f;

				this->owner->SetGravity(0.0f);
				this->owner->StartTeleport(teleportPosition, 0.1f);
				this->step++;
			}
			break;

		case 2:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);
			if (!this->owner->IsTeleporting()) this->step++;
			break;

		case 3:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);
			if (!this->owner->IsTeleporting())
			{
				this->PlayRootMotion(comboAnimationIndexes[attackCount++], false);
				if (frame >= config->advanceInputEndFrame) this->step = (attackCount < 4) ? 1 : this->step + 1;
			}
			break;

		case 4:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->SetGravity(-1.5f);
				this->PlayRootMotion(comboAnimationIndexes[attackCount], false);
				this->step++;
			}
			break;

		case 5:
			if (frame >= config->advanceInputEndFrame) return this->ResetState(State::Complete);
			break;
		}

		if (this->IsInterrupted()) return this->ResetState(State::Failed);
		return State::Run;
	}

private:
	void OnReset() override
	{
		attackCount = 0;
		this->owner->SetGravity(-0.3f);
	}
};

// 必殺技
template <typename ActorType>
class SpecialAttack : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	SpecialAttack(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	int animationIndexes[5] = { 
		this->owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Power_Attack_Start_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Power_Attack_Loop_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Knock_Down_Combat_Loop_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Knock_Down_Combat_End_Seq_0") 
	};
	std::vector<Projectile*> groupSpiral1, groupSpiral2, groupHoming;
	
	float waitTimer = 0.0f;
	const float waitDuration = 1.0f;
	float recoveryTimer = 0.0f;
	const float recoveryDuration = 6.0f;
	bool playedEffect = false;
	Effekseer::Handle handle = -1;
	
	int shotCount = 0;
	float shotIntervalTimer = 0.0f;
	const float shotInterval = 0.5f;
	int waveCount = 0;
	const float circleWaitTime = 1.5f;
	float lockOnTime = 0.3f;
	float currentRotationAngle = 0.0f;

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();
		float frame = this->GetAnimationFrame();
		AnimationConfig* config = this->GetCurrentAnimationConfig();

		if (this->step >= 2 && this->step <= 11)
		{
			for (int i = 0; i < 5; ++i)
			{
				float angle = (float)rand() / RAND_MAX * XM_2PI;
				float dist = 1.5f + ((float)rand() / RAND_MAX) * 2.5f;
				float height = ((float)rand() / RAND_MAX) * 4.0f;

				XMFLOAT3 bossPos = this->owner->GetPosition();
				Vector3 emitPos(bossPos.x + cosf(angle) * dist, bossPos.y + height, bossPos.z + sinf(angle) * dist);
				XMFLOAT3 velocity = { ((float)rand() / RAND_MAX - 0.5f) * 0.5f, 1.0f + ((float)rand() / RAND_MAX) * 2.0f, ((float)rand() / RAND_MAX - 0.5f) * 0.5f };
				
				EffectManager::Instance().EmitGpuParticle(emitPos, velocity, { 1.0f, 0.9f, 0.2f, 1.0f }, 0.5f + ((float)rand() / RAND_MAX) * 0.2f, 0.4f + ((float)rand() / RAND_MAX) * 0.4f, 2);
			}
		}

		switch (this->step)
		{
		case 0:
			this->PlayRootMotion(animationIndexes[0], false);
			this->step++;
			break;

		case 1:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->SetGravity(0.0f);
				this->owner->StartTeleport({ -0.37f, -1.460f, 0.35f }, 0.1f);
				this->step++;
			}
			break;

		case 2:
			if (!this->owner->IsTeleporting() && !playedEffect)
			{
				handle = this->owner->magicCircle->Play(this->owner->GetPosition(), 5.0f);
				this->PlayRootMotion(animationIndexes[2], true, 0.2f);
				playedEffect = true;
			}
			waitTimer += elapsedTime;
			if (waitTimer >= waitDuration) { waitTimer = 0; this->step++; }
			break;

		case 3:
			if (!this->owner->IsTeleporting())
			{
				if (groupSpiral1.empty())
				{
					int count = 6;
					XMFLOAT3 center = this->owner->GetPosition();

					for (int i = 0; i < count; ++i)
					{
						ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
						info.owner = this->owner;
						info.moveType = MovementType::Stationary;
						info.scale = 0.4f;
						info.radius = 0.5f;
						info.lifeTime = 10.0f;
						info.centerPosition = center;
						info.invincibleTime = 1.0f;

						float angle = XM_2PI * (float)i / count;
						info.currentAngle = angle;
						
						info.currentRadius = 2.0f;
						info.spawnPosition = { center.x + cosf(angle) * 2.0f, center.y, center.z + sinf(angle) * 2.0f };
						if (Projectile* p = ProjectileManager::Instance().Launch(info)) groupSpiral1.push_back(p);

						info.currentRadius = 6.0f;
						info.spawnPosition = { center.x + cosf(angle) * 6.0f, center.y, center.z + sinf(angle) * 6.0f };
						if (Projectile* p = ProjectileManager::Instance().Launch(info)) groupSpiral2.push_back(p);
					}
				}
				waitTimer += elapsedTime;
				if (waitTimer >= waitDuration) this->step++;
			}
			break;

		case 4:
			for (auto* p : groupSpiral1) if (p && p->IsActive()) p->StartSpiral(1.0f, 2.0f);
			for (auto* p : groupSpiral2) if (p && p->IsActive()) p->StartSpiral(-1.0f, 2.0f);
			groupSpiral1.clear(); groupSpiral2.clear();
			this->step++;
			break;

		case 5:
			shotCount = 0;
			shotIntervalTimer = shotInterval;
			groupHoming.clear();
			this->step++;
			break;

		case 6:
			shotIntervalTimer += elapsedTime;
			if (shotIntervalTimer >= shotInterval)
			{
				shotIntervalTimer = 0.0f;
				ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
				info.owner = this->owner;
				info.scale = 0.4f;
				info.radius = 0.5f;
				info.lifeTime = 5.0f;

				Vector3 spawnPos = this->owner->CalculateVisibleTeleportPos(20.0f);
				spawnPos.y = this->owner->GetPosition().y;
				info.spawnPosition = spawnPos;

				if (Projectile* p = ProjectileManager::Instance().Launch(info))
					p->FireAt(this->PlayerPosition(), 25.0f, true);

				if (++shotCount >= 6) this->step++;
			}
			break;

		case 7:
			waitTimer = 0.0f;
			this->step++;
			break;

		case 8:
		{
			groupHoming.clear();
			int count = 8;
			float radius = 3.0f;
			XMFLOAT3 playerPos = Player::Instance().GetPosition();

			for (int i = 0; i < count; ++i)
			{
				ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
				info.owner = this->owner;
				info.moveType = MovementType::Stationary;
				info.scale = 0.4f;
				info.radius = 0.5f;
				info.lifeTime = 8.0f;
				info.invincibleTime = 0.5f;

				float angle = XM_2PI * (float)i / count;
				info.spawnPosition = { playerPos.x + cosf(angle) * radius, this->owner->GetPosition().y, playerPos.z + sinf(angle) * radius };

				if (Projectile* p = ProjectileManager::Instance().Launch(info)) groupHoming.push_back(p);
			}
			waitTimer = 0.0f;
			this->step++;
			break;
		}

		case 9:
			waitTimer += elapsedTime;
			if (waitTimer < circleWaitTime - lockOnTime)
			{
				currentRotationAngle += 1.0f * elapsedTime;
				XMFLOAT3 playerPos = Player::Instance().GetPosition();

				for (int i = 0; i < (int)groupHoming.size(); ++i)
				{
					if (Projectile* p = groupHoming[i]; p && p->IsActive())
					{
						float angle = (XM_2PI * (float)i / groupHoming.size()) + currentRotationAngle;
						p->SetPosition({ playerPos.x + cosf(angle) * 3.0f, this->owner->GetPosition().y, playerPos.z + sinf(angle) * 3.0f });
					}
				}
			}
			if (waitTimer >= circleWaitTime) this->step++;
			break;

		case 10:
			for (auto* p : groupHoming) if (p && p->IsActive()) p->FireAt(this->PlayerPosition(), 20.0f, true);
			groupHoming.clear();

			if (++waveCount < 2) { waitTimer = 0.0f; this->step = 11; }
			else this->step = 12;
			break;

		case 11:
			waitTimer += elapsedTime;
			if (waitTimer >= 1.0f) this->step = 5;
			break;

		case 12:
			this->PlayRootMotion(animationIndexes[3], true, 0.2f);
			this->owner->magicCircle->Stop(handle);
			handle = -1;
			recoveryTimer += elapsedTime;
			if (recoveryTimer >= recoveryDuration)
			{
				this->PlayRootMotion(animationIndexes[4], true, 0.2f);
				return this->ResetState(State::Complete);
			}
			break;
		}

		if (this->IsInterrupted()) return this->ResetState(State::Failed);
		return State::Run;
	}

private:
	void OnReset() override
	{
		playedEffect = false;
		waitTimer = 0.0f;
		recoveryTimer = 0.0f;
		shotCount = 0;
		waveCount = 0;
		groupHoming.clear();
		groupSpiral1.clear();
		groupSpiral2.clear();
		this->owner->SetGravity(-0.3f);
		this->owner->SetSpecialReady(false);
	}
};

// 強制反撃1
template <typename ActorType>
class RevengeDive : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	RevengeDive(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	float intervalTimer = 0.0f;
	const float intervalDuration = 0.7f;
	int animationIndexes[2] = { 
		this->owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_Loop_02_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_End_02_Seq_0") 
	};

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();
		float frame = this->GetAnimationFrame();
		AnimationConfig* config = this->GetCurrentAnimationConfig();
		
		this->owner->SetTargetPosition(this->PlayerPosition());		
		this->UpdateAttackCollision();

		switch (this->step)
		{
		case 0:
			this->owner->SetGravity(0.0f);
			this->owner->StartTeleport(Player::Instance().launchKnockbackPosition, 0.1f);
			this->step++;
			break;

		case 1:
			intervalTimer += elapsedTime;
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);
			if (!this->owner->IsTeleporting())
			{
				this->owner->SetPosition(Player::Instance().launchKnockbackPosition);
				this->PlayRootMotion(animationIndexes[0], true);
			}
			if (intervalTimer >= intervalDuration)
			{
				intervalTimer = 0.0f;
				this->step++;
			}
			break;

		case 2:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->SetGravity(-1.5f);
				this->PlayRootMotion(animationIndexes[1], false);
				this->step++;
			}
			break;

		case 3:
			if (frame >= config->advanceInputEndFrame)
			{
				this->step = 0;
				this->owner->SetGravity(-0.3f);
				this->owner->SetRevengeState(false);
				return State::Complete;
			}
			break;
		}
		return State::Run;
	}
};

// 強制反撃2
template <typename ActorType>
class RevengeAssault : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	RevengeAssault(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	float intervalTimer = 0.0f;
	const float intervalDuration = 0.7f;
	int animationIndexes[3] = { 
		this->owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_03_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0") 
	};
	Vector3 teleportPosition;
	std::vector<Projectile*> spawnedProjectiles;

	// テレポート先をプレイヤーからどれだけ離すか。
	float teleportDistance = EnemyActionBase<ActorType>::MELEE_REACH;

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();
		float frame = this->GetAnimationFrame();
		AnimationConfig* config = this->GetCurrentAnimationConfig();
		
		this->owner->SetSuperArmor(true);
		this->owner->SetTargetPosition(this->PlayerPosition());
		
		this->UpdateAttackCollision();

		switch (this->step)
		{
		case 0:
			teleportPosition = this->owner->CalculateVisibleTeleportPos(teleportDistance);
			this->owner->SetGravity(0.0f);
			this->PlayRootMotion(animationIndexes[0], false);
			this->step++;
			break;

		case 1:
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->StartTeleport(teleportPosition, 0.2f);
				this->step++;
			}
			break;

		case 2:
			this->owner->MoveToTarget(elapsedTime, 0.1f);
			if (!this->owner->IsTeleporting())
			{
				this->PlayRootMotion(animationIndexes[1], false);
				
				if (this->IsHealthBelowRate(0.5f) && spawnedProjectiles.empty())
				{
					this->SpawnSideLightPillars(spawnedProjectiles);
				}
				this->step++;
			}
			break;

		case 3:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->SetGravity(-1.5f);
				this->PlayRootMotion(animationIndexes[2], false);
				this->step++;
			}
			break;

		case 4:
			if (frame >= config->advanceInputStartFrame)
			{
				this->FireAndClear(spawnedProjectiles, 20.0f, true);
				this->step++;
			}
			break;

		case 5:
			if (frame >= config->advanceInputEndFrame)
			{
				spawnedProjectiles.clear();
				this->step = 0;
				this->owner->SetGravity(-0.3f);
				this->owner->SetSuperArmor(false);
				this->owner->SetRevengeState(false);
				return State::Complete;
			}
			break;
		}
		return State::Run;
	}
};