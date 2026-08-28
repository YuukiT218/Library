#pragma once

#include <string>
#include <vector>

#include "Character/Enemy/BehaviorTree/BehaviorTreeAsset.h"

class EnemyBoss;
class Model;
struct AnimationConfig;

namespace ax { namespace NodeEditor { struct EditorContext; } }

// ---------------------------------------------------------------------------
// 敵の行動パターンを組み立てるエディタ。
//
//  ・ノードグラフ    ：ノードを繋いでツリーの形を作る
//  ・インスペクタ    ：選んだノードの判定・行動・アニメーション・キャンセル区間を設定
//  ・スクリプト      ：行動の中身（Lua）をその場で編集して保存・ホットリロード
//
// 編集はアセットの複製に対して行い、「適用」を押したときだけ
// 動いているボスのツリーを組み直す。壊れた状態が即座に反映されないようにするため。
// ---------------------------------------------------------------------------
class BehaviorTreeEditor
{
public:
	static BehaviorTreeEditor& Instance();

	// コピー禁止
	BehaviorTreeEditor(const BehaviorTreeEditor&) = delete;
	void operator=(const BehaviorTreeEditor&) = delete;

	void Initialize();
	void Finalize();

	// ホットリロードの監視。毎フレーム呼ぶ。
	void Update(float elapsedTime, EnemyBoss* boss);

	// ウィンドウ描画。ImGui のフレーム内で呼ぶ。
	void DrawGui(EnemyBoss* boss);

	bool IsOpen() const { return isOpen; }
	void SetOpen(bool open) { isOpen = open; }

private:
	BehaviorTreeEditor() = default;
	~BehaviorTreeEditor();

	//----------------------------------------------------------------
	// ウィンドウ
	//----------------------------------------------------------------

	void DrawGraphWindow(EnemyBoss* boss);
	void DrawInspectorWindow(EnemyBoss* boss);
	void DrawScriptWindow();

	void DrawToolbar(EnemyBoss* boss);
	void DrawGraph(EnemyBoss* boss);
	void DrawGraphNode(BehaviorNodeAsset& node, int activeNodeId);
	void HandleGraphCreate();
	void HandleGraphDelete();
	void DrawGraphContextMenu();

	void DrawNodeBasics(BehaviorNodeAsset& node);
	void DrawActionSection(BehaviorNodeAsset& node);
	void DrawClipSection(BehaviorNodeAsset& node, Model* model);
	void DrawClipDetail(BehaviorClip& clip, int clipIndex, Model* model);
	void DrawEventSection(const BehaviorClip& clip, Model* model);
	void DrawParamSection(BehaviorNodeAsset& node);
	void DrawProblemSection();

	//----------------------------------------------------------------
	// 操作
	//----------------------------------------------------------------

	// 走っているボスからアセットを取り込んで編集を始める
	void LoadFromBoss(EnemyBoss* boss);

	// ファイルから読み直す
	void ReloadFromFile();

	// 編集内容をボスへ反映する
	bool Apply(EnemyBoss* boss);

	// 編集内容をファイルへ保存する（あわせて反映も行う）
	bool Save(EnemyBoss* boss);

	// 親子付け替え。循環するなら false
	bool Reparent(int childId, int newParentId);

	// ノード位置をグラフ側から取り込む／グラフ側へ流し込む
	void PullNodePositions();
	void PushNodePositions();

	//----------------------------------------------------------------
	// スクリプト
	//----------------------------------------------------------------

	void OpenScript(const std::string& path);
	bool SaveScript();
	bool CreateScriptFromTemplate(const std::string& path, const std::string& displayName);

	// スクリプトフォルダの .lua を集める
	std::vector<std::string> CollectScriptFiles() const;

	//----------------------------------------------------------------
	// 補助
	//----------------------------------------------------------------

	BehaviorNodeAsset* GetSelectedNode();
	Model* GetBossModel(EnemyBoss* boss) const;

	// そのモデルのアニメーション名一覧
	std::vector<std::string> CollectAnimationNames(Model* model) const;

	// clip が指すアニメーションの設定（無ければ nullptr）
	AnimationConfig* FindAnimationConfig(Model* model, const std::string& clipName) const;

	void SetStatus(const std::string& message, bool isError);

private:
	ax::NodeEditor::EditorContext* graphContext = nullptr;

	// 編集中のツリー。ボスが実際に使っているものとは別物。
	BehaviorTreeAsset	asset;
	bool				assetLoaded = false;

	// グラフへノード位置を流し込むのを待っている状態
	bool				pendingLayout = false;

	int					selectedNodeId = -1;
	int					selectedClipIndex = 0;

	// 「保存されていない変更がある」ことの目印
	bool				dirty = false;

	std::vector<std::string> problems;

	// ステータス行
	std::string			statusMessage;
	bool				statusIsError = false;
	float				statusTimer = 0.0f;

	//----------------------------------------------------------------
	// スクリプト編集
	//----------------------------------------------------------------

	std::string			scriptPath;
	std::vector<char>	scriptBuffer;
	bool				scriptDirty = false;

	// 新規スクリプト名の入力欄
	std::vector<char>	newScriptName;

	//----------------------------------------------------------------
	// ホットリロード
	//----------------------------------------------------------------

	bool				hotReloadEnabled = true;
	float				hotReloadTimer = 0.0f;

	// ツリー JSON を外部で編集したときも拾えるようにする
	bool				watchTreeFile = true;
	long long			treeFileWriteTime = 0;

	bool				isOpen = true;
};
