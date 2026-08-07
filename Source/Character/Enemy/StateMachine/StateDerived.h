#pragma once
#include "StateBase.h"

//-------------------------------------------------------------------
// 基底ステートオブジェクト
template <typename ActorType>
class RootState : public StateBase<ActorType>
{
public:
	// コンストラクタ
	RootState(ActorType* actor) :StateBase<ActorType>(actor) {};
	// デストラクタ
	~RootState() {}
	// ステートに入った時のメソッド
	void Enter() override;
	// ステートで実行するメソッド
	void Execute(float elapsedTime) override;

	const char* GetName() const { return "Wander"; }
};

// ルートステートに入った時のメソッド
template <typename ActorType>
void RootState<ActorType>::Enter()
{
	// 初期ステート(Search)を指定
	owner->GetState()->ChangeState("RandomState");
}

// ルートステートで実行するメソッド
template <typename ActorType>
void RootState<ActorType>::Execute(float elapsedTime)
{
	if (currentState != nullptr)
	{
		currentState->Execute(elapsedTime);
	}
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// 徘徊ステートオブジェクト
template <typename ActorType>
class WanderState : public StateBase<ActorType>
{
public:
	// コンストラクタ
	WanderState(ActorType* actor) :StateBase<ActorType>(actor) {};
	// デストラクタ
	~WanderState() {}
	// ステートに入った時のメソッド
	void Enter() override;
	// ステートで実行するメソッド
	void Execute(float elapsedTime) override;
	bool isComplete = false;
	bool IsComplete() const override { return isComplete; }

	const char* GetName() const { return "Wander"; }
};

// 徘徊ステートに入った時のメソッド
template<typename ActorType>
void WanderState<ActorType>::Enter()
{
	owner->SetRandomTargetPosition();
	owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::WalkFWD), true);
}

// 徘徊ステートで実行するメソッド
template<typename ActorType>
void WanderState<ActorType>::Execute(float elapsedTime)
{
	// 目的地点までのXZ平面での距離判定
	DirectX::XMFLOAT3 position = owner->GetPosition();
	DirectX::XMFLOAT3 targetPosition = owner->GetTargetPosition();
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float distSq = vx * vx + vz * vz;

	owner->MoveToTarget(elapsedTime, 0.5f);

	// 目的地へ着いた
	float radius = owner->GetRadius();
	if (distSq < radius * radius)
	{
		// parent->ChangeStateでStateを切り替える
		// Idleステートへ遷移
		parentState->ChangeState("Idle");
		isComplete = true;
	}
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// 待機ステートオブジェクト
template <typename ActorType>
class IdleState : public StateBase<ActorType>
{
public:
	// コンストラクタ
	IdleState(ActorType* actor) :StateBase<ActorType>(actor) {};
	// デストラクタ
	~IdleState() {}
	// ステートに入った時のメソッド
	void Enter() override;
	// ステートで実行するメソッド
	void Execute(float elapsedTime) override;
	bool isComplete = false;
	bool IsComplete() const override { return isComplete; }

	const char* GetName() const { return "Idle"; }
};

// 待機ステートに入った時のメソッド
template <typename ActorType>
void IdleState<ActorType>::Enter()
{
	owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::IdleNormal), true);
	owner->SetStateTimer(Mathf::RandomRange(3.0f, 5.0f));
}

// 待機ステートで実行するメソッド
template <typename ActorType>
void IdleState<ActorType>::Execute(float elapsedTime)
{
	float timer = owner->GetStateTimer();
	timer -= elapsedTime;
	owner->SetStateTimer(timer);
	if (timer < 0.0f)
	{
		// parent->ChangeStateでStateを切り替える
		// Wanderステートに遷移
		parentState->ChangeState("Wander");
		isComplete = true;
	}
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// 追跡ステートオブジェクト
template <typename ActorType>
class PursuitState : public StateBase<ActorType>
{
public:
	// コンストラクタ
	PursuitState(ActorType* actor) :StateBase<ActorType>(actor) {};
	// デストラクタ
	~PursuitState() {}
	// ステートに入った時のメソッド
	void Enter() override;
	// ステートで実行するメソッド
	void Execute(float elapsedTime) override;
	bool isComplete = false;
	bool IsComplete() const override { return isComplete; }

	const char* GetName() const { return "Pursuit"; }
};

// 追跡ステートに入った時のメソッド
template <typename ActorType>
void PursuitState<ActorType>::Enter()
{
	owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::RunFWD), true);
	owner->SetStateTimer(Mathf::RandomRange(3.0f, 5.0f));
}

// 追跡ステートで実行するメソッド
template <typename ActorType>
void PursuitState<ActorType>::Execute(float elapsedTime)
{
	// 目標地点をプレイヤー位置に設定
	owner->SetTargetPosition(Player::Instance().GetPosition());

	// 目的地点へ移動
	owner->MoveToTarget(elapsedTime, 1.0);

	float vx = owner->GetTargetPosition().x - owner->GetPosition().x;
	float vy = owner->GetTargetPosition().y - owner->GetPosition().y;
	float vz = owner->GetTargetPosition().z - owner->GetPosition().z;
	float dist = sqrtf(vx * vx + vy * vy + vz * vz);
	if (dist < owner->GetAttackRange())
	{
		// Attackステートに遷移
		//乱数で攻撃パターン変化
		switch (rand() % 2)
		{
		case 0:
			parentState->ChangeState("Attack");
			isComplete = true;
			break;
		case 1:
			parentState->ChangeState("Skill");
			isComplete = true;
			break;
		}
	}
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// 通常攻撃ステートオブジェクト
template <typename ActorType>
class AttackState : public StateBase<ActorType>
{
public:
	// コンストラクタ
	AttackState(ActorType* actor) :StateBase<ActorType>(actor) {};
	// デストラクタ
	~AttackState() {}
	// ステートに入った時のメソッド
	void Enter() override;
	// ステートで実行するメソッド
	void Execute(float elapsedTime) override;
	bool isComplete = false;
	bool IsComplete() const override { return isComplete; }

	const char* GetName() const { return "Attack"; }
};

// 通常攻撃ステートに入った時のメソッド
template <typename ActorType>
void AttackState<ActorType>::Enter()
{
	isComplete = false;
	owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::Attack01), false);
}

// 通常攻撃ステートで実行するメソッド
template <typename ActorType>
void AttackState<ActorType>::Execute(float elapsedTime)
{
	if (owner->GetModel()->IsPlayAnimation())
	{
		owner->SetTargetPosition(Player::Instance().GetPosition());
		float vx = owner->GetTargetPosition().x - owner->GetPosition().x;
		float vy = owner->GetTargetPosition().y - owner->GetPosition().y;
		float vz = owner->GetTargetPosition().z - owner->GetPosition().z;
	}
	if (!owner->GetModel()->IsPlayAnimation())
	{
		// Pursuitステートに遷移
		isComplete = true;
	}
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// スキルステートオブジェクト
template <typename ActorType>
class SkillState : public StateBase<ActorType>
{
public:
	//コンストラクタ
	SkillState(ActorType* actor) :StateBase<ActorType>(actor) {};
	//デストラクタ
	~SkillState() {}
	//ステートに入った時のメソッド
	void Enter() override;
	//ステートで実行するメソッド
	void Execute(float elapsedTime) override;
	bool isComplete = false;
	bool IsComplete() const override { return isComplete; }

	const char* GetName() const { return "Skill"; }
};

// スキルステートに入った時のメソッド
template <typename ActorType>
void SkillState<ActorType>::Enter()
{
	isComplete = false;
	owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::Attack02), false);
}

// スキルステートで実行するメソッド
template <typename ActorType>
void SkillState<ActorType>::Execute(float elapsedTime)
{
	if (owner->GetModel()->IsPlayAnimation())
	{
		owner->SetTargetPosition(Player::Instance().GetPosition());
		float vx = owner->GetTargetPosition().x - owner->GetPosition().x;
		float vy = owner->GetTargetPosition().y - owner->GetPosition().y;
		float vz = owner->GetTargetPosition().z - owner->GetPosition().z;
	}
	if (!owner->GetModel()->IsPlayAnimation())
	{
		// Pursuitステートに遷移
		isComplete = true;
	}
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// スキルステートオブジェクト
template <typename ActorType>
class Skill1State : public StateBase<ActorType>
{
public:
	//コンストラクタ
	Skill1State(ActorType* actor) :StateBase<ActorType>(actor) {};
	//デストラクタ
	~Skill1State() {}
	//ステートに入った時のメソッド
	void Enter() override;
	//ステートで実行するメソッド
	void Execute(float elapsedTime) override;
	bool isComplete = false;
	bool IsComplete() const override { return isComplete; }

	const char* GetName() const { return "Skill"; }
};

// スキルステートに入った時のメソッド
template <typename ActorType>
void Skill1State<ActorType>::Enter()
{
	isComplete = false;
	owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::SpinAttack), false);
}

// スキルステートで実行するメソッド
template <typename ActorType>
void Skill1State<ActorType>::Execute(float elapsedTime)
{
	if (owner->GetModel()->IsPlayAnimation())
	{
		owner->SetTargetPosition(Player::Instance().GetPosition());
		float vx = owner->GetTargetPosition().x - owner->GetPosition().x;
		float vy = owner->GetTargetPosition().y - owner->GetPosition().y;
		float vz = owner->GetTargetPosition().z - owner->GetPosition().z;
	}
	if (!owner->GetModel()->IsPlayAnimation())
	{
		// Pursuitステートに遷移
		isComplete = true;
	}
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
template <typename ActorType>
class SearchState : public StateBase<ActorType>
{
public:
	// コンストラクタ
	SearchState(ActorType* actor) :StateBase<ActorType>(actor) {};
	// デストラクタ
	~SearchState() {}
	// ステートに入った時のメソッド
	void Enter() override;
	// ステートで実行するメソッド
	void Execute(float elapsedTime) override;
	bool isComplete = false;
	bool IsComplete() const override { return isComplete; }

	const char* GetName() const { return "Search"; }
};

// 索敵ステート
template <typename ActorType>
void SearchState<ActorType>::Enter()
{
	// 初期ステート(Idle)を指定
	ChangeState("Idle");
}

template <typename ActorType>
void SearchState<ActorType>::Execute(float elapsedTime)
{
	// サブステートを実行
	if (currentState != nullptr)
	{
		currentState->Execute(elapsedTime);
	}

	// プレイヤー索敵
	if (owner->SearchPlayer())
	{
		// Battleステートに遷移
		owner->GetState()->ChangeState("Battle");
		isComplete = true;
	}
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// 戦闘ステート
template <typename ActorType>
class BattleState : public StateBase<ActorType>
{
public:
	// コンストラクタ
	BattleState(ActorType* actor) :StateBase<ActorType>(actor) {};
	// デストラクタ
	~BattleState() {}
	// ステートに入った時のメソッド
	void Enter() override;
	// ステートで実行するメソッド
	void Execute(float elapsedTime) override;
	bool isComplete = false;
	bool IsComplete() const override { return isComplete; }

	const char* GetName() const { return "Battle"; }
};

template <typename ActorType>
void BattleState<ActorType>::Enter()
{
	// 初期ステート(Pursuit)を指定
	ChangeState("Pursuit");
}

template <typename ActorType>
void BattleState<ActorType>::Execute(float elapsedTime)
{
	// サブステートを実行
	if (currentState != nullptr)
	{
		currentState->Execute(elapsedTime);
	}
	// 他ステートへの遷移
	// タイマー処理
	float timer = owner->GetStateTimer();
	timer -= elapsedTime;
	owner->SetStateTimer(timer);
	if (timer < 0.0f)
	{
		// Searchステートへ遷移
		owner->GetState()->ChangeState("Search");
		isComplete = true;
	}
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// 被弾ステート
// 現状未使用
template <typename ActorType>
class GetHitState : public StateBase<ActorType>
{
public:
	// コンストラクタ
	GetHitState(ActorType* actor) :StateBase<ActorType>(actor) {};
	// デストラクタ
	~GetHitState() {}
	// ステートに入った時のメソッド
	void Enter() override;
	// ステートで実行するメソッド
	void Execute(float elapsedTime) override;
	bool isComplete = false;
	bool IsComplete() const override { return isComplete; }

	const char* GetName() const { return "GetHit"; }
};

template <typename ActorType>
void GetHitState<ActorType>::Enter()
{
	if (!owner->IsDamage())
	{
		isComplete = true;
	}

	owner->GetModel()->PlayAnimation(static_cast<int>(ActorType::EnemyAnimation::GetHit), false);
}

template <typename ActorType>
void GetHitState<ActorType>::Execute(float elapsedTime)
{
	// 被弾モーションが終わっていれば追跡へ遷移
	if (!owner->GetModel()->IsPlayAnimation())
	{
		// Pursuitステートに遷移
		owner->SetDamage(false);
		isComplete = true;
	}
}
//-------------------------------------------------------------------