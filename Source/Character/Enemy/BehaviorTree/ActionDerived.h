#pragma once
#include "ActionBase.h"
#include "Actions/AttackActions.h"
#include "Actions/MoveActions.h"
#include "Actions/DamageActions.h"

// ステートマシン実行用
template <typename ActorType>
class StateMachineAction : public ActionBase<ActorType>
{
public:
	StateMachineAction(ActorType* actor, StateBase<ActorType>* stateMachine)
		: ActionBase<ActorType>(actor), stateMachine(stateMachine) {}

	ActionBase<ActorType>::State Run(float elapsedTime) override {
		if (!entered) {
			stateMachine->Enter();
			entered = true;
		}
		stateMachine->Execute(elapsedTime);
		if (stateMachine->GetState()->IsComplete())
		{
			entered = false;
			return ActionBase<ActorType>::State::Complete;
		}
		if (stateMachine->GetState()->IsFailed())
		{
			entered = false;
			return ActionBase<ActorType>::State::Failed;
		}
		return ActionBase<ActorType>::State::Run;
	}
private:
	StateBase<ActorType>* stateMachine;
	bool entered = false;
};
