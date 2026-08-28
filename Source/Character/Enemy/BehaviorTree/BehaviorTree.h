#pragma once

#include <memory>
#include <string>

template <typename ActorType>
class ActionBase;

template <typename ActorType>
class JudgmentBase;

template <typename ActorType>
class BehaviorData;

template <typename ActorType>
class NodeBase;

template <typename ActorType>
class StateBase;

// ビヘイビアツリー
template <typename ActorType>
class BehaviorTree
{
public:
	// 選択ルール
	enum class SelectRule
	{
		Non,				// 無い末端ノード用
		Priority,			// 優先順位
		Sequence,			// シーケンス
		SequentialLooping,	// シーケンシャルルーピング
		Random,				// ランダム
	};

	BehaviorTree()
		: root(nullptr)
		, owner(nullptr)
	{
	}

	explicit BehaviorTree(ActorType* actor)
		: root(nullptr)
		, owner(actor)
	{
	}

	~BehaviorTree();

	// 実行ノードを推論する
	NodeBase<ActorType>* ActiveNodeInference(BehaviorData<ActorType>* data);

	// シーケンスノードから推論開始
	NodeBase<ActorType>* SequenceBack(
		NodeBase<ActorType>* sequenceNode,
		BehaviorData<ActorType>* data);

	// ノード追加
	void AddNode(
		const std::string& parentName,
		const std::string& entryName,
		int priority,
		SelectRule selectRule,
		JudgmentBase<ActorType>* judgment,
		ActionBase<ActorType>* action,
		StateBase<ActorType>* stateMachine = nullptr);

	// ノード追加（親をポインタで直接指定する版）
	//
	// 名前引きの AddNode は同名ノードがあると最初に見つかった方に繋がってしまうため、
	// エディタから組み立てるときはこちらを使う。追加したノードを返す。
	NodeBase<ActorType>* AddNodeTo(
		NodeBase<ActorType>* parentNode,
		const std::string& entryName,
		int priority,
		SelectRule selectRule,
		JudgmentBase<ActorType>* judgment,
		ActionBase<ActorType>* action);

	// 実行
	NodeBase<ActorType>* Run(
		NodeBase<ActorType>* actionNode,
		BehaviorData<ActorType>* data,
		float elapsedTime);

	// ルートノード取得
	NodeBase<ActorType>* GetRoot() const { return root; }

	// ツリーを空にする（組み直しの前に呼ぶ）
	void Clear();

	// 所有者取得
	ActorType* GetOwner() const { return owner; }

private:
	// ルートノードを削除する
	void NodeAllClear(NodeBase<ActorType>* node);

private:
	NodeBase<ActorType>* root;
	ActorType* owner;
};

template <typename ActorType>
BehaviorTree<ActorType>::~BehaviorTree()
{
	NodeAllClear(root);
	root = nullptr;
}

template <typename ActorType>
void BehaviorTree<ActorType>::AddNode(
	const std::string& parentName,
	const std::string& entryName,
	int priority,
	SelectRule selectRule,
	JudgmentBase<ActorType>* judgment,
	ActionBase<ActorType>* action,
	StateBase<ActorType>* stateMachine)
{
	(void)stateMachine;

	// 追加できなかった場合も確実に解放されるように一時所有する。
	std::unique_ptr<JudgmentBase<ActorType>> judgmentOwner(judgment);
	std::unique_ptr<ActionBase<ActorType>> actionOwner(action);

	if (parentName.empty())
	{
		if (root == nullptr)
		{
			root = new NodeBase<ActorType>(
				entryName,
				nullptr,
				priority,
				selectRule,
				judgmentOwner.release(),
				actionOwner.release());
		}

		return;
	}

	if (root == nullptr)
	{
		return;
	}

	NodeBase<ActorType>* parentNode = root->SearchNode(parentName);
	if (parentNode == nullptr)
	{
		return;
	}

	auto addNode = std::make_unique<NodeBase<ActorType>>(
		entryName,
		parentNode,
		priority,
		selectRule,
		judgmentOwner.release(),
		actionOwner.release());

	parentNode->AddChild(std::move(addNode));
}

template <typename ActorType>
NodeBase<ActorType>* BehaviorTree<ActorType>::AddNodeTo(
	NodeBase<ActorType>* parentNode,
	const std::string& entryName,
	int priority,
	SelectRule selectRule,
	JudgmentBase<ActorType>* judgment,
	ActionBase<ActorType>* action)
{
	// 追加できなかった場合も確実に解放されるように一時所有する。
	std::unique_ptr<JudgmentBase<ActorType>> judgmentOwner(judgment);
	std::unique_ptr<ActionBase<ActorType>> actionOwner(action);

	if (parentNode == nullptr)
	{
		// 親がいない = ルート。すでにルートがあるなら追加しない。
		if (root != nullptr) return nullptr;

		root = new NodeBase<ActorType>(
			entryName,
			nullptr,
			priority,
			selectRule,
			judgmentOwner.release(),
			actionOwner.release());

		return root;
	}

	auto addNode = std::make_unique<NodeBase<ActorType>>(
		entryName,
		parentNode,
		priority,
		selectRule,
		judgmentOwner.release(),
		actionOwner.release());

	NodeBase<ActorType>* added = addNode.get();
	parentNode->AddChild(std::move(addNode));
	return added;
}

template <typename ActorType>
void BehaviorTree<ActorType>::Clear()
{
	NodeAllClear(root);
	root = nullptr;
}

template <typename ActorType>
NodeBase<ActorType>* BehaviorTree<ActorType>::ActiveNodeInference(
	BehaviorData<ActorType>* data)
{
	if (root == nullptr || data == nullptr)
	{
		return nullptr;
	}

	data->Init();
	return root->Inference(data);
}

template <typename ActorType>
NodeBase<ActorType>* BehaviorTree<ActorType>::SequenceBack(
	NodeBase<ActorType>* sequenceNode,
	BehaviorData<ActorType>* data)
{
	if (sequenceNode == nullptr || data == nullptr)
	{
		return nullptr;
	}

	return sequenceNode->Inference(data);
}

template <typename ActorType>
NodeBase<ActorType>* BehaviorTree<ActorType>::Run(
	NodeBase<ActorType>* actionNode,
	BehaviorData<ActorType>* data,
	float elapsedTime)
{
	if (actionNode == nullptr || data == nullptr)
	{
		return nullptr;
	}

	if (actionNode->action != nullptr)
	{
		actionNode->action->SetBehaviorData(data);
	}

	ActionBase<ActorType>::State state = actionNode->Run(elapsedTime);

	if (state == ActionBase<ActorType>::State::Complete)
	{
		NodeBase<ActorType>* sequenceNode = data->PopSequenceNode();
		return sequenceNode == nullptr ? nullptr : SequenceBack(sequenceNode, data);
	}

	if (state == ActionBase<ActorType>::State::Failed)
	{
		return nullptr;
	}

	return actionNode;
}

template <typename ActorType>
void BehaviorTree<ActorType>::NodeAllClear(NodeBase<ActorType>* node)
{
	// NodeBase が子ノード・判定・行動を unique_ptr で保持する。
	delete node;
}