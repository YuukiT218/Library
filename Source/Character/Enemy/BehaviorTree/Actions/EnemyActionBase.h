#pragma once

#include "Character/Enemy/BehaviorTree/ActionBase.h"
#include "Character/Enemy/BehaviorTree/NodeBase.h"
#include "Character/Enemy/BehaviorTree/BehaviorData.h"
#include "Character/Player/Player.h"
#include "Character/Projectile/ProjectileManager.h"
#include "Math/Mathf.h"

#include <algorithm>
#include <cmath>
#include <vector>

// 敵の行動クラスが共通して使う処理をまとめた基底クラス
//
// 各行動クラスは step を進めながらアニメーションを再生し、
// 被弾・死亡で中断されるという同じ流れを持つため、
// その定型部分をここに集約している。
template <typename ActorType>
class EnemyActionBase : public ActionBase<ActorType>
{
public:
	using ActionBase<ActorType>::ActionBase;

	// 派生クラスで State::Complete のように短く書けるようにする
	using State = typename ActionBase<ActorType>::State;

protected:
	// アニメーション設定を引くときのキャラクター名
	static constexpr const char* ANIMATION_CONFIG_OWNER = "EnemyBoss";

	// ルートモーションを適用するノード名
	static constexpr const char* ROOT_MOTION_NODE_NAME = "root";

	// 戦闘待機アニメーション名
	static constexpr const char* IDLE_ANIMATION_NAME = "Idle_Combat_Seq_0";

	// 行動中に使うアニメーションのブレンド時間
	static constexpr float ACTION_BLEND_SECONDS = 0.2f;

	// 空中で滞空させるときの重力
	static constexpr float HOVER_GRAVITY = -0.001f;

	// 空中から叩きつけるときの重力
	static constexpr float DIVE_GRAVITY = -1.5f;

	// 素早いテレポートの消失時間
	static constexpr float QUICK_TELEPORT_FADE_OUT_SECONDS = 0.1f;

	// 光柱をプレイヤーへ撃ち出すときの速さ
	static constexpr float PILLAR_FIRE_SPEED = 20.0f;

	// 体力がこの割合以下になると、攻撃に光柱の追撃が加わる
	static constexpr float PILLAR_SUPPORT_HEALTH_RATE = 0.5f;

	// 攻撃予兆エフェクトを出す高さ
	static constexpr float ATTACK_SIGN_OFFSET_Y = 1.0f;

	//----------------------------------------------------------------
	// モデル・アニメーション
	//----------------------------------------------------------------

	Model* GetModel() const { return this->owner->GetModel(); }

	// 再生中アニメーションの経過秒
	float GetAnimationFrame() const { return GetModel()->GetCurrentAnimationSeconds(); }

	// 再生中アニメーションの設定を取得
	AnimationConfig* GetCurrentAnimationConfig() const
	{
		Model* model = GetModel();
		return model->GetAnimationConfig(ANIMATION_CONFIG_OWNER, model->GetCurrentAnimationIndex());
	}

	// 指定アニメーションの設定を取得
	AnimationConfig* GetAnimationConfig(int animationIndex) const
	{
		return GetModel()->GetAnimationConfig(ANIMATION_CONFIG_OWNER, animationIndex);
	}

	// ルートモーション付きでアニメーションを再生する
	void PlayRootMotion(int animationIndex, bool loop, float blendSeconds)
	{
		GetModel()->PlayRootMotion(animationIndex, loop, true, blendSeconds, ROOT_MOTION_NODE_NAME);
	}

	// ルートモーション付きでアニメーションを再生する（既定のブレンド時間）
	void PlayRootMotion(int animationIndex, bool loop)
	{
		PlayRootMotion(animationIndex, loop, this->owner->GetBlendSeconds());
	}

	//----------------------------------------------------------------
	// プレイヤーへの追従・攻撃判定
	//----------------------------------------------------------------

	static const DirectX::XMFLOAT3& PlayerPosition() { return Player::Instance().GetPosition(); }

	// 自分の位置に攻撃予兆エフェクトを出す
	void PlayAttackSign()
	{
		const DirectX::XMFLOAT3& selfPosition = this->owner->GetPosition();
		this->owner->attackSign->Play({ selfPosition.x, selfPosition.y + ATTACK_SIGN_OFFSET_Y, selfPosition.z });
	}

	// 剣の攻撃判定を更新する（プレイヤーが無敵中は行わない）
	void UpdateAttackCollision()
	{
		if (this->owner->isPlayerInvincible) return;

		this->owner->GetSword()->AttackAnimationCollision(
			GetModel(), GetCurrentAnimationConfig(), this->owner->GetCharacter());
	}

	// プレイヤーを追従対象に設定し、あわせて攻撃判定も更新する
	void UpdateTargetingAndAttack()
	{
		this->owner->SetTargetPosition(PlayerPosition());
		UpdateAttackCollision();
	}

	// 近接攻撃が届く距離の目安（XZ 平面）
	//
	// 剣の当たり判定球は柄元から 1.35 の位置まで並び、半径が 0.225 なので、
	// 手元からのリーチはおよそ 1.6。踏み込みと互いの体の太さを見込んでこの値にしている。
	static constexpr float MELEE_REACH = 2.2f;

	// プレイヤーとの距離（XZ 平面）
	//
	// IsNearPlayer は軸ごとの判定なので、真横なら指定どおりでも、
	// 斜めだとその約 1.4 倍離れていても「近い」と判定されてしまう。
	// 間合いを測るときはこちらを使う。
	float GetDistanceToPlayerXZ() const
	{
		const DirectX::XMFLOAT3 selfPosition = this->owner->GetPosition();
		const DirectX::XMFLOAT3 playerPosition = PlayerPosition();

		const float dx = playerPosition.x - selfPosition.x;
		const float dz = playerPosition.z - selfPosition.z;
		return std::sqrt(dx * dx + dz * dz);
	}

	// 攻撃が届く間合いに入っているか
	bool IsWithinReach(float reach) const
	{
		return GetDistanceToPlayerXZ() <= reach;
	}

	// プレイヤーに十分近づいているか
	bool IsNearPlayer(const DirectX::SimpleMath::Vector3& epsilon) const
	{
		DirectX::XMFLOAT3 playerPosition = PlayerPosition();
		return DirectX::XMVector3NearEqual(
			DirectX::XMLoadFloat3(&this->owner->GetPosition()),
			DirectX::XMLoadFloat3(&playerPosition),
			epsilon);
	}

	// プレイヤーへ線形補完で近づく
	void LerpTowardPlayer(float t)
	{
		const DirectX::XMFLOAT3& selfPosition = this->owner->GetPosition();
		DirectX::XMFLOAT3 playerPosition = PlayerPosition();

		this->owner->SetPosition({
			Mathf::Lerp(selfPosition.x, playerPosition.x, t),
			Mathf::Lerp(selfPosition.y, playerPosition.y, t),
			Mathf::Lerp(selfPosition.z, playerPosition.z, t)
			});
	}

	// 体力が最大値の指定割合以下か
	bool IsHealthBelowRate(float rate) const
	{
		return this->owner->GetHealth() <= this->owner->GetMaxHealth() * rate;
	}

	//----------------------------------------------------------------
	// 飛び道具
	//----------------------------------------------------------------

	// 保持している弾をすべてプレイヤーへ撃ち出し、リストを空にする
	void FireAndClear(std::vector<Projectile*>& projectiles, float speed, bool homing)
	{
		for (Projectile* projectile : projectiles)
		{
			if (projectile && projectile->IsActive())
			{
				projectile->FireAt(PlayerPosition(), speed, homing);
			}
		}
		projectiles.clear();
	}

	// 自分を中心に円状へ光柱を並べる
	void SpawnLightPillarCircle(std::vector<Projectile*>& projectiles,
		const DirectX::XMFLOAT3& center, int count, float radius,
		float scale, float hitRadius, float lifeTime, float invincibleTime,
		bool recordOrbit)
	{
		for (int i = 0; i < count; ++i)
		{
			ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
			info.owner = this->owner;
			info.moveType = MovementType::Stationary;
			info.scale = scale;
			info.radius = hitRadius;
			info.lifeTime = lifeTime;
			info.invincibleTime = invincibleTime;

			float angle = DirectX::XM_2PI * static_cast<float>(i) / count;
			if (recordOrbit)
			{
				info.centerPosition = center;
				info.currentRadius = radius;
				info.currentAngle = angle;
			}
			info.spawnPosition =
			{
				center.x + cosf(angle) * radius,
				center.y,
				center.z + sinf(angle) * radius
			};

			if (Projectile* projectile = ProjectileManager::Instance().Launch(info))
			{
				projectiles.push_back(projectile);
			}
		}
	}

	// 自分の左右後方に光柱を2本生成する
	void SpawnSideLightPillars(std::vector<Projectile*>& projectiles)
	{
		constexpr int PILLAR_COUNT = 2;
		constexpr float SIDE_DISTANCE = 3.0f;
		constexpr float BACK_DISTANCE = 2.0f;
		constexpr float SPAWN_HEIGHT = -1.0f;
		constexpr float PILLAR_SCALE = 0.5f;
		constexpr float PILLAR_HIT_RADIUS = 0.7f;
		constexpr float PILLAR_LIFE_TIME = 5.0f;
		constexpr float PILLAR_SPEED = 40.0f;

		const DirectX::XMFLOAT3& angle = this->owner->GetAngle();
		DirectX::XMFLOAT3 back = this->owner->CharacterBack(angle);
		DirectX::XMFLOAT3 left = this->owner->CharacterLeft(angle);
		DirectX::XMFLOAT3 right = this->owner->CharacterRight(angle);
		const DirectX::XMFLOAT3& selfPosition = this->owner->GetPosition();

		for (int i = 0; i < PILLAR_COUNT; ++i)
		{
			ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
			info.owner = this->owner;
			info.moveType = MovementType::Stationary;
			info.scale = PILLAR_SCALE;
			info.radius = PILLAR_HIT_RADIUS;
			info.lifeTime = PILLAR_LIFE_TIME;
			info.speed = PILLAR_SPEED;

			const DirectX::XMFLOAT3& side = (i == 0) ? left : right;
			info.spawnPosition =
			{
				selfPosition.x + (back.x - side.x) * SIDE_DISTANCE,
				SPAWN_HEIGHT,
				selfPosition.z + (back.z - side.z) * BACK_DISTANCE
			};

			if (Projectile* projectile = ProjectileManager::Instance().Launch(info))
			{
				projectiles.push_back(projectile);
			}
		}
	}

	//----------------------------------------------------------------
	// 中断・リセット
	//----------------------------------------------------------------

	// 被弾または死亡によって行動を中断すべきか
	bool IsInterrupted() const
	{
		return this->owner->IsAnyDamage() || this->owner->IsDeathFlag();
	}

	// 派生クラスが独自の状態を初期化するためのフック
	virtual void OnReset() {}

	// step と派生クラスの状態をリセットし、指定された結果を返す
	State ResetState(State state)
	{
		this->step = 0;
		OnReset();
		return state;
	}
};
