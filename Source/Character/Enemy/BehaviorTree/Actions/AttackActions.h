// AttackActions.h
#pragma once
#include "BehaviorTree/ActionBase.h"
#include "Character/Player/Player.h"
#include "Character/Projectile/ProjectileManager.h"
#include "Math/Mathf.h"
#include "Effect/EffectManager.h"
#include <algorithm>

using namespace DirectX;
using namespace DirectX::SimpleMath;

// 斬撃コンボ1
template <typename ActorType>
class SlashCombo1Action : public ActionBase<ActorType>
{
public:
	SlashCombo1Action(ActorType* actor) : ActionBase<ActorType>(actor) {}

	int animationIndexs[4] = { 
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_01_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_02_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_02_03_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_02_04_Seq_0") 
	};
	int AttackCount = 0;
	bool init = false;
	float timer = 0.0f;
	float duration = 1.0f;
	Vector3 epsilon{ 2.5f, 1.0f, 2.5f };

	typename ActionBase<ActorType>::State Run(float elapsedTime) override
	{
		auto* model = this->owner->GetModel();
		int index = model->GetCurrentAnimationIndex();
		float frame = model->GetCurrentAnimationSeconds();
		AnimationConfig* config = model->GetAnimationConfig("EnemyBoss", index);

		this->owner->GetSword()->AttackAnimationCollision(model, config, this->owner->GetCharacter());
		this->owner->SetTargetPosition(Player::Instance().GetPosition());

		switch (this->step)
		{
		case 0:
			model->PlayRootMotion(animationIndexs[AttackCount++], false, true, this->owner->GetBlendSeconds(), "root");
			this->step++;
			break;

		case 1:
			this->owner->TurnToTarget(elapsedTime, 10000);
			if (frame >= config->advanceInputEndFrame)
			{
				model->PlayRootMotion(animationIndexs[AttackCount++], false, true, this->owner->GetBlendSeconds(), "root");
			}
			if (AttackCount >= 4)
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

			this->owner->TurnToTarget(elapsedTime, 10000);
			if (frame >= config->advanceInputStartFrame)
			{
				auto playerPos = Player::Instance().GetPosition();
				if (!XMVector3NearEqual(XMLoadFloat3(&this->owner->GetPosition()), XMLoadFloat3(&playerPos), epsilon))
				{
					this->owner->SetPosition({
						Mathf::Lerp(this->owner->GetPosition().x, playerPos.x, t),
						Mathf::Lerp(this->owner->GetPosition().y, playerPos.y, t),
						Mathf::Lerp(this->owner->GetPosition().z, playerPos.z, t)
					});
				}
				else this->step++;
			}
			break;
		}

		case 3:
			if (!model->IsPlayAnimation()) return ResetState(ActionBase<ActorType>::State::Complete);
			break;
		}

		if (this->owner->IsAnyDamage() || this->owner->IsDeathFlag()) return ResetState(ActionBase<ActorType>::State::Failed);
		return ActionBase<ActorType>::State::Run;
	}

private:
	typename ActionBase<ActorType>::State ResetState(typename ActionBase<ActorType>::State state)
	{
		this->step = 0;
		AttackCount = 0;
		init = false;
		timer = 0.0f;
		return state;
	}
};

// 斬撃波
template <typename ActorType>
class SlashWave : public ActionBase<ActorType>
{
public:
	SlashWave(ActorType* actor) : ActionBase<ActorType>(actor) {}

	int animationIndexes[4] = { 
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_01_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_02_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_03_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_04_Seq_0") 
	};
	int AttackCount = 0;

private:
	bool hasShot = false;

public:
	typename ActionBase<ActorType>::State Run(float elapsedTime) override
	{
		this->owner->SetTargetPosition(Player::Instance().GetPosition());
		this->owner->TurnToTarget(elapsedTime, 10000);

		auto* model = this->owner->GetModel();
		int index = model->GetCurrentAnimationIndex();
		float frame = model->GetCurrentAnimationSeconds();
		AnimationConfig* config = model->GetAnimationConfig("EnemyBoss", index);

		if (!this->owner->isPlayerInvincible)
			this->owner->GetSword()->AttackAnimationCollision(model, config, this->owner->GetCharacter());

		switch (this->step)
		{
		case 0:
			model->PlayAnimation(animationIndexes[AttackCount % 4], false);
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
					p->FireAt(Player::Instance().GetPosition(), info.speed, false);

				hasShot = true;
			}

			if (frame >= config->advanceInputEndFrame)
			{
				AttackCount++;
				this->step = (AttackCount >= 4) ? 2 : 0;
			}
			break;

		case 2:
			if (!model->IsPlayAnimation()) return ResetState(ActionBase<ActorType>::State::Complete);
			break;
		}

		if (this->owner->IsAnyDamage() || this->owner->IsDeathFlag()) return ResetState(ActionBase<ActorType>::State::Failed);
		return ActionBase<ActorType>::State::Run;
	}

private:
	typename ActionBase<ActorType>::State ResetState(typename ActionBase<ActorType>::State state)
	{
		this->step = 0;
		AttackCount = 0;
		hasShot = false;
		return state;
	}
};

// 突進斬り
template <typename ActorType>
class DashSlashAction : public ActionBase<ActorType>
{
public:
	DashSlashAction(ActorType* actor) : ActionBase<ActorType>(actor) {}

	int animationIndex = this->owner->GetModel()->GetAnimationIndex("Run_Attack_01_Seq_0");
	bool init = false;
	float timer = 0.0f;
	float duration = 7.0f;
	Vector3 epsilon{ 3.5f, 1.0f, 3.5f };
	Vector3 teleportPosition;

	typename ActionBase<ActorType>::State Run(float elapsedTime) override
	{
		this->owner->SetTargetPosition(Player::Instance().GetPosition());
		if (!init) { init = true; timer = 0.0f; }

		auto* model = this->owner->GetModel();
		float frame = model->GetCurrentAnimationSeconds();
		AnimationConfig* config = model->GetAnimationConfig("EnemyBoss", animationIndex);

		if (!this->owner->isPlayerInvincible)
			this->owner->GetSword()->AttackAnimationCollision(model, config, this->owner->GetCharacter());

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
			model->PlayRootMotion(animationIndex, false, true, this->owner->GetBlendSeconds(), "root");
			this->owner->SetGravity(-0.001f);
			this->step++;
			break;

		case 1:
			if (frame >= config->advanceInputStartFrame) model->PauseAnimation(true);
			this->owner->TurnToTarget(elapsedTime, 1000);

			if (!this->owner->IsTeleporting())
			{
				auto playerPos = Player::Instance().GetPosition();
				this->owner->SetPosition({
					Mathf::Lerp(this->owner->GetPosition().x, playerPos.x, t),
					Mathf::Lerp(this->owner->GetPosition().y, playerPos.y, t),
					Mathf::Lerp(this->owner->GetPosition().z, playerPos.z, t)
				});
			}

			if (XMVector3NearEqual(XMLoadFloat3(&this->owner->GetPosition()), XMLoadFloat3(&Player::Instance().GetPosition()), epsilon))
			{
				if (this->behaviorData->IsInSequenceAndNotLast() && !this->owner->IsTeleporting())
				{
					this->owner->StartTeleport(teleportPosition, 0.1f);
					return ResetState(ActionBase<ActorType>::State::Complete);
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
				return ResetState(ActionBase<ActorType>::State::Complete);
			}
			break;
		}

		if (this->owner->IsAnyDamage() || this->owner->IsDeathFlag())
		{
			model->PauseAnimation(false);
			return ResetState(ActionBase<ActorType>::State::Failed);
		}
		return ActionBase<ActorType>::State::Run;
	}

private:
	typename ActionBase<ActorType>::State ResetState(typename ActionBase<ActorType>::State state)
	{
		this->owner->SetGravity(-0.3f);
		init = false;
		this->owner->SetPlayedEffect(false);
		this->step = 0;
		timer = 0;
		return state;
	}
};

// 光柱円形収束
template <typename ActorType>
class PillarSpiralConv : public ActionBase<ActorType>
{
public:
	PillarSpiralConv(ActorType* actor) : ActionBase<ActorType>(actor) {}

	int animationIndexes[3] = { 
		this->owner->GetModel()->GetAnimationIndex("Power_Attack_Start_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Power_Attack_Loop_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_02_Seq_0") 
	};
	std::vector<Projectile*> spawnedProjectiles;
	float waitTimer = 0.0f;
	const float waitDuration = 1.5f;
	float recoveryTimer = 0.0f;
	const float recoveryDuration = 0.75f;
	const float circleWaitTime = 2.5f;
	float lockOnTime = 0.3f;
	float currentRotationAngle = 0.0f;

	typename ActionBase<ActorType>::State Run(float elapsedTime) override
	{
		auto* model = this->owner->GetModel();

		switch (this->step)
		{
		case 0:
			model->PlayRootMotion(animationIndexes[0], false, true, 0.2f, "root");
			this->step++;
			break;

		case 1:
		{
			model->PlayRootMotion(animationIndexes[1], true, true, 0.2f, "root");
			spawnedProjectiles.clear();
			
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

				if (Projectile* p = ProjectileManager::Instance().Launch(info))
					spawnedProjectiles.push_back(p);
			}
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
						p->SetPosition({ playerPos.x + cosf(angle) * 3.0f, this->owner->GetPosition().y, playerPos.z + sinf(angle) * 3.0f });
					}
				}
			}
			if (waitTimer >= circleWaitTime) this->step++;
			break;
		}

		case 3:
			model->PlayRootMotion(animationIndexes[2], false, true, 0.2f, "root");
			for (auto* p : spawnedProjectiles)
			{
				if (p && p->IsActive()) p->FireAt(Player::Instance().GetPosition(), 20.0f, true);
			}
			spawnedProjectiles.clear();
			
			waitTimer += elapsedTime;
			if (waitTimer >= waitDuration) this->step++;
			break;

		case 4:
			if (!model->IsPlayAnimation()) this->step++;
			break;

		case 5:
			model->PlayAnimation(model->GetAnimationIndex("Idle_Combat_Seq_0"), true, 0.2f);
			recoveryTimer += elapsedTime;
			if (recoveryTimer >= recoveryDuration) return ResetState(ActionBase<ActorType>::State::Complete);
			break;
		}

		if (this->owner->IsAnyDamage() || this->owner->IsDeathFlag()) return ResetState(ActionBase<ActorType>::State::Failed);
		return ActionBase<ActorType>::State::Run;
	}

private:
	typename ActionBase<ActorType>::State ResetState(typename ActionBase<ActorType>::State state)
	{
		this->step = 0;
		waitTimer = 0.0f;
		recoveryTimer = 0.0f;
		spawnedProjectiles.clear();
		return state;
	}
};

// 光柱円形拡散
template <typename ActorType>
class PillarSpiralDiff : public ActionBase<ActorType>
{
public:
	PillarSpiralDiff(ActorType* actor) : ActionBase<ActorType>(actor) {}

	int animationIndexes[3] = { 
		this->owner->GetModel()->GetAnimationIndex("Power_Attack_Start_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Power_Attack_Loop_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_Wave_05_02_Seq_0") 
	};
	std::vector<Projectile*> spawnedProjectiles;
	float waitTimer = 0.0f;
	const float waitDuration = 1.5f;
	float recoveryTimer = 0.0f;
	const float recoveryDuration = 2.0f;

	typename ActionBase<ActorType>::State Run(float elapsedTime) override
	{
		auto* model = this->owner->GetModel();

		switch (this->step)
		{
		case 0:
			model->PlayRootMotion(animationIndexes[0], false, true, 0.2f, "root");
			this->step++;
			break;

		case 1:
			model->PlayRootMotion(animationIndexes[1], true, true, 0.2f, "root");
			if (spawnedProjectiles.empty())
			{
				int count = 8;
				float initialRadius = 2.0f;
				XMFLOAT3 center = this->owner->GetPosition();

				for (int i = 0; i < count; ++i)
				{
					ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
					info.owner = this->owner;
					info.moveType = MovementType::Stationary;
					info.scale = 0.5f;
					info.radius = 0.7f;
					info.lifeTime = 5.0f;
					info.centerPosition = center;
					info.currentRadius = initialRadius;
					info.invincibleTime = 1.0f;

					float angle = XM_2PI * (float)i / count;
					info.currentAngle = angle;
					info.spawnPosition = { center.x + cosf(angle) * initialRadius, center.y, center.z + sinf(angle) * initialRadius };

					if (Projectile* p = ProjectileManager::Instance().Launch(info)) spawnedProjectiles.push_back(p);
				}
			}

			waitTimer += elapsedTime;
			if (waitTimer >= waitDuration) this->step++;
			break;

		case 2:
			for (auto* p : spawnedProjectiles)
			{
				if (p && p->IsActive())
				{
					model->PlayRootMotion(animationIndexes[3], false, true, 0.2f, "root"); // NOTE: index bounds check needed if out of range, original code used 3.
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
			if (recoveryTimer >= recoveryDuration) return ResetState(ActionBase<ActorType>::State::Complete);
			break;
		}

		if (this->owner->IsAnyDamage() || this->owner->IsDeathFlag()) return ResetState(ActionBase<ActorType>::State::Failed);
		return ActionBase<ActorType>::State::Run;
	}

private:
	typename ActionBase<ActorType>::State ResetState(typename ActionBase<ActorType>::State state)
	{
		this->step = 0;
		waitTimer = 0.0f;
		recoveryTimer = 0.0f;
		spawnedProjectiles.clear();
		return state;
	}
};

// テレポート強襲
template <typename ActorType>
class TelePortAssault : public ActionBase<ActorType>
{
public:
	TelePortAssault(ActorType* actor) : ActionBase<ActorType>(actor) {}

	int animationIndexes[3] = { 
		this->owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_03_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0") 
	};
	Vector3 teleportPosition;
	std::vector<Projectile*> spawnedProjectiles;

	typename ActionBase<ActorType>::State Run(float elapsedTime) override
	{
		auto* model = this->owner->GetModel();
		float frame = model->GetCurrentAnimationSeconds();
		int index = model->GetCurrentAnimationIndex();
		AnimationConfig* config = model->GetAnimationConfig("EnemyBoss", index);

		if (!this->owner->isPlayerInvincible)
			this->owner->GetSword()->AttackAnimationCollision(model, config, this->owner->GetCharacter());

		this->owner->SetTargetPosition(Player::Instance().GetPosition());

		switch (this->step)
		{
		case 0:
			teleportPosition = this->owner->CalculateVisibleTeleportPos(3.0f);
			this->owner->SetGravity(-0.001f);
			model->PlayRootMotion(animationIndexes[0], false, true, this->owner->GetBlendSeconds(), "root");
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
			this->owner->TurnToTarget(elapsedTime, 10000);
			model->PlayRootMotion(animationIndexes[1], false, true, this->owner->GetBlendSeconds(), "root");
			
			if (this->owner->GetHealth() <= (this->owner->GetMaxHealth() * 0.5) && this->behaviorData->IsLastNodeInSequence() && spawnedProjectiles.empty())
			{
				for (int i = 0; i < 2; i++)
				{
					ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
					info.owner = this->owner;
					info.moveType = MovementType::Stationary;
					info.scale = 0.5f;
					info.radius = 0.7f;
					info.lifeTime = 5.0f;
					info.speed = 40.0f;

					auto posx = (i == 0) ? this->owner->CharacterLeft(this->owner->GetAngle()).x : this->owner->CharacterRight(this->owner->GetAngle()).x;
					auto posz = (i == 0) ? this->owner->CharacterLeft(this->owner->GetAngle()).z : this->owner->CharacterRight(this->owner->GetAngle()).z;

					info.spawnPosition = {
						this->owner->GetPosition().x + (this->owner->CharacterBack(this->owner->GetAngle()).x - posx) * 3.0f,
						-1,
						this->owner->GetPosition().z + (this->owner->CharacterBack(this->owner->GetAngle()).z - posz) * 2.0f
					};

					if (Projectile* p = ProjectileManager::Instance().Launch(info)) spawnedProjectiles.push_back(p);
				}
			}
			this->step++;
			break;

		case 4:
			this->owner->TurnToTarget(elapsedTime, 10000);
			if (frame >= config->advanceInputEndFrame)
			{
				this->owner->SetGravity(-0.3f);
				model->PlayRootMotion(animationIndexes[2], false, true, this->owner->GetBlendSeconds(), "root");
				this->step++;
			}
			break;

		case 5:
			if (frame >= config->advanceInputStartFrame)
			{
				if (this->behaviorData->IsLastNodeInSequence())
				{
					for (auto* p : spawnedProjectiles)
					{
						if (p && p->IsActive()) p->FireAt(Player::Instance().GetPosition(), 20.0f, true);
					}
					spawnedProjectiles.clear();
				}
				this->step++;
			}
			else this->owner->MoveToTarget(elapsedTime, 2);
			break;

		case 6:
			if (this->behaviorData->IsInSequenceAndNotLast() && frame >= config->advanceInputStartFrame) return ResetState(ActionBase<ActorType>::State::Complete);

			if (!model->IsPlayAnimation() || (frame >= config->advanceInputEndFrame && !this->owner->IsGround()))
			{
				return ResetState(ActionBase<ActorType>::State::Complete);
			}
			break;
		}

		if (this->owner->IsAnyDamage() || this->owner->IsDeathFlag()) return ResetState(ActionBase<ActorType>::State::Failed);
		return ActionBase<ActorType>::State::Run;
	}

private:
	typename ActionBase<ActorType>::State ResetState(typename ActionBase<ActorType>::State state)
	{
		this->owner->SetGravity(-0.3f);
		this->step = 0;
		return state;
	}
};

// テレポートコンボ
template <typename ActorType>
class TeleportCombo : public ActionBase<ActorType>
{
public:
	TeleportCombo(ActorType* actor) : ActionBase<ActorType>(actor) {}

	int AttackCount = 0;
	int teleportAnimationIndex = this->owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0");
	int comboAnimationIndexes[5] = { 
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_03_01_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_03_02_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Attack_Up_01_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_Loop_02_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_End_02_Seq_0") 
	};
	Vector3 teleportPosition;

	typename ActionBase<ActorType>::State Run(float elapsedTime) override
	{
		auto* model = this->owner->GetModel();
		float frame = model->GetCurrentAnimationSeconds();
		int index = model->GetCurrentAnimationIndex();
		AnimationConfig* config = model->GetAnimationConfig("EnemyBoss", index);
		
		this->owner->SetTargetPosition(Player::Instance().GetPosition());
		if (!this->owner->isPlayerInvincible)
			this->owner->GetSword()->AttackAnimationCollision(model, config, this->owner->GetCharacter());

		switch (this->step)
		{
		case 0:
			model->PlayRootMotion(teleportAnimationIndex, false, true, this->owner->GetBlendSeconds(), "root");
			this->step++;
			break;

		case 1:
			this->owner->TurnToTarget(elapsedTime, 1000);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				teleportPosition = this->owner->CalculateVisibleTeleportPos((AttackCount < 3) ? 2.0f : 1.5f);
				if (AttackCount >= 3) teleportPosition.y += 3.0f;

				this->owner->SetGravity(0.0f);
				this->owner->StartTeleport(teleportPosition, 0.1f);
				this->step++;
			}
			break;

		case 2:
			this->owner->TurnToTarget(elapsedTime, 1000);
			if (!this->owner->IsTeleporting()) this->step++;
			break;

		case 3:
			this->owner->TurnToTarget(elapsedTime, 1000);
			if (!this->owner->IsTeleporting())
			{
				model->PlayRootMotion(comboAnimationIndexes[AttackCount++], false, true, this->owner->GetBlendSeconds(), "root");
				if (frame >= config->advanceInputEndFrame) this->step = (AttackCount < 4) ? 1 : this->step + 1;
			}
			break;

		case 4:
			this->owner->TurnToTarget(elapsedTime, 1000);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->SetGravity(-1.5f);
				model->PlayRootMotion(comboAnimationIndexes[AttackCount], false, true, this->owner->GetBlendSeconds(), "root");
				this->step++;
			}
			break;

		case 5:
			if (frame >= config->advanceInputEndFrame) return ResetState(ActionBase<ActorType>::State::Complete);
			break;
		}

		if (this->owner->IsAnyDamage() || this->owner->IsDeathFlag()) return ResetState(ActionBase<ActorType>::State::Failed);
		return ActionBase<ActorType>::State::Run;
	}

private:
	typename ActionBase<ActorType>::State ResetState(typename ActionBase<ActorType>::State state)
	{
		this->step = 0;
		AttackCount = 0;
		this->owner->SetGravity(-0.3f);
		return state;
	}
};

// 必殺技
template <typename ActorType>
class SpecialAttack : public ActionBase<ActorType>
{
public:
	SpecialAttack(ActorType* actor) : ActionBase<ActorType>(actor) {}

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

	typename ActionBase<ActorType>::State Run(float elapsedTime) override
	{
		auto* model = this->owner->GetModel();
		float frame = model->GetCurrentAnimationSeconds();
		int index = model->GetCurrentAnimationIndex();
		AnimationConfig* config = model->GetAnimationConfig("EnemyBoss", index);

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
			model->PlayRootMotion(animationIndexes[0], false, true, this->owner->GetBlendSeconds(), "root");
			this->step++;
			break;

		case 1:
			this->owner->TurnToTarget(elapsedTime, 1000);
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
				model->PlayRootMotion(animationIndexes[2], true, true, 0.2f, "root");
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
					p->FireAt(Player::Instance().GetPosition(), 25.0f, true);

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
			for (auto* p : groupHoming) if (p && p->IsActive()) p->FireAt(Player::Instance().GetPosition(), 20.0f, true);
			groupHoming.clear();

			if (++waveCount < 2) { waitTimer = 0.0f; this->step = 11; }
			else this->step = 12;
			break;

		case 11:
			waitTimer += elapsedTime;
			if (waitTimer >= 1.0f) this->step = 5;
			break;

		case 12:
			model->PlayRootMotion(animationIndexes[3], true, true, 0.2f, "root");
			this->owner->magicCircle->Stop(handle);
			handle = -1;
			recoveryTimer += elapsedTime;
			if (recoveryTimer >= recoveryDuration)
			{
				model->PlayRootMotion(animationIndexes[4], true, true, 0.2f, "root");
				return ResetState(ActionBase<ActorType>::State::Complete);
			}
			break;
		}

		if (this->owner->IsAnyDamage() || this->owner->IsDeathFlag()) return ResetState(ActionBase<ActorType>::State::Failed);
		return ActionBase<ActorType>::State::Run;
	}

private:
	typename ActionBase<ActorType>::State ResetState(typename ActionBase<ActorType>::State state)
	{
		this->step = 0;
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
		return state;
	}
};

// 強制反撃1
template <typename ActorType>
class RevengeDive : public ActionBase<ActorType>
{
public:
	RevengeDive(ActorType* actor) : ActionBase<ActorType>(actor) {}

	float intervalTimer = 0.0f;
	const float intervalDuration = 0.7f;
	int animationIndexes[2] = { 
		this->owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_Loop_02_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Attack_Air_To_Floor_End_02_Seq_0") 
	};

	typename ActionBase<ActorType>::State Run(float elapsedTime) override
	{
		auto* model = this->owner->GetModel();
		float frame = model->GetCurrentAnimationSeconds();
		int index = model->GetCurrentAnimationIndex();
		AnimationConfig* config = model->GetAnimationConfig("EnemyBoss", index);
		
		this->owner->SetTargetPosition(Player::Instance().GetPosition());
		this->owner->SetSuperArmor(true);
		
		if (!this->owner->isPlayerInvincible)
			this->owner->GetSword()->AttackAnimationCollision(model, config, this->owner->GetCharacter());

		switch (this->step)
		{
		case 0:
			this->owner->SetGravity(0.0f);
			this->owner->StartTeleport(Player::Instance().launchKnockbackPosition, 0.1f);
			this->step++;
			break;

		case 1:
			intervalTimer += elapsedTime;
			this->owner->TurnToTarget(elapsedTime, 1000);
			if (!this->owner->IsTeleporting())
			{
				this->owner->SetPosition(Player::Instance().launchKnockbackPosition);
				model->PlayRootMotion(animationIndexes[0], true, true, this->owner->GetBlendSeconds(), "root");
			}
			if (intervalTimer >= intervalDuration)
			{
				intervalTimer = 0.0f;
				this->step++;
			}
			break;

		case 2:
			this->owner->TurnToTarget(elapsedTime, 1000);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->SetGravity(-1.5f);
				model->PlayRootMotion(animationIndexes[1], false, true, this->owner->GetBlendSeconds(), "root");
				this->step++;
			}
			break;

		case 3:
			if (frame >= config->advanceInputEndFrame)
			{
				this->step = 0;
				this->owner->SetGravity(-0.3f);
				this->owner->SetRevengeState(false);
				this->owner->SetSuperArmor(false);
				return ActionBase<ActorType>::State::Complete;
			}
			break;
		}
		return ActionBase<ActorType>::State::Run;
	}
};

// 強制反撃2
template <typename ActorType>
class RevengeAssault : public ActionBase<ActorType>
{
public:
	RevengeAssault(ActorType* actor) : ActionBase<ActorType>(actor) {}

	float intervalTimer = 0.0f;
	const float intervalDuration = 0.7f;
	int animationIndexes[3] = { 
		this->owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_03_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0") 
	};
	Vector3 teleportPosition;
	std::vector<Projectile*> spawnedProjectiles;

	typename ActionBase<ActorType>::State Run(float elapsedTime) override
	{
		auto* model = this->owner->GetModel();
		float frame = model->GetCurrentAnimationSeconds();
		int index = model->GetCurrentAnimationIndex();
		AnimationConfig* config = model->GetAnimationConfig("EnemyBoss", index);
		
		this->owner->SetSuperArmor(true);
		this->owner->SetTargetPosition(Player::Instance().GetPosition());
		
		if (!this->owner->isPlayerInvincible)
			this->owner->GetSword()->AttackAnimationCollision(model, config, this->owner->GetCharacter());

		switch (this->step)
		{
		case 0:
			teleportPosition = this->owner->CalculateVisibleTeleportPos(3.0f);
			this->owner->SetGravity(0.0f);
			model->PlayRootMotion(animationIndexes[0], false, true, this->owner->GetBlendSeconds(), "root");
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
				model->PlayRootMotion(animationIndexes[1], false, true, this->owner->GetBlendSeconds(), "root");
				
				if (this->owner->GetHealth() <= (this->owner->GetMaxHealth() * 0.5) && spawnedProjectiles.empty())
				{
					for (int i = 0; i < 2; i++)
					{
						ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
						info.owner = this->owner;
						info.moveType = MovementType::Stationary;
						info.scale = 0.5f;
						info.radius = 0.7f;
						info.lifeTime = 5.0f;
						info.speed = 40.0f;

						auto posx = (i == 0) ? this->owner->CharacterLeft(this->owner->GetAngle()).x : this->owner->CharacterRight(this->owner->GetAngle()).x;
						auto posz = (i == 0) ? this->owner->CharacterLeft(this->owner->GetAngle()).z : this->owner->CharacterRight(this->owner->GetAngle()).z;

						info.spawnPosition = {
							this->owner->GetPosition().x + (this->owner->CharacterBack(this->owner->GetAngle()).x - posx) * 3.0f,
							-1,
							this->owner->GetPosition().z + (this->owner->CharacterBack(this->owner->GetAngle()).z - posz) * 2.0f
						};
						
						if (Projectile* p = ProjectileManager::Instance().Launch(info)) spawnedProjectiles.push_back(p);
					}
				}
				this->step++;
			}
			break;

		case 3:
			this->owner->TurnToTarget(elapsedTime, 1000);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->SetGravity(-1.5f);
				model->PlayRootMotion(animationIndexes[2], false, true, this->owner->GetBlendSeconds(), "root");
				this->step++;
			}
			break;

		case 4:
			if (frame >= config->advanceInputStartFrame)
			{
				for (auto* p : spawnedProjectiles)
				{
					if (p && p->IsActive()) p->FireAt(Player::Instance().GetPosition(), 20.0f, true);
				}
				this->step++;
				spawnedProjectiles.clear();
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
				return ActionBase<ActorType>::State::Complete;
			}
			break;
		}
		return ActionBase<ActorType>::State::Run;
	}
};