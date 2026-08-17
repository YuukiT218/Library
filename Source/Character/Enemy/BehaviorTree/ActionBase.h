#pragma once

// 敵がターゲットの方を向く速度の段階
namespace TurnSpeed
{
	// 実質即座に振り向く（攻撃の初動などで向きを合わせる用）
	constexpr float INSTANT = 10000.0f;

	// 素早く振り向く（攻撃中の追従用）
	constexpr float FAST = 1000.0f;

	// ゆっくり振り向く（移動中の緩やかな追従用）
	constexpr float SLOW = 10.0f;
}

template <typename ActorType>
class BehaviorData;

// 行動処理基底クラス
template <typename ActorType>
class ActionBase
{
public:
	ActionBase(ActorType* actor)
		: owner(actor)
		, behaviorData(nullptr)
	{
	}

	virtual ~ActionBase() = default;

	// 実行情報
	enum class State
	{
		Run,		// 実行中
		Failed,		// 実行失敗
		Complete,	// 実行成功
	};

	// 実行処理(純粋仮想関数)
	virtual State Run(float elapsedTime) = 0;

	// BehaviorDataを設定
	void SetBehaviorData(BehaviorData<ActorType>* data) { behaviorData = data; }

protected:
	ActorType* owner;
	BehaviorData<ActorType>* behaviorData;
	int step = 0;
};