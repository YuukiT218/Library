#pragma once
#include "EnemyActionBase.h"
#include "Character/Player/Player.h"
#include "Character/Projectile/ProjectileManager.h"
#include "Math/Mathf.h"
#include "Effect/EffectManager.h"

// 通常テレポート
template <typename ActorType>
class NormalTeleport : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	NormalTeleport(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}
	State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Dodge_Combat_B_Seq_0");
	DirectX::SimpleMath::Vector3 teleportPosition;

	// テレポート先のプレイヤーからの距離
	static constexpr float TELEPORT_DISTANCE = 12.5f;

	// テレポートの消失時間
	static constexpr float TELEPORT_FADE_OUT_SECONDS = 0.3f;
};

// 三連テレポート
template <typename ActorType>
class TripleTeleportAction : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	TripleTeleportAction(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}
	State Run(float elapsedTime);
	// 跳ねる回数
	static constexpr int HOP_COUNT = 3;

	// 最後に跳ぶ先のプレイヤーからの距離
	static constexpr float LAST_HOP_DISTANCE = 2.0f;

	// 開始時に向きを合わせる旋回速度
	static constexpr float INITIAL_TURN_SPEED = DirectX::XM_2PI * 10.0f;

	// 開始時に撃つ光柱の出現位置（自分の前方への距離）
	static constexpr float PILLAR_SPAWN_FORWARD_DISTANCE = 1.5f;

	// 姿の代わりに見せる光の玉の大きさ
	static constexpr float LIGHT_BALL_SCALE = 0.5f;

	// 軌跡の粒子を撒く高さ（足元からのオフセット）
	static constexpr float TRAIL_OFFSET_Y = 1.0f;

	// 1フレームで撒く軌跡の粒子の数と散らばり具合
	static constexpr int TRAIL_EMIT_COUNT = 10;
	static constexpr float TRAIL_SPREAD = 0.5f;

	// 軌跡の粒子の速度・色・大きさ・寿命
	static constexpr float TRAIL_DRIFT_SPEED = 0.5f;
	static constexpr float TRAIL_RISE_SPEED = 1.0f;
	static constexpr DirectX::XMFLOAT4 TRAIL_COLOR = { 1.0f, 0.9f, 0.2f, 1.0f };
	static constexpr float TRAIL_MIN_SIZE = 0.1f;
	static constexpr float TRAIL_SIZE_RANGE = 0.3f;
	static constexpr float TRAIL_MIN_LIFE = 0.2f;
	static constexpr float TRAIL_LIFE_RANGE = 0.3f;

	DirectX::SimpleMath::Vector3 warpPositions[HOP_COUNT];
	DirectX::SimpleMath::Vector3 currentStartPosition;
	bool init = false;
	float timer = 0.0f;
	float duration = 0.4f; // 移動にかける時間
	float arcHeight = 1.5f;
	DirectX::SimpleMath::Vector3 warpEpsilon{ 0.1f, 10.0f, 0.1f, };
	int count = 0;
	Effekseer::Handle handle = -1;
	float intervalTimer = 0.0f;
	static constexpr float INTERVAL_DURATION = 0.2f; // 移動間のわずかな間
	float recoveryTimer = 0.0f;
	static constexpr float RECOVERY_DURATION = 1.3f; // 終了後の後隙
};

// 様子見歩き
template <typename ActorType>
class CautiousWalkAction : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	CautiousWalkAction(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}
	State Run(float elapsedTime);
	int animationIndexes[2] = { owner->GetModel()->GetAnimationIndex("Walk_Combat_Loop_F_L_90_RM_Seq_0"),
								owner->GetModel()->GetAnimationIndex("Walk_Combat_Loop_F_R_90_RM_Seq_0") };

	// 歩き続ける時間の範囲
	static constexpr float MIN_WALK_SECONDS = 2.0f;
	static constexpr float MAX_WALK_SECONDS = 4.0f;
};

// 追跡行動
template <typename ActorType>
class PursuitAction : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	PursuitAction(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}
	State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Run_Combat_Loop_F_0_Seq_0");

	// 追いかけ続ける時間の範囲
	static constexpr float MIN_PURSUIT_SECONDS = 1.0f;
	static constexpr float MAX_PURSUIT_SECONDS = 1.5f;
};

// 徘徊行動
template <typename ActorType>
class WanderAction : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	WanderAction(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}
	State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex("Walk_Combat_Loop_F_0_RM_Seq_0");

	// 徘徊するときの移動速度の倍率
	static constexpr float WANDER_SPEED_RATE = 0.1f;
};

// 待機行動
template <typename ActorType>
class IdleAction : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	IdleAction(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}
	State Run(float elapsedTime);
	int animationIndex = owner->GetModel()->GetAnimationIndex(EnemyActionBase<ActorType>::IDLE_ANIMATION_NAME);

	// 待機し続ける時間の範囲
	static constexpr float MIN_IDLE_SECONDS = 3.0f;
	static constexpr float MAX_IDLE_SECONDS = 5.0f;
};

// 落下モーション
template <typename ActorType>
class FallAction : public EnemyActionBase<ActorType>
{
public:
	using State = typename EnemyActionBase<ActorType>::State;
	FallAction(ActorType* actor) : EnemyActionBase<ActorType>(actor) {}
	State Run(float elapsedTime);
	int fallLoopAnimationIndex = owner->GetModel()->GetAnimationIndex("Jump_Combat_Loop_0_Seq_0");
	int landingAnimationIndex = owner->GetModel()->GetAnimationIndex("Jump_Combat_End_0_Seq_0");
};

//-------------------------------------------------------------
// 通常テレポート
template <typename ActorType>
typename EnemyActionBase<ActorType>::State NormalTeleport<ActorType>::Run(float elapsedTime)
{
	owner->SetTargetPosition(Player::Instance().GetPosition());

	float frame = owner->GetModel()->GetCurrentAnimationSeconds();
	AnimationConfig* config = this->GetAnimationConfig(animationIndex);

	switch (step)
	{
	case 0:
		teleportPosition = owner->CalculateVisibleTeleportPos(TELEPORT_DISTANCE, true);

		// アニメーション再生
		this->PlayRootMotion(animationIndex, false);
		step++;
		break;
	case 1:
		owner->TurnToTarget(elapsedTime, TurnSpeed::FAST);

		if (frame >= config->advanceInputEndFrame && !owner->IsTeleporting())
		{
			owner->StartTeleport(teleportPosition, TELEPORT_FADE_OUT_SECONDS);
			step++;
		}
		break;
	case 2:
		if (!owner->IsTeleporting())
		{
			step = 0;
			return State::Complete;
		}
	}
	return State::Run;
}

//-------------------------------------------------------------

//-------------------------------------------------------------
// 三連テレポート
template <typename ActorType>
typename EnemyActionBase<ActorType>::State TripleTeleportAction<ActorType>::Run(float elapsedTime)
{
	owner->SetTargetPosition(Player::Instance().GetPosition());

	// 初期化
	if (!init)
	{
		warpPositions[0] = owner->warpPositions[0];
		warpPositions[1] = owner->warpPositions[1];

		// 最初の始点を保存
		currentStartPosition = owner->GetPosition();

		// 向き調整
		owner->TurnToTarget(elapsedTime, INITIAL_TURN_SPEED);

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
		owner->TurnToTarget(elapsedTime, TurnSpeed::SLOW);
		owner->SetInvincible(true);
		// (前回のコードと同じ弾発射処理)
		ProjectileInfo info = ProjectileManager::GetLightPillarInfo();
		info.owner = owner;

		DirectX::XMFLOAT3 bossPos = owner->GetPosition();
		DirectX::XMFLOAT3 bossForward = owner->CharacterForward(owner->GetAngle());
		info.spawnPosition = bossPos;
		info.spawnPosition.x += bossForward.x * PILLAR_SPAWN_FORWARD_DISTANCE;
		info.spawnPosition.z += bossForward.z * PILLAR_SPAWN_FORWARD_DISTANCE;

		DirectX::XMFLOAT3 targetPos = Player::Instance().GetPosition();
		DirectX::XMFLOAT3 toTarget = {
			targetPos.x - info.spawnPosition.x,
			0.0f,
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
		handle = owner->lightBall->Play(owner->GetPosition(), LIGHT_BALL_SCALE);
		owner->SetScale({ 0.0f, 0.0f, 0.0f }); // 姿を消す（光の玉だけが跳ねるように見せる）
	}
	step++;
	break;

	case 1: // 放物線移動（ホッピング動作）
	{
		owner->TurnToTarget(elapsedTime, TurnSpeed::SLOW);
		timer += elapsedTime;
		// 進行度 (0.0 -> 1.0)
		float t = std::clamp(timer / duration, 0.0f, 1.0f);

		// 座標を更新する前の位置を保存しておく
		DirectX::SimpleMath::Vector3 prevPos = owner->GetPosition();
		prevPos.y += TRAIL_OFFSET_Y;

		// 線形補間（始点と終点の間を直線移動）
		float x = Mathf::Lerp(currentStartPosition.x, warpPositions[count].x, t);
		float z = Mathf::Lerp(currentStartPosition.z, warpPositions[count].z, t);
		float baseY = Mathf::Lerp(currentStartPosition.y, warpPositions[count].y, t);

		// 放物線の高さを計算 (sinカーブを利用)
		// t=0で0, t=0.5で1, t=1.0で0 になる動き * 高さ
		float arc = sinf(t * DirectX::XM_PI) * arcHeight;

		// 座標適用 (Y座標に高さを足す)
		owner->SetPosition({ x, baseY + arc, z });

		// 座標を更新した後の新しい位置を取得
		DirectX::SimpleMath::Vector3 currentPos = owner->GetPosition();
		currentPos.y += TRAIL_OFFSET_Y;

		// エフェクト追従
		owner->lightBall->SetPosition(handle, owner->GetPosition());

		// 0.0～1.0 の乱数
		auto random01 = []() { return static_cast<float>(rand()) / RAND_MAX; };

		for (int i = 0; i < TRAIL_EMIT_COUNT; ++i)
		{
			// 前フレームと今のフレームの間をランダムな割合で補間
			DirectX::SimpleMath::Vector3 emitPos = DirectX::SimpleMath::Vector3::Lerp(prevPos, currentPos, random01());

			// 一本線だと寂しいので、少しだけ周囲に散らす
			emitPos.x += (random01() - 0.5f) * TRAIL_SPREAD;
			emitPos.y += (random01() - 0.5f) * TRAIL_SPREAD;
			emitPos.z += (random01() - 0.5f) * TRAIL_SPREAD;

			// 速度：光の粒子が空中に少し留まってフワッと消えるイメージ
			DirectX::XMFLOAT3 velocity =
			{
				(random01() - 0.5f) * TRAIL_DRIFT_SPEED,
				random01() * TRAIL_RISE_SPEED, // 少し上に向かって漂う
				(random01() - 0.5f) * TRAIL_DRIFT_SPEED
			};

			// サイズと寿命（軌跡なので寿命は短めにスッと消す）
			float size = TRAIL_MIN_SIZE + random01() * TRAIL_SIZE_RANGE;
			float lifeTime = TRAIL_MIN_LIFE + random01() * TRAIL_LIFE_RANGE;

			// 魔法っぽく空中に留まる挙動にする
			EffectManager::Instance().EmitGpuParticle(emitPos, velocity, TRAIL_COLOR, size, lifeTime, GpuParticleBehavior::Float);
		}

		// 着地判定
		if (t >= 1.0f)
		{
			// 位置を目的地に強制補正
			owner->SetPosition(warpPositions[count]);

			// 次のジャンプのために現在の場所を「始点」として保存
			currentStartPosition = warpPositions[count];

			timer = 0.0f;
			count++;

			// 次のステップへ
			if (count >= HOP_COUNT) {
				this->PlayRootMotion(owner->GetModel()->GetAnimationIndex("Run_Combat_Stop_F_0_RM_Seq_0"), false);
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

		// 最後の跳び先だけは、プレイヤーのそばの画面内に決める
		if (count == HOP_COUNT - 1)
		{
			warpPositions[HOP_COUNT - 1] = owner->CalculateVisibleTeleportPos(LAST_HOP_DISTANCE, true);
		}

		if (intervalTimer >= INTERVAL_DURATION)
		{
			intervalTimer = 0.0f;
			step = 0; // 次の放物線ジャンプへ
		}
		break;

	case 3: // 終了処理＆後隙
		if (!owner->GetModel()->IsPlayAnimation())
			this->PlayRootMotion(owner->GetModel()->GetAnimationIndex(this->IDLE_ANIMATION_NAME), true, this->ACTION_BLEND_SECONDS);
		owner->SetInvincible(false);
		owner->lightBall->Stop(handle);
		owner->SetScale({ 1.0f, 1.0f, 1.0f });
		handle = -1;
		recoveryTimer += elapsedTime;
		if (recoveryTimer >= RECOVERY_DURATION)
		{
			// 姿を現す
			init = false;
			step = 0;
			count = 0;
			return State::Complete;
		}
		break;
	}
	return State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 様子見歩き
template <typename ActorType>
typename EnemyActionBase<ActorType>::State CautiousWalkAction<ActorType>::Run(float elapsedTime)
{
	float runTimer;
	switch (step)
	{
	case 0:
	{
		int randomIndex = static_cast<int>(Mathf::RandomRange(0.0f, 1.0f));
		this->PlayRootMotion(animationIndexes[randomIndex], true);
		owner->SetRunTimer(Mathf::RandomRange(MIN_WALK_SECONDS, MAX_WALK_SECONDS));
		step++;
		break;
	}
	case 1:
		runTimer = owner->GetRunTimer() - elapsedTime;
		owner->SetRunTimer(runTimer);
		if (runTimer <= 0.0f)
		{
			step = 0;
			return State::Complete;
		}
		break;
	}
	return State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 徘徊行動
template <typename ActorType>
typename EnemyActionBase<ActorType>::State WanderAction<ActorType>::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		// 徘徊モーション設定
		this->PlayRootMotion(animationIndex, true);
		step++;
		break;
	case 1:
		if (owner->GetHealth() <= 0)
		{
			step = 0;
			return State::Complete;
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
			return State::Complete;
		}

		// 目的地点へ移動
		owner->MoveToTarget(elapsedTime, WANDER_SPEED_RATE);

		// プレイヤー索敵成功したら
		if (owner->SearchPlayer())
		{
			step = 0;
			// 徘徊成功を返す
			return State::Complete;
		}
		break;
	}
	if (this->IsInterrupted())
	{
		step = 0;
		return State::Failed;
	}
	// 実行中を返す
	return State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 追跡行動
template <typename ActorType>
typename EnemyActionBase<ActorType>::State PursuitAction<ActorType>::Run(float elapsedTime)
{
	float runTimer = owner->GetRunTimer();
	switch (step)
	{
	case 0:
		// 目標地点をプレイヤー位置に設定
		owner->SetTargetPosition(Player::Instance().GetPosition());
		owner->SetRunTimer(Mathf::RandomRange(MIN_PURSUIT_SECONDS, MAX_PURSUIT_SECONDS));
		this->PlayRootMotion(animationIndex, true);
		step++;
		break;
	case 1:
		if (owner->GetHealth() <= 0)
		{
			step = 0;
			return State::Complete;
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
			return State::Complete;
		}
		// 行動時間が過ぎた時
		if (runTimer <= 0.0f)
		{
			step = 0;
			// 追跡失敗を返す
			return State::Failed;
		}
		break;
	}
	if (this->IsInterrupted())
	{
		step = 0;
		return State::Failed;
	}
	// 実行中を返す
	return State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 待機行動
template <typename ActorType>
typename EnemyActionBase<ActorType>::State IdleAction<ActorType>::Run(float elapsedTime)
{
	float runTimer = owner->GetRunTimer();
	switch (step)
	{
	case 0:
		owner->SetRunTimer(Mathf::RandomRange(MIN_IDLE_SECONDS, MAX_IDLE_SECONDS));
		owner->GetModel()->PlayAnimation(animationIndex, true, owner->GetBlendSeconds());
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
			return State::Complete;
		}

		// プレイヤーを見つけた時
		if (owner->SearchPlayer())
		{
			step = 0;
			return State::Complete;
		}
		break;
	}
	if (owner->IsDamage() || owner->IsDeathFlag())
	{
		step = 0;
		return State::Failed;
	}
	// 実行中を返す
	return State::Run;
}
//-------------------------------------------------------------

//-------------------------------------------------------------
// 落下モーション
template <typename ActorType>
typename EnemyActionBase<ActorType>::State FallAction<ActorType>::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		if (!owner->IsGround())
		{
			this->PlayRootMotion(fallLoopAnimationIndex, true);
		}
		if (owner->IsGround())
		{
			this->PlayRootMotion(landingAnimationIndex, false);
			step++;
		}
		break;
	case 1:
		if (!owner->GetModel()->IsPlayAnimation())
		{
			step = 0;
			return State::Complete;
		}
		break;
	}
	if (owner->IsDamage() || owner->IsDeathFlag())
	{
		step = 0;
		return State::Complete;
	}
	return State::Run;
}
//-------------------------------------------------------------
