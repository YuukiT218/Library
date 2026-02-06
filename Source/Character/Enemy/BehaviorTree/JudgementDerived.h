#pragma once
#include "JudgementBase.h"

//-------------------------------------------------------------
// 判定クラス宣言
//-------------------------------------------------------------
// BattleNodeに遷移できるか判定
template <typename ActorType>
class BattleJudgment : public JudgmentBase<ActorType>
{
public:
	BattleJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

// RoarNodeに遷移できるか判定
template <typename ActorType>
class RoarJudgment : public JudgmentBase<ActorType>
{
public:
	RoarJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

// AttackNodeに遷移できるか判定
template <typename ActorType>
class AttackJudgment : public JudgmentBase<ActorType>
{
public:
	AttackJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

// LongRangeNodeに遷移できるか判定
template <typename ActorType>
class LongRangeJudgment : public JudgmentBase<ActorType>
{
public:
	LongRangeJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

// SkillNodeに遷移できるか判定
template <typename ActorType>
class TackleJudgment : public JudgmentBase<ActorType>
{
public:
	TackleJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

// WanderNodeに遷移できるか判定
template <typename ActorType>
class WanderJudgment : public JudgmentBase<ActorType>
{
public:
	WanderJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

// DamageNodeに遷移できるか判定
// 任意のダメージを検出する判定クラス
template <typename ActorType>
class AnyDamageJudgment : public JudgmentBase<ActorType>
{
public:
	AnyDamageJudgment(ActorType* actor) : JudgmentBase<ActorType>(actor) {}

	bool Judgment();
};

// DeadNodeに遷移できるか判定
template <typename ActorType>
class DeadJudgment : public JudgmentBase<ActorType>
{
public:
	DeadJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

// 体力が半分以上
template <typename ActorType>
class FineJudgment : public JudgmentBase<ActorType>
{
public:
	FineJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

// 体力が半分以下
template <typename ActorType>
class DyingJudgment : public JudgmentBase<ActorType>
{
public:
	DyingJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

// 落下中か判定
template <typename ActorType>
class FallJudgment : public JudgmentBase<ActorType>
{
public:
	FallJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

// 反撃値が一定以上まで蓄積したか判定
template <typename ActorType>
class RevengeJudgment : public JudgmentBase<ActorType>
{
public:
	RevengeJudgment(ActorType* actor) :JudgmentBase(actor) {};
	// 判定
	bool Judgment();
};

template <typename ActorType>
class SpecialAttackJudgment : public JudgmentBase<ActorType>
{
public:
	SpecialAttackJudgment(ActorType* actor) :JudgmentBase(actor) {}
	bool Judgment();
};
//-------------------------------------------------------------

//-------------------------------------------------------------
// 判定クラス処理部分
//-------------------------------------------------------------
// BattleNodeに遷移できるか判定
template <typename ActorType>
bool BattleJudgment<ActorType>::Judgment()
{
	// プレイヤーを見つけているか
	if (owner->SearchPlayer())
	{
		return true;
	}
	return false;
}

// RoarNodeに遷移できるか判定
template <typename ActorType>
bool RoarJudgment<ActorType>::Judgment()
{
	// 咆哮を既に使用済みの場合falseを返す
	if (!owner->RoarUsedFlag)
	{
		return true;
	}
	return false;
}

// AttackNodeに遷移できるか判定
template <typename ActorType>
bool AttackJudgment<ActorType>::Judgment()
{
	// 対象との距離を算出
	DirectX::XMFLOAT3 position = owner->GetPosition();
	DirectX::XMFLOAT3 targetPosition = Player::Instance().GetPosition();

	float vx = targetPosition.x - position.x;
	float vy = targetPosition.y - position.y;
	float vz = targetPosition.z - position.z;
	float dist = sqrtf(vx * vx + vy * vy + vz * vz);

	// XZ平面での距離を算出
	if (dist < owner->GetAttackRange())
	{
		// AttackNodeへ遷移できる
		return true;
	}
	return false;
}

// LongRangeNodeに遷移できるか判定
template <typename ActorType>
bool LongRangeJudgment<ActorType>::Judgment()
{
	// 対象との距離を算出
	DirectX::XMFLOAT3 position = owner->GetPosition();
	DirectX::XMFLOAT3 targetPosition = Player::Instance().GetPosition();
	float vx = targetPosition.x - position.x;
	float vy = targetPosition.y - position.y;
	float vz = targetPosition.z - position.z;
	float dist = sqrtf(vx * vx + vy * vy + vz * vz);
	// XZ平面での距離を算出
	if (dist > owner->GetAttackRange() + 4.0f && dist < owner->GetAttackRange() + 40.0f)
	{
		// LongRangeNodeへ遷移できる
		return true;
	}
	return false;
}

// TackleNodeに遷移できるか判定
template <typename ActorType>
bool TackleJudgment<ActorType>::Judgment()
{
	// 対象との距離を算出
	DirectX::XMFLOAT3 position = owner->GetPosition();
	DirectX::XMFLOAT3 targetPosition = Player::Instance().GetPosition();
	float vx = targetPosition.x - position.x;
	float vy = targetPosition.y - position.y;
	float vz = targetPosition.z - position.z;
	float dist = sqrtf(vx * vx + vy * vy + vz * vz);
	// XZ平面での距離を算出
	if (dist < owner->GetAttackRange() + 15.0f)
	{
		// TackleNodeへ遷移できる
		return true;
	}
	return false;
}

// WanderNodeに遷移できるか判定
template <typename ActorType>
bool WanderJudgment<ActorType>::Judgment()
{
	// 目的地点までのXZ平面での距離判定
	DirectX::XMFLOAT3 position = owner->GetPosition();
	DirectX::XMFLOAT3 targetPosition = owner->GetTargetPosition();
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float distSq = vx * vx + vz * vz;

	// 目的地から離れている場合
	float radius = owner->GetRadius();
	if (distSq > radius * radius)
	{
		return true;
	}

	return false;
}

template<typename ActorType>
bool AnyDamageJudgment<ActorType>::Judgment()
{
	if (owner->IsAnyDamage())
	{
		return true;
	}
	return false;
}

// DeadNodeに遷移できるか判定
template <typename ActorType>
bool DeadJudgment<ActorType>::Judgment()
{
	// hpが0なら死ぬ
	if (owner->IsDeathFlag())
	{
		return true;
	}
	return false;
}

// 体力が半分以上
template <typename ActorType>
bool FineJudgment<ActorType>::Judgment()
{
	if (owner->GetHealth() > (owner->GetMaxHealth() * 0.5))
	{
		return true;
	}
	return false;
}

// 体力が半分以下
template <typename ActorType>
bool DyingJudgment<ActorType>::Judgment()
{
	if (owner->GetHealth() <= (owner->GetMaxHealth() * 0.5))
	{
		return true;
	}
	return false;
}



// 落下中か判定
template <typename ActorType>
bool FallJudgment<ActorType>::Judgment()
{
	// 地面に接地していない
	if (!owner->IsGround())
	{
		// 地面との距離が 0.5m 以上離れている場合のみ落下とみなす
		if (owner->GetDistanceFromGround() > 0.5f)
		{
			return true;
		}
	}
	return false;
}

template<typename ActorType>
bool RevengeJudgment<ActorType>::Judgment()
{
	if (owner->GetRevengeState())
	{
		return true;
	}
	return false;
}

//-------------------------------------------------------------

template<typename ActorType>
bool SpecialAttackJudgment<ActorType>::Judgment()
{
	if (owner->GetHealth() <= owner->GetMaxHealth() * 0.5 && owner->GetSpecialReady())
	{
		return true;
	}
	return false;
}
