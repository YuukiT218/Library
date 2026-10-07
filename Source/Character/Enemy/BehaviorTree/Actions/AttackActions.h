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

	// コンボの段数
	static constexpr int COMBO_COUNT = 4;

	int animationIndexes[COMBO_COUNT] = {
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
			if (attackCount >= COMBO_COUNT)
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

	// 斬撃波を撃つ回数
	static constexpr int WAVE_COUNT = 4;

	// 斬撃波を出す位置（自分の前方への距離）
	static constexpr float WAVE_SPAWN_FORWARD_DISTANCE = 1.5f;

	int animationIndexes[WAVE_COUNT] = {
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
			model->PlayAnimation(animationIndexes[attackCount % WAVE_COUNT], false);
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

				info.spawnPosition = { bossPos.x + bossForward.x * WAVE_SPAWN_FORWARD_DISTANCE, bossPos.y, bossPos.z + bossForward.z * WAVE_SPAWN_FORWARD_DISTANCE };
				info.direction = bossForward;

				if (Projectile* p = ProjectileManager::Instance().Launch(info))
					p->FireAt(this->PlayerPosition(), info.speed, false);

				hasShot = true;
			}

			if (frame >= config->advanceInputEndFrame)
			{
				attackCount++;
				this->step = (attackCount >= WAVE_COUNT) ? 2 : 0;
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

	// プレイヤーへ寄っていくのにかける時間（単発・シーケンス中・終了後）
	static constexpr float APPROACH_DURATION = 7.0f;
	static constexpr float SEQUENCE_APPROACH_DURATION = 15.0f;
	static constexpr float RESET_APPROACH_DURATION = 3.0f;

	// シーケンス中、次の行動へ繋ぐテレポート先までの距離
	static constexpr float SEQUENCE_TELEPORT_DISTANCE = 15.0f;

	int animationIndex = this->owner->GetModel()->GetAnimationIndex("Run_Attack_01_Seq_0");
	bool init = false;
	float timer = 0.0f;
	float duration = APPROACH_DURATION;

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

		if (!this->owner->HasPlayedEffect())
		{
			this->PlayAttackSign();
			this->owner->SetPlayedEffect(true);
		}

		switch (this->step)
		{
		case 0:
			if (this->behaviorData->IsInSequence())
			{
				teleportPosition = this->owner->CalculateVisibleTeleportPos(SEQUENCE_TELEPORT_DISTANCE);
				duration = SEQUENCE_APPROACH_DURATION;
			}
			this->PlayRootMotion(animationIndex, false);
			this->owner->SetGravity(this->HOVER_GRAVITY);
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
				// シーケンス途中なら踏み込んだところで打ち切り、テレポートで次の行動へ繋ぐ
				if (this->behaviorData->IsInSequenceAndNotLast() && !this->owner->IsTeleporting())
				{
					this->owner->StartTeleport(teleportPosition, this->QUICK_TELEPORT_FADE_OUT_SECONDS);
					return this->ResetState(State::Complete);
				}
				// 打ち切らないときはそのまま斬りまで通す
				// （上の条件の裏返しなので、キャンセルを切ったときも必ずこちらへ入る）
				if (!this->behaviorData->IsInSequenceAndNotLast() && !this->owner->IsTeleporting())
				{
					this->PlayAttackSign();
					model->PauseAnimation(false);
					this->step++;
				}
			}
			break;

		case 2:
			if (!model->IsPlayAnimation() || (!this->owner->IsGround() && frame >= config->advanceInputEndFrame))
			{
				duration = RESET_APPROACH_DURATION;
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
		this->owner->SetGravity(Character::DEFAULT_GRAVITY);
		init = false;
		this->owner->SetPlayedEffect(false);
		timer = 0.0f;
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

	// 光柱の見た目の大きさ・当たり判定の半径・寿命・ヒット後の無敵時間
	static constexpr float PILLAR_SCALE = 0.4f;
	static constexpr float PILLAR_HIT_RADIUS = 0.5f;
	static constexpr float PILLAR_LIFE_TIME = 8.0f;
	static constexpr float PILLAR_INVINCIBLE_TIME = 0.5f;

	// 光柱がプレイヤーの周りを回る速さ（ラジアン/秒）
	static constexpr float ORBIT_ANGULAR_SPEED = 1.0f;

	std::vector<Projectile*> spawnedProjectiles;
	float waitTimer = 0.0f;
	static constexpr float WAIT_DURATION = 1.5f;
	float recoveryTimer = 0.0f;
	static constexpr float RECOVERY_DURATION = 0.75f;
	static constexpr float CIRCLE_WAIT_TIME = 2.5f;
	// 発射直前、光柱の回転を止めて狙いを定める時間
	static constexpr float LOCK_ON_TIME = 0.3f;
	float currentRotationAngle = 0.0f;

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();

		switch (this->step)
		{
		case 0:
			this->PlayRootMotion(animationIndexes[0], false, this->ACTION_BLEND_SECONDS);
			this->step++;
			break;

		case 1:
		{
			this->PlayRootMotion(animationIndexes[1], true, this->ACTION_BLEND_SECONDS);
			spawnedProjectiles.clear();

			// プレイヤーを囲むように光柱を円状に配置する
			XMFLOAT3 playerPos = this->PlayerPosition();
			XMFLOAT3 center = { playerPos.x, this->owner->GetPosition().y, playerPos.z };
			this->SpawnLightPillarCircle(spawnedProjectiles, center,
				PILLAR_COUNT, CIRCLE_RADIUS, PILLAR_SCALE, PILLAR_HIT_RADIUS, PILLAR_LIFE_TIME, PILLAR_INVINCIBLE_TIME, false);
			waitTimer = 0.0f;
			this->step++;
			break;
		}

		case 2:
		{
			waitTimer += elapsedTime;
			if (waitTimer < CIRCLE_WAIT_TIME - LOCK_ON_TIME)
			{
				currentRotationAngle += ORBIT_ANGULAR_SPEED * elapsedTime;
				XMFLOAT3 playerPos = Player::Instance().GetPosition();
				int count = static_cast<int>(spawnedProjectiles.size());

				for (int i = 0; i < count; ++i)
				{
					if (Projectile* p = spawnedProjectiles[i]; p && p->IsActive())
					{
						float angle = (XM_2PI * static_cast<float>(i) / count) + currentRotationAngle;
						p->SetPosition({ playerPos.x + cosf(angle) * CIRCLE_RADIUS, this->owner->GetPosition().y, playerPos.z + sinf(angle) * CIRCLE_RADIUS });
					}
				}
			}
			if (waitTimer >= CIRCLE_WAIT_TIME) this->step++;
			break;
		}

		case 3:
			this->PlayRootMotion(animationIndexes[2], false, this->ACTION_BLEND_SECONDS);
			this->FireAndClear(spawnedProjectiles, this->PILLAR_FIRE_SPEED, true);

			waitTimer += elapsedTime;
			if (waitTimer >= WAIT_DURATION) this->step++;
			break;

		case 4:
			if (!model->IsPlayAnimation()) this->step++;
			break;

		case 5:
			model->PlayAnimation(model->GetAnimationIndex(this->IDLE_ANIMATION_NAME), true, this->ACTION_BLEND_SECONDS);
			recoveryTimer += elapsedTime;
			if (recoveryTimer >= RECOVERY_DURATION) return this->ResetState(State::Complete);
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

	// 光柱の見た目の大きさ・当たり判定の半径・寿命・ヒット後の無敵時間
	static constexpr float PILLAR_SCALE = 0.5f;
	static constexpr float PILLAR_HIT_RADIUS = 0.7f;
	static constexpr float PILLAR_LIFE_TIME = 5.0f;
	static constexpr float PILLAR_INVINCIBLE_TIME = 1.0f;

	// 外へ広がる螺旋の回転速度と広がる速さ
	static constexpr float SPIRAL_ANGULAR_SPEED = 3.0f;
	static constexpr float SPIRAL_EXPAND_SPEED = 5.0f;

	std::vector<Projectile*> spawnedProjectiles;
	float waitTimer = 0.0f;
	static constexpr float WAIT_DURATION = 1.5f;
	float recoveryTimer = 0.0f;
	static constexpr float RECOVERY_DURATION = 2.0f;

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();

		switch (this->step)
		{
		case 0:
			this->PlayRootMotion(animationIndexes[0], false, this->ACTION_BLEND_SECONDS);
			this->step++;
			break;

		case 1:
			this->PlayRootMotion(animationIndexes[1], true, this->ACTION_BLEND_SECONDS);
			if (spawnedProjectiles.empty())
			{
				// 自分を中心に光柱を円状に配置し、外へ広がる軌道情報を持たせる
				this->SpawnLightPillarCircle(spawnedProjectiles, this->owner->GetPosition(),
					PILLAR_COUNT, INITIAL_RADIUS, PILLAR_SCALE, PILLAR_HIT_RADIUS, PILLAR_LIFE_TIME, PILLAR_INVINCIBLE_TIME, true);
			}

			waitTimer += elapsedTime;
			if (waitTimer >= WAIT_DURATION) this->step++;
			break;

		case 2:
			for (auto* p : spawnedProjectiles)
			{
				if (p && p->IsActive())
				{
					this->PlayRootMotion(animationIndexes[2], false, this->ACTION_BLEND_SECONDS);
					p->StartSpiral(SPIRAL_ANGULAR_SPEED, SPIRAL_EXPAND_SPEED);
				}
			}
			spawnedProjectiles.clear();
			this->step++;
			break;

		case 3:
			if (!model->IsPlayAnimation()) this->step++;
			break;

		case 4:
			model->PlayAnimation(model->GetAnimationIndex(this->IDLE_ANIMATION_NAME), true, this->ACTION_BLEND_SECONDS);
			recoveryTimer += elapsedTime;
			if (recoveryTimer >= RECOVERY_DURATION) return this->ResetState(State::Complete);
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
	// 斬りかかる間にプレイヤーへ詰める速さの倍率
	static constexpr float APPROACH_SPEED_RATE = 2.0f;

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
			this->owner->SetGravity(this->HOVER_GRAVITY);
			this->PlayRootMotion(animationIndexes[0], false);
			this->step++;
			break;

		case 1:
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->StartTeleport(teleportPosition, this->QUICK_TELEPORT_FADE_OUT_SECONDS);
				this->step++;
			}
			break;

		case 2:
			if (!this->owner->IsTeleporting()) this->step++;
			break;

		case 3:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::INSTANT);
			this->PlayRootMotion(animationIndexes[1], false);

			if (this->IsHealthBelowRate(this->PILLAR_SUPPORT_HEALTH_RATE) && this->behaviorData->IsLastNodeInSequence() && spawnedProjectiles.empty())
			{
				this->SpawnSideLightPillars(spawnedProjectiles);
			}
			this->step++;
			break;

		case 4:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::INSTANT);
			if (frame >= config->advanceInputEndFrame)
			{
				this->owner->SetGravity(Character::DEFAULT_GRAVITY);
				this->PlayRootMotion(animationIndexes[2], false);
				this->step++;
			}
			break;

		case 5:
			if (frame >= config->advanceInputStartFrame)
			{
				if (this->behaviorData->IsLastNodeInSequence())
				{
					this->FireAndClear(spawnedProjectiles, this->PILLAR_FIRE_SPEED, true);
				}
				this->step++;
			}
			else this->owner->MoveToTarget(elapsedTime, APPROACH_SPEED_RATE);
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
		this->owner->SetGravity(Character::DEFAULT_GRAVITY);
	}
};

// テレポートコンボ
template <typename ActorType>
class TeleportCombo : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	TeleportCombo(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}

	// 地上で斬る回数（以降は空中からの叩きつけ）
	static constexpr int GROUND_ATTACK_COUNT = 3;

	// 地上で斬るときに最後のテレポートの前までに出す段数
	static constexpr int TELEPORT_ATTACK_COUNT = 4;

	// テレポート先のプレイヤーからの距離（地上・空中）
	static constexpr float GROUND_TELEPORT_DISTANCE = 2.0f;
	static constexpr float AIR_TELEPORT_DISTANCE = 1.5f;

	// 空中からの攻撃でテレポートする高さ
	static constexpr float AIR_TELEPORT_HEIGHT = 3.0f;

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
				const bool isGroundAttack = attackCount < GROUND_ATTACK_COUNT;
				teleportPosition = this->owner->CalculateVisibleTeleportPos(isGroundAttack ? GROUND_TELEPORT_DISTANCE : AIR_TELEPORT_DISTANCE);
				if (!isGroundAttack) teleportPosition.y += AIR_TELEPORT_HEIGHT;

				this->owner->SetGravity(0.0f);
				this->owner->StartTeleport(teleportPosition, this->QUICK_TELEPORT_FADE_OUT_SECONDS);
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
				if (frame >= config->advanceInputEndFrame) this->step = (attackCount < TELEPORT_ATTACK_COUNT) ? 1 : this->step + 1;
			}
			break;

		case 4:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->SetGravity(this->DIVE_GRAVITY);
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
		this->owner->SetGravity(Character::DEFAULT_GRAVITY);
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
	// 必殺技を撃つ位置（ステージ中央）
	static constexpr XMFLOAT3 CAST_POSITION = { -0.37f, -1.460f, 0.35f };

	// 魔法陣エフェクトの大きさ
	static constexpr float MAGIC_CIRCLE_SCALE = 5.0f;

	// 溜め中に毎フレーム出す光の粒の数と出現範囲
	static constexpr int CHARGE_PARTICLE_COUNT = 5;
	static constexpr float CHARGE_PARTICLE_MIN_DISTANCE = 1.5f;
	static constexpr float CHARGE_PARTICLE_DISTANCE_RANGE = 2.5f;
	static constexpr float CHARGE_PARTICLE_HEIGHT_RANGE = 4.0f;
	static constexpr float CHARGE_PARTICLE_DRIFT_SPEED = 0.5f;
	static constexpr float CHARGE_PARTICLE_MIN_RISE_SPEED = 1.0f;
	static constexpr float CHARGE_PARTICLE_RISE_SPEED_RANGE = 2.0f;
	static constexpr XMFLOAT4 CHARGE_PARTICLE_COLOR = { 1.0f, 0.9f, 0.2f, 1.0f };
	static constexpr float CHARGE_PARTICLE_MIN_SIZE = 0.5f;
	static constexpr float CHARGE_PARTICLE_SIZE_RANGE = 0.2f;
	static constexpr float CHARGE_PARTICLE_MIN_LIFE = 0.4f;
	static constexpr float CHARGE_PARTICLE_LIFE_RANGE = 0.4f;

	// 溜めの粒子を出し続ける step の範囲
	static constexpr int CHARGE_PARTICLE_FIRST_STEP = 2;
	static constexpr int CHARGE_PARTICLE_LAST_STEP = 11;

	// 光柱の見た目の大きさと当たり判定の半径
	static constexpr float PILLAR_SCALE = 0.4f;
	static constexpr float PILLAR_HIT_RADIUS = 0.5f;

	// 螺旋状に回る光柱の本数・寿命・ヒット後の無敵時間・内側と外側の半径
	static constexpr int SPIRAL_PILLAR_COUNT = 6;
	static constexpr float SPIRAL_PILLAR_LIFE_TIME = 10.0f;
	static constexpr float SPIRAL_PILLAR_INVINCIBLE_TIME = 1.0f;
	static constexpr float SPIRAL_INNER_RADIUS = 2.0f;
	static constexpr float SPIRAL_OUTER_RADIUS = 6.0f;

	// 螺旋の回転速度（内側と外側で逆回転）と広がる速さ
	static constexpr float SPIRAL_ANGULAR_SPEED = 1.0f;
	static constexpr float SPIRAL_EXPAND_SPEED = 2.0f;

	// 画面内から撃ってくる追尾弾の本数・寿命・出現距離・速さ
	static constexpr int HOMING_SHOT_COUNT = 6;
	static constexpr float HOMING_SHOT_LIFE_TIME = 5.0f;
	static constexpr float HOMING_SHOT_SPAWN_DISTANCE = 20.0f;
	static constexpr float HOMING_SHOT_SPEED = 25.0f;

	// プレイヤーを囲む光柱の本数・半径・寿命・ヒット後の無敵時間・回る速さ
	static constexpr int SURROUND_PILLAR_COUNT = 8;
	static constexpr float SURROUND_RADIUS = 3.0f;
	static constexpr float SURROUND_PILLAR_LIFE_TIME = 8.0f;
	static constexpr float SURROUND_PILLAR_INVINCIBLE_TIME = 0.5f;
	static constexpr float SURROUND_ANGULAR_SPEED = 1.0f;

	// 囲み攻撃を繰り返す回数と、次の波までの間隔
	static constexpr int SURROUND_WAVE_COUNT = 2;
	static constexpr float SURROUND_WAVE_INTERVAL = 1.0f;

	std::vector<Projectile*> groupSpiral1, groupSpiral2, groupHoming;

	float waitTimer = 0.0f;
	static constexpr float WAIT_DURATION = 1.0f;
	float recoveryTimer = 0.0f;
	static constexpr float RECOVERY_DURATION = 6.0f;
	bool playedEffect = false;
	Effekseer::Handle handle = -1;

	int shotCount = 0;
	float shotIntervalTimer = 0.0f;
	static constexpr float SHOT_INTERVAL = 0.5f;
	int waveCount = 0;
	static constexpr float CIRCLE_WAIT_TIME = 1.5f;
	// 発射直前、光柱の回転を止めて狙いを定める時間
	static constexpr float LOCK_ON_TIME = 0.3f;
	float currentRotationAngle = 0.0f;

	State Run(float elapsedTime) override
	{
		auto* model = this->GetModel();
		float frame = this->GetAnimationFrame();
		AnimationConfig* config = this->GetCurrentAnimationConfig();

		if (this->step >= CHARGE_PARTICLE_FIRST_STEP && this->step <= CHARGE_PARTICLE_LAST_STEP)
		{
			// 0.0～1.0 の乱数
			auto random01 = []() { return static_cast<float>(rand()) / RAND_MAX; };

			for (int i = 0; i < CHARGE_PARTICLE_COUNT; ++i)
			{
				float angle = random01() * XM_2PI;
				float dist = CHARGE_PARTICLE_MIN_DISTANCE + random01() * CHARGE_PARTICLE_DISTANCE_RANGE;
				float height = random01() * CHARGE_PARTICLE_HEIGHT_RANGE;

				XMFLOAT3 bossPos = this->owner->GetPosition();
				Vector3 emitPos(bossPos.x + cosf(angle) * dist, bossPos.y + height, bossPos.z + sinf(angle) * dist);
				XMFLOAT3 velocity =
				{
					(random01() - 0.5f) * CHARGE_PARTICLE_DRIFT_SPEED,
					CHARGE_PARTICLE_MIN_RISE_SPEED + random01() * CHARGE_PARTICLE_RISE_SPEED_RANGE,
					(random01() - 0.5f) * CHARGE_PARTICLE_DRIFT_SPEED
				};
				float size = CHARGE_PARTICLE_MIN_SIZE + random01() * CHARGE_PARTICLE_SIZE_RANGE;
				float lifeTime = CHARGE_PARTICLE_MIN_LIFE + random01() * CHARGE_PARTICLE_LIFE_RANGE;

				EffectManager::Instance().EmitGpuParticle(emitPos, velocity, CHARGE_PARTICLE_COLOR, size, lifeTime, GpuParticleBehavior::Drift);
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
				this->owner->StartTeleport(CAST_POSITION, this->QUICK_TELEPORT_FADE_OUT_SECONDS);
				this->step++;
			}
			break;

		case 2:
			if (!this->owner->IsTeleporting() && !playedEffect)
			{
				handle = this->owner->magicCircle->Play(this->owner->GetPosition(), MAGIC_CIRCLE_SCALE);
				this->PlayRootMotion(animationIndexes[2], true, this->ACTION_BLEND_SECONDS);
				playedEffect = true;
			}
			waitTimer += elapsedTime;
			if (waitTimer >= WAIT_DURATION) { waitTimer = 0.0f; this->step++; }
			break;

		case 3:
			if (!this->owner->IsTeleporting())
			{
				if (groupSpiral1.empty())
				{
					XMFLOAT3 center = this->owner->GetPosition();

					for (int i = 0; i < SPIRAL_PILLAR_COUNT; ++i)
					{
						ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
						info.owner = this->owner;
						info.moveType = MovementType::Stationary;
						info.scale = PILLAR_SCALE;
						info.radius = PILLAR_HIT_RADIUS;
						info.lifeTime = SPIRAL_PILLAR_LIFE_TIME;
						info.centerPosition = center;
						info.invincibleTime = SPIRAL_PILLAR_INVINCIBLE_TIME;

						float angle = XM_2PI * static_cast<float>(i) / SPIRAL_PILLAR_COUNT;
						info.currentAngle = angle;

						info.currentRadius = SPIRAL_INNER_RADIUS;
						info.spawnPosition = { center.x + cosf(angle) * SPIRAL_INNER_RADIUS, center.y, center.z + sinf(angle) * SPIRAL_INNER_RADIUS };
						if (Projectile* p = ProjectileManager::Instance().Launch(info)) groupSpiral1.push_back(p);

						info.currentRadius = SPIRAL_OUTER_RADIUS;
						info.spawnPosition = { center.x + cosf(angle) * SPIRAL_OUTER_RADIUS, center.y, center.z + sinf(angle) * SPIRAL_OUTER_RADIUS };
						if (Projectile* p = ProjectileManager::Instance().Launch(info)) groupSpiral2.push_back(p);
					}
				}
				waitTimer += elapsedTime;
				if (waitTimer >= WAIT_DURATION) this->step++;
			}
			break;

		case 4:
			for (auto* p : groupSpiral1) if (p && p->IsActive()) p->StartSpiral(SPIRAL_ANGULAR_SPEED, SPIRAL_EXPAND_SPEED);
			for (auto* p : groupSpiral2) if (p && p->IsActive()) p->StartSpiral(-SPIRAL_ANGULAR_SPEED, SPIRAL_EXPAND_SPEED);
			groupSpiral1.clear(); groupSpiral2.clear();
			this->step++;
			break;

		case 5:
			shotCount = 0;
			shotIntervalTimer = SHOT_INTERVAL;
			groupHoming.clear();
			this->step++;
			break;

		case 6:
			shotIntervalTimer += elapsedTime;
			if (shotIntervalTimer >= SHOT_INTERVAL)
			{
				shotIntervalTimer = 0.0f;
				ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
				info.owner = this->owner;
				info.scale = PILLAR_SCALE;
				info.radius = PILLAR_HIT_RADIUS;
				info.lifeTime = HOMING_SHOT_LIFE_TIME;

				Vector3 spawnPos = this->owner->CalculateVisibleTeleportPos(HOMING_SHOT_SPAWN_DISTANCE);
				spawnPos.y = this->owner->GetPosition().y;
				info.spawnPosition = spawnPos;

				if (Projectile* p = ProjectileManager::Instance().Launch(info))
					p->FireAt(this->PlayerPosition(), HOMING_SHOT_SPEED, true);

				if (++shotCount >= HOMING_SHOT_COUNT) this->step++;
			}
			break;

		case 7:
			waitTimer = 0.0f;
			this->step++;
			break;

		case 8:
		{
			groupHoming.clear();
			XMFLOAT3 playerPos = Player::Instance().GetPosition();

			for (int i = 0; i < SURROUND_PILLAR_COUNT; ++i)
			{
				ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
				info.owner = this->owner;
				info.moveType = MovementType::Stationary;
				info.scale = PILLAR_SCALE;
				info.radius = PILLAR_HIT_RADIUS;
				info.lifeTime = SURROUND_PILLAR_LIFE_TIME;
				info.invincibleTime = SURROUND_PILLAR_INVINCIBLE_TIME;

				float angle = XM_2PI * static_cast<float>(i) / SURROUND_PILLAR_COUNT;
				info.spawnPosition = { playerPos.x + cosf(angle) * SURROUND_RADIUS, this->owner->GetPosition().y, playerPos.z + sinf(angle) * SURROUND_RADIUS };

				if (Projectile* p = ProjectileManager::Instance().Launch(info)) groupHoming.push_back(p);
			}
			waitTimer = 0.0f;
			this->step++;
			break;
		}

		case 9:
			waitTimer += elapsedTime;
			if (waitTimer < CIRCLE_WAIT_TIME - LOCK_ON_TIME)
			{
				currentRotationAngle += SURROUND_ANGULAR_SPEED * elapsedTime;
				XMFLOAT3 playerPos = Player::Instance().GetPosition();

				for (int i = 0; i < static_cast<int>(groupHoming.size()); ++i)
				{
					if (Projectile* p = groupHoming[i]; p && p->IsActive())
					{
						float angle = (XM_2PI * static_cast<float>(i) / groupHoming.size()) + currentRotationAngle;
						p->SetPosition({ playerPos.x + cosf(angle) * SURROUND_RADIUS, this->owner->GetPosition().y, playerPos.z + sinf(angle) * SURROUND_RADIUS });
					}
				}
			}
			if (waitTimer >= CIRCLE_WAIT_TIME) this->step++;
			break;

		case 10:
			for (auto* p : groupHoming) if (p && p->IsActive()) p->FireAt(this->PlayerPosition(), this->PILLAR_FIRE_SPEED, true);
			groupHoming.clear();

			if (++waveCount < SURROUND_WAVE_COUNT) { waitTimer = 0.0f; this->step = 11; }
			else this->step = 12;
			break;

		case 11:
			waitTimer += elapsedTime;
			if (waitTimer >= SURROUND_WAVE_INTERVAL) this->step = 5;
			break;

		case 12:
			this->PlayRootMotion(animationIndexes[3], true, this->ACTION_BLEND_SECONDS);
			this->owner->magicCircle->Stop(handle);
			handle = -1;
			recoveryTimer += elapsedTime;
			if (recoveryTimer >= RECOVERY_DURATION)
			{
				this->PlayRootMotion(animationIndexes[4], true, this->ACTION_BLEND_SECONDS);
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
		this->owner->SetGravity(Character::DEFAULT_GRAVITY);
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
	static constexpr float INTERVAL_DURATION = 0.7f;
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
			this->owner->StartTeleport(Player::Instance().launchKnockbackPosition, this->QUICK_TELEPORT_FADE_OUT_SECONDS);
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
			if (intervalTimer >= INTERVAL_DURATION)
			{
				intervalTimer = 0.0f;
				this->step++;
			}
			break;

		case 2:
			this->owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);
			if (frame >= config->advanceInputEndFrame && !this->owner->IsTeleporting())
			{
				this->owner->SetGravity(this->DIVE_GRAVITY);
				this->PlayRootMotion(animationIndexes[1], false);
				this->step++;
			}
			break;

		case 3:
			if (frame >= config->advanceInputEndFrame)
			{
				this->step = 0;
				this->owner->SetGravity(Character::DEFAULT_GRAVITY);
				this->owner->SetRevenge(false);
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
	static constexpr float INTERVAL_DURATION = 0.7f;
	int animationIndexes[3] = {
		this->owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_03_Seq_0"),
		this->owner->GetModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0")
	};
	Vector3 teleportPosition;
	std::vector<Projectile*> spawnedProjectiles;

	// テレポートの消失時間
	static constexpr float TELEPORT_FADE_OUT_SECONDS = 0.2f;

	// テレポート中にプレイヤーへ寄っていく速さの倍率
	static constexpr float TELEPORT_DRIFT_SPEED_RATE = 0.1f;

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
				this->owner->StartTeleport(teleportPosition, TELEPORT_FADE_OUT_SECONDS);
				this->step++;
			}
			break;

		case 2:
			this->owner->MoveToTarget(elapsedTime, TELEPORT_DRIFT_SPEED_RATE);
			if (!this->owner->IsTeleporting())
			{
				this->PlayRootMotion(animationIndexes[1], false);

				if (this->IsHealthBelowRate(this->PILLAR_SUPPORT_HEALTH_RATE) && spawnedProjectiles.empty())
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
				this->owner->SetGravity(this->DIVE_GRAVITY);
				this->PlayRootMotion(animationIndexes[2], false);
				this->step++;
			}
			break;

		case 4:
			if (frame >= config->advanceInputStartFrame)
			{
				this->FireAndClear(spawnedProjectiles, this->PILLAR_FIRE_SPEED, true);
				this->step++;
			}
			break;

		case 5:
			if (frame >= config->advanceInputEndFrame)
			{
				spawnedProjectiles.clear();
				this->step = 0;
				this->owner->SetGravity(Character::DEFAULT_GRAVITY);
				this->owner->SetSuperArmor(false);
				this->owner->SetRevenge(false);
				return State::Complete;
			}
			break;
		}
		return State::Run;
	}
};