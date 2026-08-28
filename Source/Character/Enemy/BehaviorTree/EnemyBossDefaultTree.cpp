#include "EnemyBossBehavior.h"

#include <vector>

// ---------------------------------------------------------------------------
// これまで EnemyBoss.cpp に直接書かれていたツリー構成を、データとして持つ場所。
//
// EnemyBoss にも Lua にも依存しない純粋なデータ定義なので、
// ゲーム本体を起動しなくても JSON を書き出せる。
// ---------------------------------------------------------------------------

namespace
{
	// 既定のツリーを組み立てるための小さな補助。
	// 追加した順がそのまま子ノードの並び順になる（シーケンスの実行順に効く）。
	class DefaultTreeWriter
	{
	public:
		explicit DefaultTreeWriter(BehaviorTreeAsset& asset) : asset(asset) {}

		int AddBranch(
			int parentId,
			const std::string& name,
			int priority,
			BehaviorSelectRule rule,
			const std::string& judgment = "")
		{
			BehaviorNodeAsset& node = asset.AddNode(parentId, name);
			node.priority = priority;
			node.selectRule = rule;
			node.judgment = judgment;
			node.actionKind = BehaviorActionKind::None;
			return node.id;
		}

		int AddLeaf(
			int parentId,
			const std::string& name,
			int priority,
			const std::string& actionName,
			const std::string& judgment = "")
		{
			BehaviorNodeAsset& node = asset.AddNode(parentId, name);
			node.priority = priority;
			node.selectRule = BehaviorSelectRule::Non;
			node.judgment = judgment;
			node.actionKind = BehaviorActionKind::Native;
			node.actionName = actionName;
			return node.id;
		}

	private:
		BehaviorTreeAsset& asset;
	};

	// ノードをツリーの形に見えるよう並べ直す（エディタで開いたときの初期配置）
	void LayoutTree(BehaviorTreeAsset& asset)
	{
		constexpr float COLUMN_WIDTH = 260.0f;
		constexpr float ROW_HEIGHT = 90.0f;

		float nextRow = 0.0f;

		// 深さ優先で並べると、親の右側に子がぶら下がった形になる
		struct Frame { int id; int depth; };
		std::vector<Frame> stack;

		if (const BehaviorNodeAsset* root = asset.FindRoot())
		{
			stack.push_back({ root->id, 0 });
		}

		while (!stack.empty())
		{
			const Frame frame = stack.back();
			stack.pop_back();

			if (BehaviorNodeAsset* node = asset.FindNode(frame.id))
			{
				node->editorX = COLUMN_WIDTH * static_cast<float>(frame.depth);
				node->editorY = ROW_HEIGHT * nextRow;
				nextRow += 1.0f;
			}

			// 逆順に積むと、取り出すときに元の並び順になる
			const std::vector<BehaviorNodeAsset*> children = asset.FindChildren(frame.id);
			for (auto it = children.rbegin(); it != children.rend(); ++it)
			{
				stack.push_back({ (*it)->id, frame.depth + 1 });
			}
		}
	}
}

namespace EnemyBossBehavior
{
	const char* GetTreeAssetPath()
	{
		return "Data/Json/BehaviorTree_EnemyBoss.json";
	}

	const char* GetScriptDirectory()
	{
		return "Data/Script/Enemy/EnemyBoss";
	}

	BehaviorTreeAsset MakeDefaultTreeAsset()
	{
		BehaviorTreeAsset asset;
		asset.character = "EnemyBoss";

		DefaultTreeWriter writer(asset);

		const int root = writer.AddBranch(-1, "Root", 0, BehaviorSelectRule::Priority);

		// 戦闘
		const int battle = writer.AddBranch(root, "Battle", 1, BehaviorSelectRule::Priority, "BattleJudgment");
		writer.AddLeaf(battle, "Dead", 1, "DeadAction", "DeadJudgment");
		writer.AddLeaf(battle, "Damage", 1, "UnifiedDamageAction", "AnyDamageJudgment");
		writer.AddLeaf(battle, "Fall", 2, "FallAction", "FallJudgment");
		writer.AddLeaf(battle, "Pursuit", 3, "PursuitAction");

		// 近距離攻撃
		const int attack = writer.AddBranch(battle, "Attack", 2, BehaviorSelectRule::Random, "AttackJudgment");
		writer.AddLeaf(attack, "SlashCombo", 1, "SlashCombo1Action");
		writer.AddLeaf(attack, "TeleportCombo", 2, "TeleportCombo");
		writer.AddLeaf(attack, "TeleportAssault", 3, "TelePortAssault");
		writer.AddLeaf(attack, "NormalTeleport", 4, "NormalTeleport");
		writer.AddLeaf(attack, "PillarSpiralConv", 4, "PillarSpiralConv", "DyingJudgment");
		{
			const int sequence = writer.AddBranch(attack, "TeleportDashSlash", 4, BehaviorSelectRule::Sequence);
			writer.AddLeaf(sequence, "NormalTeleport", 4, "NormalTeleport");
			writer.AddLeaf(sequence, "DashSlash", 4, "DashSlashAction");
		}
		{
			const int sequence = writer.AddBranch(attack, "TeleportSlashWave", 4, BehaviorSelectRule::Sequence);
			writer.AddLeaf(sequence, "NormalTeleport", 4, "NormalTeleport");
			writer.AddLeaf(sequence, "SlashWave", 4, "SlashWave");
		}

		// 遠距離攻撃
		const int longRange = writer.AddBranch(battle, "LongRange", 2, BehaviorSelectRule::Random, "LongRangeJudgment");
		writer.AddLeaf(longRange, "DashSlash", 1, "DashSlashAction");
		{
			const int sequence = writer.AddBranch(longRange, "DashSlashCombo", 2, BehaviorSelectRule::Sequence, "DyingJudgment");
			writer.AddLeaf(sequence, "DashSlash", 2, "DashSlashAction");
			writer.AddLeaf(sequence, "DashSlash", 2, "DashSlashAction");
			writer.AddLeaf(sequence, "DashSlash", 2, "DashSlashAction");
		}
		writer.AddLeaf(longRange, "TripleTeleport", 2, "TripleTeleportAction");
		{
			const int sequence = writer.AddBranch(longRange, "TripleTeleportAssault", 2, BehaviorSelectRule::Sequence);
			writer.AddLeaf(sequence, "TeleportAssault", 2, "TelePortAssault");
			writer.AddLeaf(sequence, "TeleportAssault", 2, "TelePortAssault");
			writer.AddLeaf(sequence, "TeleportAssault", 2, "TelePortAssault");
		}
		writer.AddLeaf(longRange, "TeleportCombo", 2, "TeleportCombo");
		writer.AddLeaf(longRange, "SlashWave", 2, "SlashWave");
		writer.AddLeaf(longRange, "PillarSpiralDiff", 3, "PillarSpiralDiff");
		writer.AddLeaf(longRange, "PillarSpiralConv", 3, "PillarSpiralConv", "DyingJudgment");

		// 反撃
		const int revenge = writer.AddBranch(battle, "Revenge", 1, BehaviorSelectRule::Random, "RevengeJudgment");
		writer.AddLeaf(revenge, "RevengeDive", 1, "RevengeDive");
		writer.AddLeaf(revenge, "RevengeAssault", 1, "RevengeAssault");

		// 必殺技
		const int special = writer.AddBranch(battle, "Special", 1, BehaviorSelectRule::Priority, "SpecialAttackJudgment");
		writer.AddLeaf(special, "SpecialAttack", 1, "SpecialAttack");

		// 非戦闘
		const int scout = writer.AddBranch(root, "Scout", 2, BehaviorSelectRule::Priority);
		writer.AddLeaf(scout, "Idle", 2, "IdleAction");

		LayoutTree(asset);
		return asset;
	}
}
