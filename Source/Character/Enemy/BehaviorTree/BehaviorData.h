#pragma once

#include <vector>
#include <stack>
#include <map>
#include "BehaviorTree.h"

// Behavior保存データ
template <typename ActorType>
class BehaviorData
{
public:
	// コンストラクタ
	BehaviorData() { Init(); }
	// シーケンスノードのプッシュ
	void PushSequenceNode(NodeBase<ActorType>* node) { sequenceStack.push(node); }
	// シーケンスノードのポップ
	NodeBase<ActorType>* PopSequenceNode();
	// シーケンスステップのゲッター
	int GetSequenceStep(std::string name);
	// シーケンスステップのセッター
	void SetSequenceStep(std::string name, int step);
	// 初期化
	void Init();
private:
	std::stack<NodeBase<ActorType>*> sequenceStack;		// 実行する中間ノードをスタック
	std::unordered_map<std::string, int> runSequenceStepMap;		// 実行中の中間ノードのステップを記録
};



// シーケンスノードのポップ
template <typename ActorType>
NodeBase<ActorType>* BehaviorData<ActorType>::PopSequenceNode()
{
	// 空ならNULL
	if (sequenceStack.empty() != 0)
	{
		return nullptr;
	}
	// スタックの一番上のデータを取り出す
	NodeBase<ActorType>* node = sequenceStack.top();
	if (node != nullptr)
	{
		// 取り出したデータを削除
		sequenceStack.pop();
	}
	return node;
}

// シーケンスステップのゲッター
template <typename ActorType>
int BehaviorData<ActorType>::GetSequenceStep(std::string name)
{
	// ステップが無ければ追加
	if (runSequenceStepMap.count(name) == 0)
	{
		runSequenceStepMap.insert(std::make_pair(name, 0));
	}

	return runSequenceStepMap.at(name);
}

// シーケンスステップのセッター
template <typename ActorType>
void BehaviorData<ActorType>::SetSequenceStep(std::string name, int step)
{
	runSequenceStepMap.at(name) = step;
}

// 初期化
template <typename ActorType>
void BehaviorData<ActorType>::Init()
{
	// スタックとマップを初期化
	runSequenceStepMap.clear();
	while (sequenceStack.size() > 0)
	{
		sequenceStack.pop();
	}
}
