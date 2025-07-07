#pragma once

// 実行判定
template <typename ActorType>
class JudgmentBase
{
public:
	JudgmentBase(ActorType* actor) :owner(actor) {}
	virtual bool Judgment() = 0;
protected:
	std::shared_ptr<ActorType> owner;
};