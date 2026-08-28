#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "BehaviorRegistry.h"
#include "BehaviorTree.h"
#include "BehaviorTreeAsset.h"
#include "NodeBase.h"

// ---------------------------------------------------------------------------
// BehaviorTreeAsset から実行時のツリーを組み立てる。
//
// 親子は名前ではなくノード ID で辿るので、同じ名前のノードが並んでいても
// 意図した通りの形になる。
// ---------------------------------------------------------------------------
template <typename ActorType>
class BehaviorTreeBuilder
{
public:
	// Lua 行動の生成はアクターごとに事情が違うので外から渡してもらう。
	// Lua を使わないなら nullptr で構わない。
	using LuaActionFactory =
		std::function<ActionBase<ActorType>* (ActorType*, const BehaviorNodeAsset&)>;

	// アセットの内容でツリーを作り直す。
	// 途中で失敗しても壊れたツリーが残らないよう、まず検証してから組み立てる。
	static bool Build(
		BehaviorTree<ActorType>& tree,
		ActorType* owner,
		const BehaviorTreeAsset& asset,
		const LuaActionFactory& luaActionFactory,
		std::vector<std::string>& problems)
	{
		problems.clear();

		if (!asset.Validate(problems)) return false;

		const BehaviorNodeAsset* root = nullptr;
		for (const BehaviorNodeAsset& node : asset.nodes)
		{
			if (node.parentId < 0) { root = &node; break; }
		}
		if (root == nullptr)
		{
			problems.emplace_back("ルートノードがありません");
			return false;
		}

		tree.Clear();

		// 親から順に作る必要があるので、ルートから幅優先で辿る
		std::unordered_map<int, NodeBase<ActorType>*> created;
		std::vector<const BehaviorNodeAsset*> queue{ root };

		for (size_t head = 0; head < queue.size(); ++head)
		{
			const BehaviorNodeAsset& node = *queue[head];

			NodeBase<ActorType>* parentNode = nullptr;
			if (node.parentId >= 0)
			{
				const auto it = created.find(node.parentId);
				if (it == created.end())
				{
					// Validate を通っていれば起きないが、念のため
					problems.emplace_back("親ノードを先に作れませんでした: " + node.name);
					continue;
				}
				parentNode = it->second;
			}

			NodeBase<ActorType>* added = tree.AddNodeTo(
				parentNode,
				node.name,
				node.priority,
				ToSelectRule(node.selectRule),
				CreateJudgment(node, owner, problems),
				CreateAction(node, owner, luaActionFactory, problems));

			if (added == nullptr)
			{
				problems.emplace_back("ノードを追加できませんでした: " + node.name);
				continue;
			}

			added->SetSourceId(node.id);
			created[node.id] = added;

			for (const BehaviorNodeAsset* child : asset.FindChildren(node.id))
			{
				queue.emplace_back(child);
			}
		}

		return problems.empty();
	}

	// アセットの選択ルールを BehaviorTree 側の列挙へ移す
	static typename BehaviorTree<ActorType>::SelectRule ToSelectRule(BehaviorSelectRule rule)
	{
		using SelectRule = typename BehaviorTree<ActorType>::SelectRule;

		switch (rule)
		{
		case BehaviorSelectRule::Priority:			return SelectRule::Priority;
		case BehaviorSelectRule::Sequence:			return SelectRule::Sequence;
		case BehaviorSelectRule::SequentialLooping:	return SelectRule::SequentialLooping;
		case BehaviorSelectRule::Random:			return SelectRule::Random;
		case BehaviorSelectRule::Non:
		default:									return SelectRule::Non;
		}
	}

	// 逆向きの変換（既存のツリーをアセットへ書き出すときに使う）
	static BehaviorSelectRule FromSelectRule(typename BehaviorTree<ActorType>::SelectRule rule)
	{
		using SelectRule = typename BehaviorTree<ActorType>::SelectRule;

		switch (rule)
		{
		case SelectRule::Priority:			return BehaviorSelectRule::Priority;
		case SelectRule::Sequence:			return BehaviorSelectRule::Sequence;
		case SelectRule::SequentialLooping:	return BehaviorSelectRule::SequentialLooping;
		case SelectRule::Random:			return BehaviorSelectRule::Random;
		case SelectRule::Non:
		default:							return BehaviorSelectRule::Non;
		}
	}

private:
	static JudgmentBase<ActorType>* CreateJudgment(
		const BehaviorNodeAsset& node,
		ActorType* owner,
		std::vector<std::string>& problems)
	{
		if (node.judgment.empty()) return nullptr;

		JudgmentBase<ActorType>* judgment =
			BehaviorRegistry<ActorType>::Instance().CreateJudgment(node.judgment, owner);

		if (judgment == nullptr)
		{
			problems.emplace_back(
				"判定が登録されていません: " + node.judgment + "  (ノード " + node.name + ")");
		}
		return judgment;
	}

	static ActionBase<ActorType>* CreateAction(
		const BehaviorNodeAsset& node,
		ActorType* owner,
		const LuaActionFactory& luaActionFactory,
		std::vector<std::string>& problems)
	{
		switch (node.actionKind)
		{
		case BehaviorActionKind::Native:
		{
			ActionBase<ActorType>* action =
				BehaviorRegistry<ActorType>::Instance().CreateAction(node.actionName, owner);

			if (action == nullptr)
			{
				problems.emplace_back(
					"行動が登録されていません: " + node.actionName + "  (ノード " + node.name + ")");
			}
			return action;
		}

		case BehaviorActionKind::Lua:
		{
			if (!luaActionFactory)
			{
				problems.emplace_back(
					"Lua 行動を作れません（ファクトリ未設定）: " + node.name);
				return nullptr;
			}
			return luaActionFactory(owner, node);
		}

		case BehaviorActionKind::None:
		default:
			return nullptr;
		}
	}
};
