#pragma once

#include <vector>
#include <string>
#include "BehaviorTree.h"
#include "ActionBase.h"

// メモリリーク調査用
#define debug_new new(_NORMAL_BLOCK,__FILE__,__LINE__)

// ノード
template <typename ActorType>
class NodeBase
{
	friend class BehaviorTree<ActorType>;
public:
	// コンストラクタ
	NodeBase(std::string name, NodeBase* parent, int priority,
		typename BehaviorTree<ActorType>::SelectRule selectRule, JudgmentBase<ActorType>* judgment, ActionBase<ActorType>* action) :
		name(name), parent(parent), priority(priority),
		selectRule(selectRule), judgment(judgment), action(action), children(NULL)
	{
	}
	// デストラクタ
	~NodeBase();
	// 名前ゲッター
	std::string GetName() { return name; }
	// 優先順位ゲッター
	int GetPriority() { return priority; }
	// 子ノード追加
	void AddChild(NodeBase* child) { children.push_back(child); }
	// 行動データを持っているか
	bool HasAction() { return action != nullptr ? true : false; }
	// 実行可否判定
	bool Judgment();
	// 優先順位選択
	NodeBase* SelectPriority(std::vector<NodeBase*>* list);
	// ランダム選択
	NodeBase* SelectRandom(std::vector<NodeBase*>* list);
	// シーケンス選択
	NodeBase* SelectSequence(std::vector<NodeBase*>* list, BehaviorData<ActorType>* data);
	// ノード検索
	NodeBase* SearchNode(std::string searchName);
	// ノード推論
	NodeBase* Inference(BehaviorData<ActorType>* data);
	// 子ノード数を取得
	size_t GetChildrenCount() const { return children.size(); }
	// 実行
	typename ActionBase<ActorType>::State Run(float elapsedTime);
	std::vector<NodeBase*>		children;							// 子ノード
protected:
	std::string					name;								// 名前
	typename BehaviorTree<ActorType>::SelectRule	selectRule;		// 選択ルール
	JudgmentBase<ActorType>* judgment;								// 判定クラス
	ActionBase<ActorType>* action;									// 実行クラス
	unsigned int				priority;							// 優先順位
	int lastRandomIndex = -1;										// 前回の乱数
	NodeBase* parent;												// 親ノード
};

// デストラクタ
template <typename ActorType>
NodeBase<ActorType>::~NodeBase()
{
}

// ノード検索
template <typename ActorType>
NodeBase<ActorType>* NodeBase<ActorType>::SearchNode(std::string searchName)
{
	// 名前が一致
	if (name == searchName)
	{
		return this;
	}
	else {
		// 子ノードで検索
		for (auto itr = children.begin(); itr != children.end(); itr++)
		{
			NodeBase<ActorType>* ret = (*itr)->SearchNode(searchName);

			if (ret != nullptr)
			{
				return ret;
			}
		}
	}

	return nullptr;
}

// ノード推論
template <typename ActorType>
NodeBase<ActorType>* NodeBase<ActorType>::Inference(BehaviorData<ActorType>* data)
{
	std::vector<NodeBase<ActorType>*> list;
	NodeBase<ActorType>* result = nullptr;

	// childrenの数だけループを行う。
	for (int i = 0; i < children.size(); i++)
	{
		// children.at(i)->judgmentがnullptrでなければ
		if (children.at(i)->judgment != nullptr)
		{
			// children.at(i)->judgment->Judgment()関数を実行し、tureであれば
			// listにchildren.at(i)を追加していく
			if (children.at(i)->judgment->Judgment() == true)
				list.emplace_back(children.at(i));
		}
		else
		{
			// 判定クラスがなければ無条件に追加
			list.emplace_back(children.at(i));
		}
	}

	// 選択ルールでノード決め
	switch (selectRule)
	{
		// 優先順位
	case BehaviorTree<ActorType>::SelectRule::Priority:
		result = SelectPriority(&list);
		break;
		// ランダム
	case BehaviorTree<ActorType>::SelectRule::Random:
		result = SelectRandom(&list);
		break;
		// シーケンス
	case BehaviorTree<ActorType>::SelectRule::Sequence:
	case BehaviorTree<ActorType>::SelectRule::SequentialLooping:
		result = SelectSequence(&list, data);
		break;
	}

	if (result != nullptr)
	{
		// 行動があれば終了
		if (result->HasAction() == true)
		{
			return result;
		}
		else
		{
			// 決まったノードで推論開始
			result = result->Inference(data);
		}
	}

	return result;
}

// 優先順位でノード選択
template <typename ActorType>
NodeBase<ActorType>* NodeBase<ActorType>::SelectPriority(std::vector<NodeBase*>* list)
{
	// 優先順位が高いノードを格納するポインタ
	NodeBase<ActorType>* selectNode = nullptr;
	int priority = INT_MAX;

	// 一番優先順位が高いノードを探してselectNodeに格納
	// リスト内のノードをループ
	for (auto node : *list)
	{
		// 現在のノードの優先順位を取得
		int currentPriority = node->GetPriority();

		// より優先順位が高い（数値が小さい）場合は更新
		if (currentPriority < priority)
		{
			priority = currentPriority;		// 優先順位を更新
			selectNode = node;              // ノードを選択
		}
	}

	return selectNode;
}


// ランダムでノード選択
template <typename ActorType>
NodeBase<ActorType>* NodeBase<ActorType>::SelectRandom(std::vector<NodeBase*>* list)
{
	static bool initialized = false;
	if (!initialized) {
		srand(static_cast<unsigned int>(time(nullptr)));
		initialized = true;
	}
	int selectNo = 0;
	// listのサイズで乱数を取得してselectNoに格納
	if (list->size() <= 1) {
		selectNo = 0;
	}
	else {
		do {
			selectNo = rand() % list->size();
		} while (selectNo == lastRandomIndex);
	}
	lastRandomIndex = selectNo;

	// listのselectNo番目の実態をリターン
	return (*list).at(selectNo);
}

// シーケンス・シーケンシャルルーピングでノード選択
template <typename ActorType>
NodeBase<ActorType>* NodeBase<ActorType>::SelectSequence(std::vector<NodeBase*>* list, BehaviorData<ActorType>* data)
{
	int step = 0;

	// 指定されている中間ノードのシーケンスがどこまで実行されたか取得する
	step = data->GetSequenceStep(name);

	// 中間ノードに登録されているノード数以上の場合、
	if (step >= children.size())
	{
		// ルールによって処理を切り替える
		// ルールがBehaviorTree::SelectRule::SequentialLoopingのときは最初から実行するため、stepに0を代入
		// ルールがBehaviorTree::SelectRule::Sequenceのときは次に実行できるノードがないため、nullptrをリターン
		if (selectRule != BehaviorTree<ActorType>::SelectRule::SequentialLooping)
		{
			return nullptr;
		}
		else
		{
			step = 0;
		}
	}
	// 実行可能リストに登録されているデータの数だけループを行う
	for (auto itr = list->begin(); itr != list->end(); itr++)
	{
		// 子ノードが実行可能リストに含まれているか
		if (children.at(step)->GetName() == (*itr)->GetName())
		{
			//シーケンスノードを記録
			data->PushSequenceNode(this);
			//シーケンスステップを更新
			data->SetSequenceStep(GetName(), step + 1);
			return children.at(step);
		}
	}
	// 指定された中間ノードに実行可能ノードがないのでnullptrをリターンする
	return nullptr;
}

// 判定
template <typename ActorType>
bool NodeBase<ActorType>::Judgment()
{
	// judgmentがあるか判断。あればメンバ関数Judgment()実行した結果をリターン。
	if (judgment != nullptr)
	{
		return judgment->Judgment();
	}

	return true;
}

// ノード実行
template <typename ActorType>
typename ActionBase<ActorType>::State NodeBase<ActorType>::Run(float elapsedTime)
{
	// actionがあるか判断。あればメンバ関数Run()実行した結果をリターン。
	if (action != nullptr)
	{
		return action->Run(elapsedTime);
	}

	return ActionBase<ActorType>::State::Failed;
}
