#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ActionBase.h"
#include "JudgementBase.h"

// ---------------------------------------------------------------------------
// 名前から行動・判定クラスを生成するためのレジストリ。
//
// エディタは「どのクラスか」を文字列でしか保存できないので、
// C++ 側で名前と生成関数の対応をここに登録しておく。
// 登録順を覚えているので、エディタのコンボボックスにそのまま並べられる。
// ---------------------------------------------------------------------------
template <typename ActorType>
class BehaviorRegistry
{
public:
	using ActionFactory = std::function<ActionBase<ActorType>* (ActorType*)>;
	using JudgmentFactory = std::function<JudgmentBase<ActorType>* (ActorType*)>;

	static BehaviorRegistry& Instance()
	{
		static BehaviorRegistry instance;
		return instance;
	}

	// 行動クラスを登録する。同じ名前で二度登録した場合は後勝ち。
	void RegisterAction(const std::string& name, ActionFactory factory, const std::string& description = "")
	{
		if (actionFactories.find(name) == actionFactories.end())
		{
			actionNames.emplace_back(name);
		}
		actionFactories[name] = std::move(factory);
		actionDescriptions[name] = description;
	}

	// 判定クラスを登録する
	void RegisterJudgment(const std::string& name, JudgmentFactory factory, const std::string& description = "")
	{
		if (judgmentFactories.find(name) == judgmentFactories.end())
		{
			judgmentNames.emplace_back(name);
		}
		judgmentFactories[name] = std::move(factory);
		judgmentDescriptions[name] = description;
	}

	// 名前から行動を生成する。未登録なら nullptr
	ActionBase<ActorType>* CreateAction(const std::string& name, ActorType* actor) const
	{
		const auto it = actionFactories.find(name);
		if (it == actionFactories.end()) return nullptr;
		return it->second(actor);
	}

	// 名前から判定を生成する。未登録なら nullptr
	JudgmentBase<ActorType>* CreateJudgment(const std::string& name, ActorType* actor) const
	{
		const auto it = judgmentFactories.find(name);
		if (it == judgmentFactories.end()) return nullptr;
		return it->second(actor);
	}

	bool HasAction(const std::string& name) const { return actionFactories.count(name) > 0; }
	bool HasJudgment(const std::string& name) const { return judgmentFactories.count(name) > 0; }

	const std::vector<std::string>& GetActionNames() const { return actionNames; }
	const std::vector<std::string>& GetJudgmentNames() const { return judgmentNames; }

	const std::string& GetActionDescription(const std::string& name) const
	{
		static const std::string empty;
		const auto it = actionDescriptions.find(name);
		return it == actionDescriptions.end() ? empty : it->second;
	}

	const std::string& GetJudgmentDescription(const std::string& name) const
	{
		static const std::string empty;
		const auto it = judgmentDescriptions.find(name);
		return it == judgmentDescriptions.end() ? empty : it->second;
	}

private:
	BehaviorRegistry() = default;

	std::unordered_map<std::string, ActionFactory>		actionFactories;
	std::unordered_map<std::string, JudgmentFactory>	judgmentFactories;
	std::unordered_map<std::string, std::string>		actionDescriptions;
	std::unordered_map<std::string, std::string>		judgmentDescriptions;

	// コンボボックスの並びを安定させるため、登録順も覚えておく
	std::vector<std::string> actionNames;
	std::vector<std::string> judgmentNames;
};

// 登録を 1 行で書くためのヘルパー
#define REGISTER_BEHAVIOR_ACTION(ActorType, ActionClass)                                  \
	BehaviorRegistry<ActorType>::Instance().RegisterAction(                               \
		#ActionClass,                                                                     \
		[](ActorType* actor) -> ActionBase<ActorType>* { return new ActionClass(actor); })

#define REGISTER_BEHAVIOR_JUDGMENT(ActorType, JudgmentClass)                              \
	BehaviorRegistry<ActorType>::Instance().RegisterJudgment(                             \
		#JudgmentClass,                                                                   \
		[](ActorType* actor) -> JudgmentBase<ActorType>* { return new JudgmentClass(actor); })
