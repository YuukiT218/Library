#pragma once

#include <climits>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <string>
#include <vector>

#include "ActionBase.h"
#include "BehaviorTree.h"
#include "JudgementBase.h"

// ノード
template <typename ActorType>
class NodeBase
{
	friend class BehaviorTree<ActorType>;

public:
	NodeBase(
		std::string name,
		NodeBase* parent,
		int priority,
		typename BehaviorTree<ActorType>::SelectRule selectRule,
		JudgmentBase<ActorType>* judgment,
		ActionBase<ActorType>* action)
		: name(std::move(name))
		, selectRule(selectRule)
		, judgment(judgment)
		, action(action)
		, priority(static_cast<unsigned int>(priority))
		, parent(parent)
	{
	}

	~NodeBase() = default;

	std::string GetName() const { return name; }
	int GetPriority() const { return static_cast<int>(priority); }

	typename BehaviorTree<ActorType>::SelectRule GetSelectRule() const { return selectRule; }

	// このノードの元になった BehaviorTreeAsset のノード ID。
	// エディタが「今どのノードが動いているか」を名前ではなく ID で特定するのに使う。
	// （同じ名前のノードが複数あるツリーでも取り違えない）
	int GetSourceId() const { return sourceId; }
	void SetSourceId(int id) { sourceId = id; }

	ActionBase<ActorType>* GetAction() const { return action.get(); }

	void AddChild(std::unique_ptr<NodeBase> child)
	{
		children.emplace_back(std::move(child));
	}

	bool HasAction() const
	{
		return action != nullptr;
	}

	bool Judgment();

	NodeBase* SelectPriority(const std::vector<NodeBase*>* list);
	NodeBase* SelectRandom(const std::vector<NodeBase*>* list);
	NodeBase* SelectSequence(
		const std::vector<NodeBase*>* list,
		BehaviorData<ActorType>* data);

	NodeBase* SearchNode(const std::string& searchName);
	NodeBase* Inference(BehaviorData<ActorType>* data);

	size_t GetChildrenCount() const
	{
		return children.size();
	}

	typename ActionBase<ActorType>::State Run(float elapsedTime);

	std::vector<std::unique_ptr<NodeBase>> children;

protected:
	std::string name;
	typename BehaviorTree<ActorType>::SelectRule selectRule;
	std::unique_ptr<JudgmentBase<ActorType>> judgment;
	std::unique_ptr<ActionBase<ActorType>> action;
	unsigned int priority;
	int lastRandomIndex = -1;
	int sourceId = -1;
	NodeBase* parent;
};

template <typename ActorType>
NodeBase<ActorType>* NodeBase<ActorType>::SearchNode(
	const std::string& searchName)
{
	if (name == searchName)
	{
		return this;
	}

	for (const auto& child : children)
	{
		NodeBase* result = child->SearchNode(searchName);
		if (result != nullptr)
		{
			return result;
		}
	}

	return nullptr;
}

template <typename ActorType>
NodeBase<ActorType>* NodeBase<ActorType>::Inference(
	BehaviorData<ActorType>* data)
{
	std::vector<NodeBase<ActorType>*> candidates;
	NodeBase<ActorType>* result = nullptr;

	for (const auto& child : children)
	{
		if (child->Judgment())
		{
			candidates.emplace_back(child.get());
		}
	}

	switch (selectRule)
	{
	case BehaviorTree<ActorType>::SelectRule::Priority:
		result = SelectPriority(&candidates);
		break;

	case BehaviorTree<ActorType>::SelectRule::Random:
		result = SelectRandom(&candidates);
		break;

	case BehaviorTree<ActorType>::SelectRule::Sequence:
	case BehaviorTree<ActorType>::SelectRule::SequentialLooping:
		result = SelectSequence(&candidates, data);
		break;

	case BehaviorTree<ActorType>::SelectRule::Non:
	default:
		break;
	}

	if (result == nullptr)
	{
		return nullptr;
	}

	if (result->HasAction())
	{
		return result;
	}

	return result->Inference(data);
}

template <typename ActorType>
NodeBase<ActorType>* NodeBase<ActorType>::SelectPriority(
	const std::vector<NodeBase*>* list)
{
	if (list == nullptr || list->empty())
	{
		return nullptr;
	}

	NodeBase* selectedNode = nullptr;
	int lowestPriority = INT_MAX;

	for (NodeBase* node : *list)
	{
		const int currentPriority = node->GetPriority();
		if (currentPriority < lowestPriority)
		{
			lowestPriority = currentPriority;
			selectedNode = node;
		}
	}

	return selectedNode;
}

template <typename ActorType>
NodeBase<ActorType>* NodeBase<ActorType>::SelectRandom(
	const std::vector<NodeBase*>* list)
{
	if (list == nullptr || list->empty())
	{
		return nullptr;
	}

	static bool initialized = false;
	if (!initialized)
	{
		std::srand(static_cast<unsigned int>(std::time(nullptr)));
		initialized = true;
	}

	int selectedIndex = 0;
	if (list->size() > 1)
	{
		do
		{
			selectedIndex = std::rand() % static_cast<int>(list->size());
		} while (selectedIndex == lastRandomIndex);
	}

	lastRandomIndex = selectedIndex;
	return list->at(selectedIndex);
}

template <typename ActorType>
NodeBase<ActorType>* NodeBase<ActorType>::SelectSequence(
	const std::vector<NodeBase*>* list,
	BehaviorData<ActorType>* data)
{
	if (list == nullptr || data == nullptr || children.empty())
	{
		return nullptr;
	}

	int step = data->GetSequenceStep(name);

	if (step >= static_cast<int>(children.size()))
	{
		if (selectRule != BehaviorTree<ActorType>::SelectRule::SequentialLooping)
		{
			return nullptr;
		}

		step = 0;
	}

	NodeBase* nextNode = children.at(step).get();

	for (NodeBase* candidate : *list)
	{
		if (candidate == nextNode)
		{
			data->PushSequenceNode(this);
			data->SetSequenceStep(GetName(), step + 1);
			return nextNode;
		}
	}

	return nullptr;
}

template <typename ActorType>
bool NodeBase<ActorType>::Judgment()
{
	return judgment == nullptr || judgment->Judgment();
}

template <typename ActorType>
typename ActionBase<ActorType>::State NodeBase<ActorType>::Run(
	float elapsedTime)
{
	if (action == nullptr)
	{
		return ActionBase<ActorType>::State::Failed;
	}

	return action->Run(elapsedTime);
}