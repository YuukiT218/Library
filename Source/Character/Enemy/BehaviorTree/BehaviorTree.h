#pragma once
#include <string>

template <typename ActorType>
class ActionBase;
template <typename ActorType>
class JudgmentBase;
template <typename ActorType>
class BehaviorData;
template <typename ActorType>
class NodeBase;

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

public:
	BehaviorTree() :root(nullptr), owner(nullptr) {}
	BehaviorTree(ActorType* actor) :root(nullptr), owner(actor) {}
	~BehaviorTree();

	// 実行ノードを推論する
	NodeBase<ActorType>* ActiveNodeInference(BehaviorData<ActorType>* data);

	// シーケンスノードから推論開始
	NodeBase<ActorType>* SequenceBack(NodeBase<ActorType>* sequenceNode, BehaviorData<ActorType>* data);

	// ノード追加
	void AddNode(std::string parentName, std::string entryName, int priority, SelectRule selectRule, JudgmentBase<ActorType>* judgment, ActionBase<ActorType>* action, StateBase<ActorType>* stateMachine = nullptr);

	// 実行
	NodeBase<ActorType>* Run(NodeBase<ActorType>* actionNode, BehaviorData<ActorType>* data, float elapsedTime);
private:
	// ノード全削除
	void NodeAllClear(NodeBase<ActorType>* delNode);
private:
	// ルートノード
	std::unique_ptr<NodeBase<ActorType>> root;
	std::shared_ptr<ActorType> owner;
};

// デストラクタ
template <typename ActorType>
BehaviorTree<ActorType>::~BehaviorTree()
{
	//NodeAllClear(root);
}

template <typename ActorType>
void BehaviorTree<ActorType>::AddNode(std::string parentName, std::string entryName, int priority, SelectRule selectRule, JudgmentBase<ActorType>* judgment, ActionBase<ActorType>* action, StateBase<ActorType>* stateMachine)
{
	// ルートノードが無ければ
	if (parentName != "")
	{
		// 親ノードを検索
		NodeBase<ActorType>* parentNode = root->SearchNode(parentName);

		// 親ノードが見つかれば
		if (parentNode != nullptr)
		{
			// ノードを追加
			NodeBase<ActorType>* addNode = new NodeBase<ActorType>(entryName, parentNode, priority, selectRule, judgment, action);
			parentNode->AddChild(addNode);
		}
	}

	else
	{
		if (root == nullptr)
		{
			// ルートノードを追加
			root = std::make_unique<NodeBase<ActorType>>(entryName, nullptr, priority, selectRule, judgment, action);
		}
	}
}

// 次に実行するノードを推論
template <typename ActorType>
NodeBase<ActorType>* BehaviorTree<ActorType>::ActiveNodeInference(BehaviorData<ActorType>* data)
{
	// データをリセットして開始
	data->Init();
	return root->Inference(data);
}

// シーケンスノードからの推論開始
template <typename ActorType>
NodeBase<ActorType>* BehaviorTree<ActorType>::SequenceBack(NodeBase<ActorType>* sequenceNode, BehaviorData<ActorType>* data)
{
	return sequenceNode->Inference(data);
}

// ノード実行
template <typename ActorType>
NodeBase<ActorType>* BehaviorTree<ActorType>::Run(NodeBase<ActorType>* actionNode, BehaviorData<ActorType>* data, float elapsedTime)
{
	// ノード実行
	ActionBase<ActorType>::State state = actionNode->Run(elapsedTime);

	// 正常終了
	if (state == ActionBase<ActorType>::State::Complete)
	{
		// シーケンスの途中かを判断
		NodeBase<ActorType>* sequenceNode = data->PopSequenceNode();

		// 途中じゃないなら終了
		if (sequenceNode == nullptr)
		{
			return nullptr;
		}
		else
		{
			// 途中ならそこから始める
			return SequenceBack(sequenceNode, data);
		}
	}
	// 失敗
	else if (state == ActionBase<ActorType>::State::Failed)
	{
		// 失敗は終了
		return nullptr;
	}

	// 現状維持
	return actionNode;
}

// 登録されたノードを全て削除する
template <typename ActorType>
void BehaviorTree<ActorType>::NodeAllClear(NodeBase<ActorType>* delNode)
{
	// 子ノードの数を取得
	size_t count = delNode->children.size();
	if (count > 0)
	{
		// 子ノードの数だけループ
		for (NodeBase<ActorType>* node : delNode->children)
		{
			// 再帰的に削除
			NodeAllClear(node);
		}
		delete delNode;
	}
	// 子ノードが無ければ
	else
	{
		delete delNode;
	}
}


