#include "BehaviorTreeAsset.h"

#include <algorithm>
#include <fstream>
#include <unordered_set>

bool BehaviorTreeAsset::Load(const std::string& path, std::string& error)
{
	std::ifstream file(path);
	if (!file)
	{
		error = "ファイルを開けませんでした: " + path;
		return false;
	}

	try
	{
		nlohmann::json root;
		file >> root;
		*this = root.get<BehaviorTreeAsset>();
	}
	catch (const std::exception& e)
	{
		error = std::string("JSON の解析に失敗しました: ") + e.what();
		return false;
	}

	error.clear();
	return true;
}

bool BehaviorTreeAsset::Save(const std::string& path, std::string& error) const
{
	std::ofstream file(path);
	if (!file)
	{
		error = "ファイルを書き込めませんでした: " + path;
		return false;
	}

	try
	{
		const nlohmann::json root = *this;
		file << root.dump(1, '\t');
	}
	catch (const std::exception& e)
	{
		error = std::string("JSON の書き出しに失敗しました: ") + e.what();
		return false;
	}

	error.clear();
	return true;
}

BehaviorNodeAsset* BehaviorTreeAsset::FindNode(int id)
{
	for (BehaviorNodeAsset& node : nodes)
	{
		if (node.id == id) return &node;
	}
	return nullptr;
}

const BehaviorNodeAsset* BehaviorTreeAsset::FindNode(int id) const
{
	return const_cast<BehaviorTreeAsset*>(this)->FindNode(id);
}

BehaviorNodeAsset* BehaviorTreeAsset::FindRoot()
{
	for (BehaviorNodeAsset& node : nodes)
	{
		if (node.parentId < 0) return &node;
	}
	return nullptr;
}

std::vector<BehaviorNodeAsset*> BehaviorTreeAsset::FindChildren(int parentId)
{
	std::vector<BehaviorNodeAsset*> children;
	for (BehaviorNodeAsset& node : nodes)
	{
		if (node.parentId == parentId) children.emplace_back(&node);
	}
	return children;
}

std::vector<const BehaviorNodeAsset*> BehaviorTreeAsset::FindChildren(int parentId) const
{
	std::vector<const BehaviorNodeAsset*> children;
	for (const BehaviorNodeAsset& node : nodes)
	{
		if (node.parentId == parentId) children.emplace_back(&node);
	}
	return children;
}

int BehaviorTreeAsset::GenerateId() const
{
	int maxId = 0;
	for (const BehaviorNodeAsset& node : nodes)
	{
		maxId = (std::max)(maxId, node.id);
	}
	return maxId + 1;
}

BehaviorNodeAsset& BehaviorTreeAsset::AddNode(int parentId, const std::string& name)
{
	BehaviorNodeAsset node;
	node.id = GenerateId();
	node.parentId = parentId;
	node.name = name;

	// 親の右隣に置いておくと、追加直後に画面外へ飛ばずに済む
	if (const BehaviorNodeAsset* parent = FindNode(parentId))
	{
		node.editorX = parent->editorX + 260.0f;
		node.editorY = parent->editorY + 120.0f * static_cast<float>(FindChildren(parentId).size());
	}

	nodes.emplace_back(std::move(node));
	return nodes.back();
}

void BehaviorTreeAsset::RemoveNodeRecursive(int id)
{
	// 削除対象を先に集めてから消す（走査中に配列を壊さないため）
	std::unordered_set<int> doomed;
	std::vector<int> pending{ id };

	while (!pending.empty())
	{
		const int current = pending.back();
		pending.pop_back();

		if (!doomed.insert(current).second) continue;

		for (const BehaviorNodeAsset& node : nodes)
		{
			if (node.parentId == current) pending.emplace_back(node.id);
		}
	}

	nodes.erase(
		std::remove_if(nodes.begin(), nodes.end(),
			[&doomed](const BehaviorNodeAsset& node) { return doomed.count(node.id) > 0; }),
		nodes.end());
}

bool BehaviorTreeAsset::IsDescendantOf(int descendantId, int ancestorId) const
{
	// 壊れたデータで無限ループしないよう、辿る回数をノード数で頭打ちにする
	int current = descendantId;
	for (size_t guard = 0; guard <= nodes.size(); ++guard)
	{
		if (current < 0) return false;
		if (current == ancestorId) return true;

		const BehaviorNodeAsset* node = FindNode(current);
		if (node == nullptr) return false;

		current = node->parentId;
	}
	return false;
}

bool BehaviorTreeAsset::Validate(std::vector<std::string>& problems) const
{
	problems.clear();

	if (nodes.empty())
	{
		problems.emplace_back("ノードが 1 つもありません");
		return false;
	}

	// ID の重複
	std::unordered_set<int> seenIds;
	for (const BehaviorNodeAsset& node : nodes)
	{
		if (!seenIds.insert(node.id).second)
		{
			problems.emplace_back("ID が重複しています: " + std::to_string(node.id));
		}
	}

	// ルートはちょうど 1 つ
	int rootCount = 0;
	for (const BehaviorNodeAsset& node : nodes)
	{
		if (node.parentId < 0) ++rootCount;
	}
	if (rootCount == 0)
	{
		problems.emplace_back("ルートノードがありません");
	}
	else if (rootCount > 1)
	{
		problems.emplace_back("ルートノードが " + std::to_string(rootCount) + " 個あります（1 つにしてください）");
	}

	for (const BehaviorNodeAsset& node : nodes)
	{
		const std::string label = "[" + node.name + "]";

		if (node.name.empty())
		{
			problems.emplace_back("ID " + std::to_string(node.id) + " のノードに名前がありません");
		}

		// 親が存在するか
		if (node.parentId >= 0 && FindNode(node.parentId) == nullptr)
		{
			problems.emplace_back(label + " の親ノード (ID " + std::to_string(node.parentId) + ") が見つかりません");
		}

		// 自分自身を先祖に持つ（循環）
		if (node.parentId >= 0 && IsDescendantOf(node.parentId, node.id))
		{
			problems.emplace_back(label + " の親子関係が循環しています");
		}

		const bool hasChildren = !FindChildren(node.id).empty();
		const bool hasAction = (node.actionKind != BehaviorActionKind::None);

		// 行動も子も持たないノードは、選ばれても何も起きない
		if (!hasAction && !hasChildren)
		{
			problems.emplace_back(label + " は行動も子ノードも持っていません");
		}

		// 行動を持つノードは末端。子がいると Inference が子側へ降りてしまう
		if (hasAction && hasChildren)
		{
			problems.emplace_back(label + " は行動と子ノードの両方を持っています（どちらか一方にしてください）");
		}

		if (hasChildren && node.selectRule == BehaviorSelectRule::Non)
		{
			problems.emplace_back(label + " は子ノードを持つのに選択ルールが Non です");
		}

		if (node.actionKind == BehaviorActionKind::Native && node.actionName.empty())
		{
			problems.emplace_back(label + " の Native 行動名が空です");
		}

		if (node.actionKind == BehaviorActionKind::Lua && node.actionScript.empty())
		{
			problems.emplace_back(label + " の Lua スクリプトパスが空です");
		}
	}

	return problems.empty();
}
