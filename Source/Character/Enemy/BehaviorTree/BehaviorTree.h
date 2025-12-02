#pragma once
#include <string>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

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

	// ノード追加:
	void AddNode(std::string parentName, std::string entryName, int priority, SelectRule selectRule, JudgmentBase<ActorType>* judgment, ActionBase<ActorType>* action, StateBase<ActorType>* stateMachine = nullptr);

	// 実行
	NodeBase<ActorType>* Run(NodeBase<ActorType>* actionNode, BehaviorData<ActorType>* data, float elapsedTime);
private:
	// ノード全削除
	void NodeAllClear(NodeBase<ActorType>* delNode);
private:
	// ルートノード
	NodeBase<ActorType>* root;
	ActorType* owner;
};

// デストラクタ
template <typename ActorType>
BehaviorTree<ActorType>::~BehaviorTree()
{
	NodeAllClear(root);
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
			root = new NodeBase<ActorType>(entryName, nullptr, priority, selectRule, judgment, action);
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
	// BehaviorDataをActionに設定
	if (actionNode->action != nullptr)
	{
		actionNode->action->SetBehaviorData(data);
	}

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
	if (delNode == nullptr) return;

	if (delNode->judgment)
	{
		delete delNode->judgment;
	}
	if (delNode->action)
	{
		delete delNode->action;
	}

	// 子ノードを先に削除
	for (NodeBase<ActorType>* node : delNode->children)
	{
		NodeAllClear(node);
	}

	// 自分自身を削除（NodeBaseのデストラクタが呼ばれる）
	delete delNode;
}


