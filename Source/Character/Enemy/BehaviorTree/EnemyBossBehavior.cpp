#include "EnemyBossBehavior.h"

#include "ActionDerived.h"
#include "BehaviorRegistry.h"
#include "BehaviorTree.h"
#include "BehaviorTreeBuilder.h"
#include "JudgementDerived.h"
#include "NodeBase.h"

#include "Character/Enemy/EnemyBoss.h"
#include "Script/LuaAction.h"
#include "Script/LuaBossBindings.h"
#include "Script/LuaScriptSystem.h"

namespace EnemyBossBehavior
{
	void RegisterBehaviors()
	{
		static bool registered = false;
		if (registered) return;
		registered = true;

		// ---- 行動 ----
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, SlashCombo1Action);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, SlashWave);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, DashSlashAction);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, NormalTeleport);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, TeleportCombo);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, TelePortAssault);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, TripleTeleportAction);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, PillarSpiralConv);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, PillarSpiralDiff);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, RevengeDive);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, RevengeAssault);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, SpecialAttack);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, PursuitAction);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, IdleAction);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, WanderAction);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, CautiousWalkAction);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, FallAction);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, UnifiedDamageAction);
		REGISTER_BEHAVIOR_ACTION(EnemyBoss, DeadAction);

		// ---- 判定 ----
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, BattleJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, AttackJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, LongRangeJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, RevengeJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, SpecialAttackJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, AnyDamageJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, DeadJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, DyingJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, FineJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, FallJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, TackleJudgment);
		REGISTER_BEHAVIOR_JUDGMENT(EnemyBoss, WanderJudgment);

		// RoarJudgment は EnemyBoss に無いメンバー（RoarUsedFlag）を参照しているため、
		// このボスでは実体化できない。使えるようになったらここに追加する。
	}

	bool BuildTree(
		BehaviorTree<EnemyBoss>& tree,
		EnemyBoss* owner,
		const BehaviorTreeAsset& asset,
		std::vector<std::string>& problems)
	{
		RegisterBehaviors();

		const auto luaFactory =
			[](EnemyBoss* actor, const BehaviorNodeAsset& node) -> ActionBase<EnemyBoss>*
		{
			return new LuaAction(actor, node);
		};

		return BehaviorTreeBuilder<EnemyBoss>::Build(tree, owner, asset, luaFactory, problems);
	}

	void PreloadScripts(const BehaviorTreeAsset& asset)
	{
		LuaBossBindings::EnsureRegistered();

		LuaScriptSystem& lua = LuaScriptSystem::Instance();
		if (!lua.IsReady()) return;

		for (const BehaviorNodeAsset& node : asset.nodes)
		{
			if (node.actionKind != BehaviorActionKind::Lua) continue;
			if (node.actionScript.empty()) continue;

			lua.LoadScript(node.actionScript);
		}
	}
}
