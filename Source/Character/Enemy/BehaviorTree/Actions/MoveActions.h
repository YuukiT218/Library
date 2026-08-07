#pragma once
#include "ActionBase.h"
#include "Character/Player/Player.h"
#include "Character\Projectile/ProjectileManager.h"
#include "Math\Mathf.h"
#include "Effect/EffectManager.h"

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
	DirectX::SimpleMath::Vector3 warpEpsilon{ 0.1f, 10.0f, 0.1f, };
	int count = 0;
	Effekseer::Handle handle = -1;
	float intervalTimer = 0.0f;
	const float intervalDuration = 0.2f; // 移動間のわずかな間
	float recoveryTimer = 0.0f;
	const float recoveryDuration = 1.3f; // 終了後の後隙
};

// 様子見歩き
template <typename ActorType>
class CautiousWalkAction : public ActionBase<ActorType>
{
public:
	CautiousWalkAction(ActorType* actor) : ActionBase(actor) {}
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
		owner->TurnToTarget(elapsedTime, 10.0f);
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
		owner->TurnToTarget(elapsedTime, 10.0f);
		timer += elapsedTime;
		// 進行度 (0.0 -> 1.0)
		float t = std::clamp(timer / duration, 0.0f, 1.0f);

		// 座標を更新する前の位置を保存しておく
		DirectX::SimpleMath::Vector3 prevPos = owner->GetPosition();
		prevPos.y += 1;

		// 線形補間（始点と終点の間を直線移動）
		float x = Mathf::Lerp(currentStartPosition.x, WarpPosition[count].x, t);
		float z = Mathf::Lerp(currentStartPosition.z, WarpPosition[count].z, t);
		float base_y = Mathf::Lerp(currentStartPosition.y, WarpPosition[count].y, t);

		// 放物線の高さを計算 (sinカーブを利用)
		// t=0で0, t=0.5で1, t=1.0で0 になる動き * 高さ
		float arc = sinf(t * DirectX::XM_PI) * arcHeight;

		// 座標適用 (Y座標に高さを足す)
		owner->SetPosition({ x, base_y + arc, z });

		// 座標を更新した後の新しい位置を取得
		DirectX::SimpleMath::Vector3 currentPos = owner->GetPosition();
		currentPos.y += 1;

		// エフェクト追従
		owner->lightBall->SetPosition(handle, owner->GetPosition());

		int emitCount = 10; // 軌跡の密度（お好みで調整）

		for (int i = 0; i < emitCount; ++i)
		{
			// 前フレームと今のフレームの間をランダムな割合で補間
			float lerpT = (float)rand() / RAND_MAX;
			DirectX::SimpleMath::Vector3 emitPos = DirectX::SimpleMath::Vector3::Lerp(prevPos, currentPos, lerpT);

			// 一本線だと寂しいので、少しだけ周囲に散らす（半径0.5の範囲でブレさせる）
			emitPos.x += ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
			emitPos.y += ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
			emitPos.z += ((float)rand() / RAND_MAX - 0.5f) * 0.5f;

			// 速度：光の粒子が空中に少し留まってフワッと消えるイメージ
			float vx = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
			float vy = ((float)rand() / RAND_MAX) * 1.0f; // 少し上に向かって漂う
			float vz = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
			DirectX::XMFLOAT3 velocity = { vx, vy, vz };

			// 色：光の玉に合わせた色（例は明るい黄色/ゴールド系）
			DirectX::XMFLOAT4 color = { 1.0f, 0.9f, 0.2f, 1.0f };

			// サイズと寿命（軌跡なので寿命は短めにスッと消す）
			float size = 0.1f + ((float)rand() / RAND_MAX) * 0.3f;
			float lifeTime = 0.2f + ((float)rand() / RAND_MAX) * 0.3f;

			// パーティクル発生 (behaviorTypeは以前作成した、魔法っぽく空中に留まる 1 を指定すると綺麗です)
			EffectManager::Instance().EmitGpuParticle(emitPos, velocity, color, size, lifeTime, 1);
		}

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
				owner->GetModel()->PlayRootMotion(owner->GetModel()->GetAnimationIndex("Run_Combat_Stop_F_0_RM_Seq_0"), false, true, owner->GetBlendSeconds(), "root");
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
		if (!owner->GetModel()->IsPlayAnimation())
			owner->GetModel()->PlayRootMotion(owner->GetModel()->GetAnimationIndex("Idle_Combat_Seq_0"), true, true, 0.2f, "root");
		owner->SetInvincible(false);
		owner->lightBall->Stop(handle);
		owner->SetScale({ 1,1,1 });
		handle = -1;
		recoveryTimer += elapsedTime;
		if (recoveryTimer >= recoveryDuration)
		{
			// 姿を現す
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
		owner->SetRunTimer(Mathf::RandomRange(1.0f, 1.5f));
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
