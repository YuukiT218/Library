#pragma once

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