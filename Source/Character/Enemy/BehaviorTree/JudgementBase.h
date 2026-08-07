#pragma once

// 実行判定
template <typename ActorType>
class JudgmentBase
{
public:
	JudgmentBase(ActorType* actor)
		: owner(actor)
	{
	}

	virtual ~JudgmentBase() = default;

	virtual bool Judgment() = 0;

protected:
	ActorType* owner;
};