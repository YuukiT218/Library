#include "BehaviorTreeEditor.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

#include "imgui.h"
#include "imgui_node_editor.h"

#include "Character/Enemy/BehaviorTree/BehaviorRegistry.h"
#include "Character/Enemy/BehaviorTree/EnemyBossBehavior.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Model/Model.h"
#include "Model/ModelResource.h"
#include "Script/LuaScriptSystem.h"

namespace ed = ax::NodeEditor;
namespace fs = std::filesystem;

namespace
{
	//----------------------------------------------------------------
	// グラフ上の ID
	//
	// ノードエディタは 0 を無効値として扱うので、
	// アセットのノード ID（1 以上）から重ならないように振り分ける。
	//----------------------------------------------------------------

	int MakeNodeId(int assetId) { return assetId * 4 + 1; }
	int MakeInputPinId(int assetId) { return assetId * 4 + 2; }
	int MakeOutputPinId(int assetId) { return assetId * 4 + 3; }
	int MakeLinkId(int childAssetId) { return childAssetId * 4 + 4; }

	int AssetIdFromNodeId(int nodeId) { return (nodeId - 1) / 4; }
	int AssetIdFromPinId(int pinId) { return (pinId - 2) / 4; }
	int AssetIdFromLinkId(int linkId) { return (linkId - 4) / 4; }

	bool IsInputPin(int pinId) { return (pinId % 4) == 2; }

	//----------------------------------------------------------------
	// ImGui の小さな補助
	//----------------------------------------------------------------

	// std::string を InputText で編集する
	bool StringInput(const char* label, std::string& value, size_t bufferSize = 256)
	{
		std::vector<char> buffer((std::max)(bufferSize, value.size() + 64), '\0');
		std::copy(value.begin(), value.end(), buffer.begin());

		if (ImGui::InputText(label, buffer.data(), buffer.size()))
		{
			value = buffer.data();
			return true;
		}
		return false;
	}

	// 文字列の候補から 1 つ選ぶ。emptyLabel を渡すと「未設定」を選べるようになる。
	bool StringCombo(
		const char* label,
		std::string& value,
		const std::vector<std::string>& items,
		const char* emptyLabel = nullptr)
	{
		const char* preview = value.empty()
			? (emptyLabel != nullptr ? emptyLabel : "")
			: value.c_str();

		bool changed = false;
		if (ImGui::BeginCombo(label, preview))
		{
			if (emptyLabel != nullptr)
			{
				if (ImGui::Selectable(emptyLabel, value.empty()))
				{
					value.clear();
					changed = true;
				}
			}

			for (const std::string& item : items)
			{
				const bool selected = (item == value);
				if (ImGui::Selectable(item.c_str(), selected))
				{
					value = item;
					changed = true;
				}
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		return changed;
	}

	const char* SelectRuleLabel(BehaviorSelectRule rule)
	{
		switch (rule)
		{
		case BehaviorSelectRule::Priority:			return "Priority";
		case BehaviorSelectRule::Sequence:			return "Sequence";
		case BehaviorSelectRule::SequentialLooping:	return "SequentialLooping";
		case BehaviorSelectRule::Random:			return "Random";
		case BehaviorSelectRule::Non:
		default:									return "Non";
		}
	}

	const char* ActionKindLabel(BehaviorActionKind kind)
	{
		switch (kind)
		{
		case BehaviorActionKind::Native:	return "Native (C++)";
		case BehaviorActionKind::Lua:		return "Lua";
		case BehaviorActionKind::None:
		default:							return u8"なし（中間ノード）";
		}
	}

	// ノードの見た目の色。役割が一目で分かるようにする。
	ImU32 NodeColor(const BehaviorNodeAsset& node, bool isActive)
	{
		if (isActive) return IM_COL32(70, 120, 60, 255);	// 実行中

		switch (node.actionKind)
		{
		case BehaviorActionKind::Lua:		return IM_COL32(50, 65, 95, 255);
		case BehaviorActionKind::Native:	return IM_COL32(60, 60, 60, 255);
		case BehaviorActionKind::None:
		default:							return IM_COL32(45, 45, 55, 255);
		}
	}

	// InputTextMultiline から呼ばれるバッファ拡張
	int ScriptResizeCallback(ImGuiInputTextCallbackData* data)
	{
		if (data->EventFlag != ImGuiInputTextFlags_CallbackResize) return 0;

		auto* buffer = static_cast<std::vector<char>*>(data->UserData);

		const size_t required =
			(std::max)(static_cast<size_t>(data->BufSize), static_cast<size_t>(data->BufTextLen) + 1);

		buffer->resize(required);
		data->Buf = buffer->data();
		data->BufSize = static_cast<int>(buffer->size());
		return 0;
	}

	// 文字列を編集用バッファへ移す
	void SetBuffer(std::vector<char>& buffer, const std::string& text, size_t minimumSize = 4096)
	{
		buffer.assign((std::max)(minimumSize, text.size() + 1), '\0');
		std::copy(text.begin(), text.end(), buffer.begin());
	}

	long long FileWriteTime(const std::string& path)
	{
		std::error_code ec;
		const fs::file_time_type time = fs::last_write_time(path, ec);
		if (ec) return 0;
		return time.time_since_epoch().count();
	}

	// 新規スクリプトのひな形。
	// そのまま動くものを置いておくと、書き始めるときに迷わない。
	std::string MakeScriptTemplate(const std::string& displayName)
	{
		return
			"-- " + displayName + "\n"
			"--\n"
			"-- self          : この行動インスタンス専用の作業領域（step などを自由に置ける）\n"
			"-- self.node     : エディタで設定した内容（name / params / clips）\n"
			"-- Boss.*        : EnemyBoss の操作\n"
			"-- 戻り値         : \"run\"（続行）/ \"complete\"（成功）/ \"failed\"（中断）\n"
			"\n"
			"local M = {}\n"
			"\n"
			"function M:Run(dt)\n"
			"    Boss.SetTargetToPlayer()\n"
			"\n"
			"    if self.step == nil then self.step = 0 end\n"
			"\n"
			"    if self.step == 0 then\n"
			"        -- インスペクタの「アニメーション」1 番目を再生する\n"
			"        Boss.PlayClip(1)\n"
			"        self.step = 1\n"
			"\n"
			"    elseif self.step == 1 then\n"
			"        Boss.TurnToTarget(dt, Boss.TURN_INSTANT)\n"
			"        Boss.UpdateAttackCollision()\n"
			"\n"
			"        -- キャンセル受付に入ったら次へ進める\n"
			"        if Boss.IsPastCancelEnd(1) then\n"
			"            self.step = 2\n"
			"        end\n"
			"\n"
			"    elseif self.step == 2 then\n"
			"        if not Boss.IsPlayingAnimation() then\n"
			"            return \"complete\"\n"
			"        end\n"
			"    end\n"
			"\n"
			"    -- 被弾・死亡で中断\n"
			"    if Boss.IsInterrupted() then\n"
			"        return \"failed\"\n"
			"    end\n"
			"\n"
			"    return \"run\"\n"
			"end\n"
			"\n"
			"function M:OnReset()\n"
			"    self.step = 0\n"
			"end\n"
			"\n"
			"return M\n";
	}
}

//--------------------------------------------------------------------
// 生成・破棄
//--------------------------------------------------------------------

BehaviorTreeEditor& BehaviorTreeEditor::Instance()
{
	static BehaviorTreeEditor instance;
	return instance;
}

BehaviorTreeEditor::~BehaviorTreeEditor()
{
	Finalize();
}

void BehaviorTreeEditor::Initialize()
{
	if (graphContext != nullptr) return;

	ed::Config config;

	// ノードの位置はツリー JSON 側に持たせるので、
	// ノードエディタ自前の保存ファイルは使わない。
	config.SettingsFile = nullptr;

	graphContext = ed::CreateEditor(&config);

	SetBuffer(newScriptName, "", 128);
}

void BehaviorTreeEditor::Finalize()
{
	if (graphContext == nullptr) return;

	ed::DestroyEditor(graphContext);
	graphContext = nullptr;
}

//--------------------------------------------------------------------
// 更新（ホットリロード監視）
//--------------------------------------------------------------------

void BehaviorTreeEditor::Update(float elapsedTime, EnemyBoss* boss)
{
	if (statusTimer > 0.0f) statusTimer -= elapsedTime;

	if (!hotReloadEnabled) return;

	// 毎フレーム更新時刻を見に行くほどではないので、少し間隔を空ける
	hotReloadTimer -= elapsedTime;
	if (hotReloadTimer > 0.0f) return;
	hotReloadTimer = 0.25f;

	LuaScriptSystem& lua = LuaScriptSystem::Instance();
	if (lua.IsReady())
	{
		const std::vector<std::string> reloaded = lua.HotReload();
		if (!reloaded.empty())
		{
			SetStatus(u8"スクリプトを再読み込みしました: " + reloaded.front()
				+ (reloaded.size() > 1 ? u8" ほか" + std::to_string(reloaded.size() - 1) + u8" 件" : ""), false);

			// 編集中のファイルが外で書き換わったら、表示も追従させる。
			// ただしエディタ側に未保存の変更があるときは上書きしない。
			for (const std::string& path : reloaded)
			{
				if (path == scriptPath && !scriptDirty) OpenScript(path);
			}
		}
	}

	// ツリー JSON が外部で書き換わっていたら取り込む
	if (watchTreeFile && boss != nullptr)
	{
		const std::string path = EnemyBossBehavior::GetTreeAssetPath();
		const long long writeTime = FileWriteTime(path);

		if (writeTime != 0 && treeFileWriteTime != 0 && writeTime != treeFileWriteTime && !dirty)
		{
			treeFileWriteTime = writeTime;
			ReloadFromFile();
			Apply(boss);
		}
		else if (treeFileWriteTime == 0)
		{
			treeFileWriteTime = writeTime;
		}
	}
}

//--------------------------------------------------------------------
// 描画
//--------------------------------------------------------------------

void BehaviorTreeEditor::DrawGui(EnemyBoss* boss)
{
	if (!isOpen) return;

	Initialize();

	if (!assetLoaded && boss != nullptr) LoadFromBoss(boss);

	DrawGraphWindow(boss);
	DrawInspectorWindow(boss);
	DrawScriptWindow();
}

void BehaviorTreeEditor::DrawGraphWindow(EnemyBoss* boss)
{
	if (!ImGui::Begin(u8"Behavior Tree", &isOpen, ImGuiWindowFlags_None))
	{
		ImGui::End();
		return;
	}

	DrawToolbar(boss);
	ImGui::Separator();
	DrawGraph(boss);

	ImGui::End();
}

void BehaviorTreeEditor::DrawToolbar(EnemyBoss* boss)
{
	if (ImGui::Button(u8"適用")) Apply(boss);
	ImGui::SameLine();
	if (ImGui::Button(u8"保存")) Save(boss);
	ImGui::SameLine();
	if (ImGui::Button(u8"ファイルから再読込"))
	{
		ReloadFromFile();
		Apply(boss);
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"既定の構成に戻す"))
	{
		asset = EnemyBossBehavior::MakeDefaultTreeAsset();
		pendingLayout = true;
		dirty = true;
		selectedNodeId = -1;
		SetStatus(u8"既定の構成を読み込みました（未保存）", false);
	}

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(16.0f, 0.0f));
	ImGui::SameLine();

	if (ImGui::Button(u8"ノード追加"))
	{
		const int parentId = (selectedNodeId >= 0) ? selectedNodeId : -1;
		if (asset.FindRoot() == nullptr)
		{
			BehaviorNodeAsset& node = asset.AddNode(-1, "Root");
			node.selectRule = BehaviorSelectRule::Priority;
			selectedNodeId = node.id;
		}
		else if (parentId >= 0)
		{
			BehaviorNodeAsset& node = asset.AddNode(parentId, "NewNode");
			selectedNodeId = node.id;

			// 親が末端のままだと子が選ばれないので、選択ルールを補っておく
			if (BehaviorNodeAsset* parent = asset.FindNode(parentId))
			{
				if (parent->selectRule == BehaviorSelectRule::Non)
				{
					parent->selectRule = BehaviorSelectRule::Priority;
				}
			}
		}
		pendingLayout = true;
		dirty = true;
	}

	ImGui::SameLine();
	if (ImGui::Button(u8"選択ノード削除") && selectedNodeId >= 0)
	{
		asset.RemoveNodeRecursive(selectedNodeId);
		selectedNodeId = -1;
		dirty = true;
	}

	ImGui::SameLine();
	ImGui::Checkbox(u8"ホットリロード", &hotReloadEnabled);

	// 状態表示
	ImGui::SameLine();
	ImGui::Dummy(ImVec2(16.0f, 0.0f));
	ImGui::SameLine();

	if (dirty) ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), u8"* 未保存");
	else       ImGui::TextDisabled(u8"保存済み");

	if (statusTimer > 0.0f && !statusMessage.empty())
	{
		const ImVec4 color = statusIsError
			? ImVec4(1.0f, 0.45f, 0.45f, 1.0f)
			: ImVec4(0.55f, 0.9f, 0.55f, 1.0f);
		ImGui::TextColored(color, "%s", statusMessage.c_str());
	}

	DrawProblemSection();
}

void BehaviorTreeEditor::DrawProblemSection()
{
	if (problems.empty()) return;

	ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f),
		u8"問題が %d 件あります", static_cast<int>(problems.size()));

	if (ImGui::TreeNode(u8"問題の内容"))
	{
		for (const std::string& problem : problems)
		{
			ImGui::BulletText("%s", problem.c_str());
		}
		ImGui::TreePop();
	}
}

void BehaviorTreeEditor::DrawGraph(EnemyBoss* boss)
{
	const int activeNodeId = (boss != nullptr) ? boss->GetActiveNodeSourceId() : -1;

	ed::SetCurrentEditor(graphContext);
	ed::Begin("BehaviorTreeGraph", ImVec2(0.0f, 0.0f));

	if (pendingLayout)
	{
		PushNodePositions();
		pendingLayout = false;
	}

	for (BehaviorNodeAsset& node : asset.nodes)
	{
		DrawGraphNode(node, activeNodeId);
	}

	// 親子関係をリンクとして描く
	for (const BehaviorNodeAsset& node : asset.nodes)
	{
		if (node.parentId < 0) continue;
		if (asset.FindNode(node.parentId) == nullptr) continue;

		ed::Link(
			ed::LinkId(MakeLinkId(node.id)),
			ed::PinId(MakeOutputPinId(node.parentId)),
			ed::PinId(MakeInputPinId(node.id)));
	}

	HandleGraphCreate();
	HandleGraphDelete();

	// 選択ノードを拾う
	if (ed::GetSelectedObjectCount() > 0)
	{
		ed::NodeId selected[1];
		if (ed::GetSelectedNodes(selected, 1) > 0)
		{
			selectedNodeId = AssetIdFromNodeId(static_cast<int>(selected[0].Get()));
		}
	}

	ed::Suspend();
	DrawGraphContextMenu();
	ed::Resume();

	ed::End();

	// ドラッグで動かした位置をアセットへ書き戻す
	PullNodePositions();

	ed::SetCurrentEditor(nullptr);
}

void BehaviorTreeEditor::DrawGraphNode(BehaviorNodeAsset& node, int activeNodeId)
{
	const bool isActive = (node.id == activeNodeId);

	ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(NodeColor(node, isActive)));
	ed::BeginNode(ed::NodeId(MakeNodeId(node.id)));

	// ヘッダ
	ImGui::PushID(node.id);
	if (isActive) ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), u8"%s  (実行中)", node.name.c_str());
	else          ImGui::Text("%s", node.name.c_str());

	// 入力ピン（親から）
	ed::BeginPin(ed::PinId(MakeInputPinId(node.id)), ed::PinKind::Input);
	ImGui::Text("-> in");
	ed::EndPin();

	ImGui::SameLine();
	ImGui::Dummy(ImVec2(40.0f, 0.0f));
	ImGui::SameLine();

	// 出力ピン（子へ）
	ed::BeginPin(ed::PinId(MakeOutputPinId(node.id)), ed::PinKind::Output);
	ImGui::Text("out ->");
	ed::EndPin();

	// 中身の要約
	ImGui::TextDisabled(u8"優先度 %d / %s", node.priority, SelectRuleLabel(node.selectRule));

	if (!node.judgment.empty())
	{
		ImGui::TextDisabled(u8"判定: %s", node.judgment.c_str());
	}

	switch (node.actionKind)
	{
	case BehaviorActionKind::Native:
		ImGui::TextDisabled(u8"C++: %s", node.actionName.c_str());
		break;

	case BehaviorActionKind::Lua:
	{
		const fs::path scriptFile(node.actionScript);
		ImGui::TextDisabled(u8"Lua: %s", scriptFile.filename().string().c_str());

		if (LuaScriptSystem::Instance().HasError(node.actionScript))
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), u8"スクリプトエラー");
		}
		break;
	}

	case BehaviorActionKind::None:
	default:
		break;
	}

	if (!node.clips.empty())
	{
		ImGui::TextDisabled(u8"アニメ %d 本", static_cast<int>(node.clips.size()));
	}

	ImGui::PopID();
	ed::EndNode();
	ed::PopStyleColor();
}

void BehaviorTreeEditor::HandleGraphCreate()
{
	if (!ed::BeginCreate()) { ed::EndCreate(); return; }

	ed::PinId startPin;
	ed::PinId endPin;

	if (ed::QueryNewLink(&startPin, &endPin))
	{
		if (startPin && endPin)
		{
			int startId = static_cast<int>(startPin.Get());
			int endId = static_cast<int>(endPin.Get());

			// 出力→入力の向きに揃える
			if (IsInputPin(startId)) std::swap(startId, endId);

			const int parentId = AssetIdFromPinId(startId);
			const int childId = AssetIdFromPinId(endId);

			const BehaviorNodeAsset* parent = asset.FindNode(parentId);
			const BehaviorNodeAsset* child = asset.FindNode(childId);

			if (parent == nullptr || child == nullptr || parentId == childId)
			{
				ed::RejectNewItem(ImColor(255, 80, 80), 2.0f);
			}
			else if (asset.IsDescendantOf(parentId, childId))
			{
				// 子孫を親にすると輪になってしまう
				ed::RejectNewItem(ImColor(255, 80, 80), 2.0f);
			}
			else if (ed::AcceptNewItem())
			{
				Reparent(childId, parentId);
			}
		}
	}

	ed::EndCreate();
}

void BehaviorTreeEditor::HandleGraphDelete()
{
	if (!ed::BeginDelete()) { ed::EndDelete(); return; }

	ed::LinkId linkId;
	while (ed::QueryDeletedLink(&linkId))
	{
		if (!ed::AcceptDeletedItem()) continue;

		// リンクを切る＝親を外す。ルートが 2 つになるので検証で引っかかる。
		const int childId = AssetIdFromLinkId(static_cast<int>(linkId.Get()));
		if (BehaviorNodeAsset* child = asset.FindNode(childId))
		{
			child->parentId = -1;
			dirty = true;
		}
	}

	ed::NodeId nodeId;
	while (ed::QueryDeletedNode(&nodeId))
	{
		if (!ed::AcceptDeletedItem()) continue;

		const int assetId = AssetIdFromNodeId(static_cast<int>(nodeId.Get()));
		asset.RemoveNodeRecursive(assetId);
		if (selectedNodeId == assetId) selectedNodeId = -1;
		dirty = true;
	}

	ed::EndDelete();
}

void BehaviorTreeEditor::DrawGraphContextMenu()
{
	if (ImGui::BeginPopup("BehaviorTreeNodeMenu"))
	{
		if (ImGui::MenuItem(u8"子ノードを追加") && selectedNodeId >= 0)
		{
			asset.AddNode(selectedNodeId, "NewNode");
			pendingLayout = true;
			dirty = true;
		}
		if (ImGui::MenuItem(u8"このノードを削除") && selectedNodeId >= 0)
		{
			asset.RemoveNodeRecursive(selectedNodeId);
			selectedNodeId = -1;
			dirty = true;
		}
		ImGui::EndPopup();
	}

	ed::NodeId contextNode;
	if (ed::ShowNodeContextMenu(&contextNode))
	{
		selectedNodeId = AssetIdFromNodeId(static_cast<int>(contextNode.Get()));
		ImGui::OpenPopup("BehaviorTreeNodeMenu");
	}
}

//--------------------------------------------------------------------
// インスペクタ
//--------------------------------------------------------------------

void BehaviorTreeEditor::DrawInspectorWindow(EnemyBoss* boss)
{
	if (!ImGui::Begin(u8"Behavior Inspector", nullptr, ImGuiWindowFlags_None))
	{
		ImGui::End();
		return;
	}

	BehaviorNodeAsset* node = GetSelectedNode();
	if (node == nullptr)
	{
		ImGui::TextDisabled(u8"グラフでノードを選択してください");
		ImGui::End();
		return;
	}

	Model* model = GetBossModel(boss);

	ImGui::PushID(node->id);

	DrawNodeBasics(*node);
	ImGui::Separator();
	DrawActionSection(*node);
	ImGui::Separator();
	DrawClipSection(*node, model);
	ImGui::Separator();
	DrawParamSection(*node);

	ImGui::PopID();
	ImGui::End();
}

void BehaviorTreeEditor::DrawNodeBasics(BehaviorNodeAsset& node)
{
	ImGui::Text(u8"ノード ID: %d", node.id);

	if (StringInput(u8"名前", node.name)) dirty = true;

	if (ImGui::DragInt(u8"優先度", &node.priority, 0.1f, 0, 99))
	{
		dirty = true;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(u8"Priority ルールの親では、小さいほど先に選ばれる");
	}

	// 選択ルール
	if (ImGui::BeginCombo(u8"子の選び方", SelectRuleLabel(node.selectRule)))
	{
		const BehaviorSelectRule rules[] =
		{
			BehaviorSelectRule::Non,
			BehaviorSelectRule::Priority,
			BehaviorSelectRule::Sequence,
			BehaviorSelectRule::SequentialLooping,
			BehaviorSelectRule::Random,
		};

		for (BehaviorSelectRule rule : rules)
		{
			const bool selected = (rule == node.selectRule);
			if (ImGui::Selectable(SelectRuleLabel(rule), selected))
			{
				node.selectRule = rule;
				dirty = true;
			}
			if (selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// 判定
	const std::vector<std::string>& judgments =
		BehaviorRegistry<EnemyBoss>::Instance().GetJudgmentNames();

	if (StringCombo(u8"実行判定", node.judgment, judgments, u8"（常に実行可）"))
	{
		dirty = true;
	}

	if (StringInput(u8"メモ", node.comment, 512)) dirty = true;
}

void BehaviorTreeEditor::DrawActionSection(BehaviorNodeAsset& node)
{
	ImGui::Text(u8"行動");

	if (ImGui::BeginCombo(u8"種類", ActionKindLabel(node.actionKind)))
	{
		const BehaviorActionKind kinds[] =
		{
			BehaviorActionKind::None,
			BehaviorActionKind::Native,
			BehaviorActionKind::Lua,
		};

		for (BehaviorActionKind kind : kinds)
		{
			const bool selected = (kind == node.actionKind);
			if (ImGui::Selectable(ActionKindLabel(kind), selected))
			{
				node.actionKind = kind;
				dirty = true;
			}
			if (selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	switch (node.actionKind)
	{
	case BehaviorActionKind::Native:
	{
		const std::vector<std::string>& actions =
			BehaviorRegistry<EnemyBoss>::Instance().GetActionNames();

		if (StringCombo(u8"C++ クラス", node.actionName, actions, u8"（未設定）")) dirty = true;

		ImGui::TextDisabled(u8"C++ で書かれた行動。アニメーション設定はクラス側が持っています。");
		break;
	}

	case BehaviorActionKind::Lua:
	{
		const std::vector<std::string> scripts = CollectScriptFiles();
		if (StringCombo(u8"スクリプト", node.actionScript, scripts, u8"（未設定）")) dirty = true;

		if (!node.actionScript.empty())
		{
			if (ImGui::Button(u8"エディタで開く")) OpenScript(node.actionScript);
			ImGui::SameLine();
			if (ImGui::Button(u8"再読み込み"))
			{
				LuaScriptSystem::Instance().ReloadScript(node.actionScript);
			}

			if (LuaScriptSystem::Instance().HasError(node.actionScript))
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s",
					LuaScriptSystem::Instance().GetError(node.actionScript).c_str());
			}
		}

		ImGui::Separator();
		ImGui::TextDisabled(u8"新しいスクリプトを作る");
		ImGui::InputText(u8"ファイル名", newScriptName.data(), newScriptName.size());
		ImGui::SameLine();
		if (ImGui::Button(u8"作成"))
		{
			std::string name = newScriptName.data();
			if (!name.empty())
			{
				if (name.size() < 4 || name.substr(name.size() - 4) != ".lua") name += ".lua";

				const std::string path =
					std::string(EnemyBossBehavior::GetScriptDirectory()) + "/" + name;

				if (CreateScriptFromTemplate(path, node.name))
				{
					node.actionScript = path;
					node.actionKind = BehaviorActionKind::Lua;
					dirty = true;
					OpenScript(path);
					SetBuffer(newScriptName, "", 128);
				}
			}
		}
		break;
	}

	case BehaviorActionKind::None:
	default:
		ImGui::TextDisabled(u8"行動を持たない中間ノードです。子ノードの選び方だけを決めます。");
		break;
	}
}

void BehaviorTreeEditor::DrawClipSection(BehaviorNodeAsset& node, Model* model)
{
	ImGui::Text(u8"アニメーション");

	if (node.actionKind != BehaviorActionKind::Lua)
	{
		ImGui::TextDisabled(u8"注: ここの設定が効くのは Lua 行動だけです（Boss.PlayClip から使われます）");
	}

	if (ImGui::Button(u8"アニメーションを追加"))
	{
		node.clips.emplace_back();
		selectedClipIndex = static_cast<int>(node.clips.size()) - 1;
		dirty = true;
	}

	if (node.clips.empty())
	{
		ImGui::TextDisabled(u8"まだ設定されていません");
		return;
	}

	selectedClipIndex = std::clamp(selectedClipIndex, 0, static_cast<int>(node.clips.size()) - 1);

	// 一覧
	ImGui::BeginChild("ClipList", ImVec2(220.0f, 120.0f), true);
	for (size_t i = 0; i < node.clips.size(); ++i)
	{
		const BehaviorClip& clip = node.clips[i];

		char label[256];
		snprintf(label, sizeof(label), "%d: %s##clip%zu",
			static_cast<int>(i) + 1,
			clip.clip.empty() ? u8"（未設定）" : clip.clip.c_str(),
			i);

		if (ImGui::Selectable(label, selectedClipIndex == static_cast<int>(i)))
		{
			selectedClipIndex = static_cast<int>(i);
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginGroup();

	if (ImGui::Button(u8"上へ") && selectedClipIndex > 0)
	{
		std::swap(node.clips[selectedClipIndex], node.clips[selectedClipIndex - 1]);
		--selectedClipIndex;
		dirty = true;
	}
	if (ImGui::Button(u8"下へ") && selectedClipIndex + 1 < static_cast<int>(node.clips.size()))
	{
		std::swap(node.clips[selectedClipIndex], node.clips[selectedClipIndex + 1]);
		++selectedClipIndex;
		dirty = true;
	}
	if (ImGui::Button(u8"削除"))
	{
		node.clips.erase(node.clips.begin() + selectedClipIndex);
		dirty = true;
		ImGui::EndGroup();
		return;
	}

	ImGui::EndGroup();

	DrawClipDetail(node.clips[selectedClipIndex], selectedClipIndex, model);
}

void BehaviorTreeEditor::DrawClipDetail(BehaviorClip& clip, int clipIndex, Model* model)
{
	ImGui::PushID(clipIndex);
	ImGui::Separator();

	const std::vector<std::string> animationNames = CollectAnimationNames(model);
	if (StringCombo(u8"クリップ", clip.clip, animationNames, u8"（未設定）")) dirty = true;

	if (ImGui::Checkbox(u8"ループ", &clip.loop)) dirty = true;
	ImGui::SameLine();
	if (ImGui::Checkbox(u8"ルートモーション", &clip.rootMotion)) dirty = true;

	if (ImGui::DragFloat(u8"ブレンド時間", &clip.blendSeconds, 0.005f, 0.0f, 2.0f, "%.3f s")) dirty = true;
	if (ImGui::DragFloat(u8"再生速度", &clip.speedScale, 0.01f, 0.05f, 5.0f, "%.2f x")) dirty = true;

	// --- キャンセル受付 ---
	ImGui::Separator();
	ImGui::Text(u8"キャンセル受付区間");
	ImGui::TextDisabled(u8"次の行動へ移ってよい時間帯。連続行動のつなぎ目をここで決めます。");

	AnimationConfig* config = FindAnimationConfig(model, clip.clip);

	float animationLength = 0.0f;
	if (model != nullptr && !clip.clip.empty())
	{
		const int index = model->GetAnimationIndex(clip.clip.c_str());
		const std::vector<ModelResource::Animation>& animations =
			model->GetResource()->GetAnimations();

		if (index >= 0 && index < static_cast<int>(animations.size()))
		{
			animationLength = animations[index].secondsLength;
		}
	}

	bool overrideCancel = (clip.cancelStart >= 0.0f || clip.cancelEnd >= 0.0f);

	if (ImGui::Checkbox(u8"このノードだけ別の値にする", &overrideCancel))
	{
		if (overrideCancel)
		{
			// アニメーション共通の値を初期値として引き継ぐ
			clip.cancelStart = (config != nullptr) ? config->advanceInputStartFrame : 0.0f;
			clip.cancelEnd = (config != nullptr) ? config->advanceInputEndFrame : animationLength;
		}
		else
		{
			clip.cancelStart = -1.0f;
			clip.cancelEnd = -1.0f;
		}
		dirty = true;
	}

	if (overrideCancel)
	{
		const float maxSeconds = (animationLength > 0.0f) ? animationLength : 10.0f;

		if (ImGui::SliderFloat(u8"開始", &clip.cancelStart, 0.0f, maxSeconds, "%.3f s")) dirty = true;
		if (ImGui::SliderFloat(u8"終了", &clip.cancelEnd, 0.0f, maxSeconds, "%.3f s")) dirty = true;

		if (clip.cancelEnd < clip.cancelStart) clip.cancelEnd = clip.cancelStart;
	}
	else if (config != nullptr)
	{
		ImGui::TextDisabled(u8"アニメーション共通の設定を使用中: %.3f s 〜 %.3f s",
			config->advanceInputStartFrame, config->advanceInputEndFrame);
		ImGui::TextDisabled(u8"（イベントシーケンサーの先行入力受付と同じ値です）");
	}
	else
	{
		ImGui::TextDisabled(u8"このアニメーションにはまだ AnimationConfig がありません");
	}

	if (animationLength > 0.0f)
	{
		ImGui::TextDisabled(u8"アニメーション長: %.3f s", animationLength);
	}

	DrawEventSection(clip, model);

	ImGui::PopID();
}

void BehaviorTreeEditor::DrawEventSection(const BehaviorClip& clip, Model* model)
{
	AnimationConfig* config = FindAnimationConfig(model, clip.clip);
	if (config == nullptr) return;

	ImGui::Separator();

	if (!ImGui::TreeNode(u8"イベント / 属性")) return;

	ImGui::TextDisabled(u8"アニメーション単位の設定です。細かい編集はイベントシーケンサーで行えます。");

	// --- イベント ---
	ImGui::Text(u8"イベント (%d)", static_cast<int>(config->events.size()));
	for (size_t i = 0; i < config->events.size(); ++i)
	{
		AnimationEvent& e = config->events[i];
		ImGui::PushID(static_cast<int>(i));

		const char* typeLabel = (e.eventType == EventType::Camera) ? "Camera" : "Effect";
		ImGui::Text(u8"[%s] %s  %.3f 〜 %.3f s",
			typeLabel, e.eventName.c_str(), e.timeInSeconds, e.timeOutSeconds);

		ImGui::SameLine();
		if (ImGui::SmallButton(u8"削除"))
		{
			config->events.erase(config->events.begin() + i);
			ImGui::PopID();
			break;
		}
		ImGui::PopID();
	}

	if (ImGui::SmallButton(u8"イベント追加"))
	{
		AnimationEvent e{};
		e.timeInSeconds = 0.0f;
		e.timeOutSeconds = 0.1f;
		e.eventType = EventType::Effect;
		e.eventName = "NewEvent";
		config->events.emplace_back(e);
	}

	// --- 属性 ---
	ImGui::Text(u8"属性 (%d)", static_cast<int>(config->attributes.size()));
	for (const AnimationAttribute& attribute : config->attributes)
	{
		const char* flagLabel = "None";
		switch (attribute.flag)
		{
		case AnimationFlag::Attack:		flagLabel = "Attack"; break;
		case AnimationFlag::Invincible:	flagLabel = "Invincible"; break;
		case AnimationFlag::Guard:		flagLabel = "Guard"; break;
		case AnimationFlag::SuperArmor:	flagLabel = "SuperArmor"; break;
		case AnimationFlag::None:
		default:						break;
		}
		ImGui::BulletText(u8"%s  %.3f 〜 %.3f s", flagLabel, attribute.startTime, attribute.endTime);
	}

	ImGui::TreePop();
}

void BehaviorTreeEditor::DrawParamSection(BehaviorNodeAsset& node)
{
	ImGui::Text(u8"パラメータ");
	ImGui::TextDisabled(u8"Lua から self.node.params.<名前> で読めます");

	for (size_t i = 0; i < node.params.size(); ++i)
	{
		BehaviorParam& param = node.params[i];
		ImGui::PushID(static_cast<int>(i));

		ImGui::SetNextItemWidth(120.0f);
		if (StringInput("##name", param.name, 64)) dirty = true;

		ImGui::SameLine();
		ImGui::SetNextItemWidth(80.0f);

		const char* typeLabels[] = { "Float", "Int", "Bool", "String" };
		int typeIndex = static_cast<int>(param.type);
		if (ImGui::Combo("##type", &typeIndex, typeLabels, IM_ARRAYSIZE(typeLabels)))
		{
			param.type = static_cast<BehaviorParam::Type>(typeIndex);
			dirty = true;
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(140.0f);

		switch (param.type)
		{
		case BehaviorParam::Type::Float:
			if (ImGui::DragFloat("##value", &param.floatValue, 0.01f)) dirty = true;
			break;
		case BehaviorParam::Type::Int:
			if (ImGui::DragInt("##value", &param.intValue, 0.1f)) dirty = true;
			break;
		case BehaviorParam::Type::Bool:
			if (ImGui::Checkbox("##value", &param.boolValue)) dirty = true;
			break;
		case BehaviorParam::Type::String:
			if (StringInput("##value", param.stringValue, 128)) dirty = true;
			break;
		default:
			break;
		}

		ImGui::SameLine();
		if (ImGui::SmallButton(u8"削除"))
		{
			node.params.erase(node.params.begin() + i);
			dirty = true;
			ImGui::PopID();
			break;
		}

		ImGui::PopID();
	}

	if (ImGui::Button(u8"パラメータ追加"))
	{
		BehaviorParam param;
		param.name = "param" + std::to_string(node.params.size() + 1);
		node.params.emplace_back(std::move(param));
		dirty = true;
	}
}

//--------------------------------------------------------------------
// スクリプトエディタ
//--------------------------------------------------------------------

void BehaviorTreeEditor::DrawScriptWindow()
{
	if (!ImGui::Begin(u8"Behavior Script", nullptr, ImGuiWindowFlags_None))
	{
		ImGui::End();
		return;
	}

	// 開くファイルの選択
	std::string openPath = scriptPath;
	const std::vector<std::string> scripts = CollectScriptFiles();

	ImGui::SetNextItemWidth(360.0f);
	if (StringCombo(u8"ファイル", openPath, scripts, u8"（未選択）"))
	{
		if (!openPath.empty()) OpenScript(openPath);
	}

	if (scriptPath.empty())
	{
		ImGui::TextDisabled(u8"編集するスクリプトを選んでください");
		ImGui::End();
		return;
	}

	ImGui::SameLine();
	if (ImGui::Button(u8"保存 (Ctrl+S)")) SaveScript();

	ImGui::SameLine();
	if (ImGui::Button(u8"破棄して読み直す")) OpenScript(scriptPath);

	ImGui::SameLine();
	if (scriptDirty) ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), u8"* 未保存");
	else             ImGui::TextDisabled(u8"保存済み");

	// エラー表示
	LuaScriptSystem& lua = LuaScriptSystem::Instance();
	if (lua.HasError(scriptPath))
	{
		ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", lua.GetError(scriptPath).c_str());
	}

	// Ctrl+S で保存
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
		ImGui::GetIO().KeyCtrl &&
		ImGui::IsKeyPressed('S', false))
	{
		SaveScript();
	}

	const ImVec2 size(-1.0f, (std::max)(80.0f, ImGui::GetContentRegionAvail().y - 140.0f));
	if (ImGui::InputTextMultiline(
		"##script",
		scriptBuffer.data(),
		scriptBuffer.size(),
		size,
		ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackResize,
		ScriptResizeCallback,
		&scriptBuffer))
	{
		scriptDirty = true;
	}

	// ログ
	ImGui::Separator();
	ImGui::Text(u8"Lua ログ");
	ImGui::SameLine();
	if (ImGui::SmallButton(u8"消去")) lua.ClearLog();

	ImGui::BeginChild("LuaLog", ImVec2(0.0f, 0.0f), true);
	for (const LuaScriptSystem::LogEntry& entry : lua.GetLog())
	{
		ImVec4 color(0.8f, 0.8f, 0.8f, 1.0f);
		if (entry.level == LuaScriptSystem::LogEntry::Level::Error)   color = ImVec4(1.0f, 0.45f, 0.45f, 1.0f);
		if (entry.level == LuaScriptSystem::LogEntry::Level::Warning) color = ImVec4(1.0f, 0.85f, 0.4f, 1.0f);

		ImGui::TextColored(color, "%s", entry.message.c_str());
	}
	ImGui::EndChild();

	ImGui::End();
}

void BehaviorTreeEditor::OpenScript(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		SetStatus(u8"スクリプトを開けませんでした: " + path, true);
		return;
	}

	const std::string text(
		(std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());

	SetBuffer(scriptBuffer, text, 8192);
	scriptPath = path;
	scriptDirty = false;

	// 読み込んでいないファイルを開いた場合もエラーを見られるようにする
	LuaScriptSystem::Instance().LoadScript(path);
}

bool BehaviorTreeEditor::SaveScript()
{
	if (scriptPath.empty()) return false;

	std::ofstream file(scriptPath, std::ios::binary);
	if (!file)
	{
		SetStatus(u8"スクリプトを保存できませんでした: " + scriptPath, true);
		return false;
	}

	file << scriptBuffer.data();
	file.close();

	scriptDirty = false;

	// 保存したその場で読み直す（ファイル監視を待たない）
	const bool ok = LuaScriptSystem::Instance().ReloadScript(scriptPath);
	SetStatus(ok ? u8"保存して読み込み直しました" : u8"保存しましたがスクリプトにエラーがあります", !ok);
	return ok;
}

bool BehaviorTreeEditor::CreateScriptFromTemplate(const std::string& path, const std::string& displayName)
{
	std::error_code ec;
	fs::create_directories(fs::path(path).parent_path(), ec);

	if (fs::exists(path))
	{
		SetStatus(u8"同じ名前のファイルがすでにあります: " + path, true);
		return false;
	}

	std::ofstream file(path, std::ios::binary);
	if (!file)
	{
		SetStatus(u8"スクリプトを作成できませんでした: " + path, true);
		return false;
	}

	file << MakeScriptTemplate(displayName.empty() ? fs::path(path).stem().string() : displayName);
	file.close();

	SetStatus(u8"スクリプトを作成しました: " + path, false);
	return true;
}

std::vector<std::string> BehaviorTreeEditor::CollectScriptFiles() const
{
	std::vector<std::string> files;

	std::error_code ec;
	const fs::path directory(EnemyBossBehavior::GetScriptDirectory());

	if (!fs::exists(directory, ec)) return files;

	for (const fs::directory_entry& entry : fs::recursive_directory_iterator(directory, ec))
	{
		if (!entry.is_regular_file()) continue;
		if (entry.path().extension() != ".lua") continue;

		// JSON に入れる形（区切りは / ）へ揃えておく
		std::string path = entry.path().generic_string();
		files.emplace_back(std::move(path));
	}

	std::sort(files.begin(), files.end());
	return files;
}

//--------------------------------------------------------------------
// 読み込み・保存・反映
//--------------------------------------------------------------------

void BehaviorTreeEditor::LoadFromBoss(EnemyBoss* boss)
{
	if (boss == nullptr) return;

	asset = boss->GetBehaviorAsset();
	problems = boss->GetBehaviorProblems();

	assetLoaded = true;
	pendingLayout = true;
	dirty = false;

	treeFileWriteTime = FileWriteTime(EnemyBossBehavior::GetTreeAssetPath());
}

void BehaviorTreeEditor::ReloadFromFile()
{
	BehaviorTreeAsset loaded;
	std::string error;

	if (!loaded.Load(EnemyBossBehavior::GetTreeAssetPath(), error))
	{
		SetStatus(error, true);
		return;
	}

	asset = std::move(loaded);
	assetLoaded = true;
	pendingLayout = true;
	dirty = false;
	selectedNodeId = -1;

	treeFileWriteTime = FileWriteTime(EnemyBossBehavior::GetTreeAssetPath());
	SetStatus(u8"ファイルから読み込みました", false);
}

bool BehaviorTreeEditor::Apply(EnemyBoss* boss)
{
	if (boss == nullptr)
	{
		SetStatus(u8"ボスがいないので反映できません", true);
		return false;
	}

	const bool ok = boss->ApplyBehaviorAsset(asset, problems);
	SetStatus(ok ? u8"ツリーを組み直しました" : u8"ツリーに問題があります（下の一覧を確認してください）", !ok);
	return ok;
}

bool BehaviorTreeEditor::Save(EnemyBoss* boss)
{
	// 保存前に位置を取り込んでおく（見た目の配置も一緒に残す）
	PullNodePositions();

	std::vector<std::string> validationProblems;
	if (!asset.Validate(validationProblems))
	{
		problems = validationProblems;
		SetStatus(u8"問題があるため保存しませんでした", true);
		return false;
	}

	std::string error;
	if (!asset.Save(EnemyBossBehavior::GetTreeAssetPath(), error))
	{
		SetStatus(error, true);
		return false;
	}

	dirty = false;
	treeFileWriteTime = FileWriteTime(EnemyBossBehavior::GetTreeAssetPath());

	const bool applied = Apply(boss);
	if (applied) SetStatus(u8"保存して反映しました", false);
	return applied;
}

bool BehaviorTreeEditor::Reparent(int childId, int newParentId)
{
	BehaviorNodeAsset* child = asset.FindNode(childId);
	if (child == nullptr) return false;

	// 付け替え先が自分の子孫だと輪になる
	if (asset.IsDescendantOf(newParentId, childId)) return false;

	child->parentId = newParentId;
	dirty = true;

	// 末端のままの親には子が降りてこないので、選択ルールを補う
	if (BehaviorNodeAsset* parent = asset.FindNode(newParentId))
	{
		if (parent->selectRule == BehaviorSelectRule::Non)
		{
			parent->selectRule = BehaviorSelectRule::Priority;
		}
	}
	return true;
}

void BehaviorTreeEditor::PullNodePositions()
{
	if (graphContext == nullptr) return;

	for (BehaviorNodeAsset& node : asset.nodes)
	{
		const ImVec2 position = ed::GetNodePosition(ed::NodeId(MakeNodeId(node.id)));

		// まだ配置されていないノードは (FLT_MAX, FLT_MAX) が返るので触らない
		if (position.x > 1.0e6f || position.y > 1.0e6f) continue;

		if (position.x != node.editorX || position.y != node.editorY)
		{
			node.editorX = position.x;
			node.editorY = position.y;
			dirty = true;
		}
	}
}

void BehaviorTreeEditor::PushNodePositions()
{
	if (graphContext == nullptr) return;

	for (const BehaviorNodeAsset& node : asset.nodes)
	{
		ed::SetNodePosition(ed::NodeId(MakeNodeId(node.id)), ImVec2(node.editorX, node.editorY));
	}
}

//--------------------------------------------------------------------
// 補助
//--------------------------------------------------------------------

BehaviorNodeAsset* BehaviorTreeEditor::GetSelectedNode()
{
	return (selectedNodeId >= 0) ? asset.FindNode(selectedNodeId) : nullptr;
}

Model* BehaviorTreeEditor::GetBossModel(EnemyBoss* boss) const
{
	return (boss != nullptr) ? boss->GetModel() : nullptr;
}

std::vector<std::string> BehaviorTreeEditor::CollectAnimationNames(Model* model) const
{
	std::vector<std::string> names;
	if (model == nullptr || model->GetResource() == nullptr) return names;

	for (const ModelResource::Animation& animation : model->GetResource()->GetAnimations())
	{
		names.emplace_back(animation.name);
	}
	return names;
}

AnimationConfig* BehaviorTreeEditor::FindAnimationConfig(Model* model, const std::string& clipName) const
{
	if (model == nullptr || clipName.empty()) return nullptr;

	const int index = model->GetAnimationIndex(clipName.c_str());
	if (index < 0) return nullptr;

	return model->GetAnimationConfig("EnemyBoss", index);
}

void BehaviorTreeEditor::SetStatus(const std::string& message, bool isError)
{
	statusMessage = message;
	statusIsError = isError;
	statusTimer = isError ? 8.0f : 4.0f;
}
