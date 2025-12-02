#pragma once

#include <vector>
#include <stack>
#include <map>
#include "BehaviorTree.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

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
	// シーケンス実行中かどうか判定
	bool IsInSequence() const { return !sequenceStack.empty(); }
	// 現在のシーケンスノード名を取得
	std::string GetCurrentSequenceName() const;
	// 特定のシーケンス中かどうか判定
	bool IsInSequence(std::string sequenceName) const;
	// 現在のシーケンスステップを取得
	int GetCurrentSequenceStep() const;
	// シーケンスの最後のノードかどうか判定
	bool IsLastNodeInSequence() const;
	// シーケンス中で、かつ最後ではないか判定
	bool IsInSequenceAndNotLast() const;
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

// 現在のシーケンスノード名を取得
template <typename ActorType>
std::string BehaviorData<ActorType>::GetCurrentSequenceName() const
{
	if (sequenceStack.empty()) return "";
	return sequenceStack.top()->GetName();
}

// 特定のシーケンス中かどうか判定
template <typename ActorType>
bool BehaviorData<ActorType>::IsInSequence(std::string sequenceName) const
{
	if (sequenceStack.empty()) return false;
	return sequenceStack.top()->GetName() == sequenceName;
}

// 現在のシーケンスステップを取得
template <typename ActorType>
int BehaviorData<ActorType>::GetCurrentSequenceStep() const
{
	if (sequenceStack.empty()) return -1;
	std::string name = sequenceStack.top()->GetName();
	auto it = runSequenceStepMap.find(name);
	if (it != runSequenceStepMap.end())
		return it->second;
	return 0;
}

// シーケンスの最後のノードかどうか判定
template <typename ActorType>
bool BehaviorData<ActorType>::IsLastNodeInSequence() const
{
	if (sequenceStack.empty()) return false;

	NodeBase<ActorType>* sequenceNode = sequenceStack.top();
	std::string sequenceName = sequenceNode->GetName();

	// 現在のステップを取得
	auto it = runSequenceStepMap.find(sequenceName);
	if (it == runSequenceStepMap.end()) return false;

	int currentStep = it->second;
	int childrenCount = sequenceNode->children.size();

	// 次のステップが子ノードの数以上なら最後
	return currentStep >= childrenCount;
}

// シーケンス中で、かつ最後ではないか判定
template <typename ActorType>
bool BehaviorData<ActorType>::IsInSequenceAndNotLast() const
{
	return IsInSequence() && !IsLastNodeInSequence();
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
