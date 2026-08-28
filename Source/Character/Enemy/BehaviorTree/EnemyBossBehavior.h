#pragma once

#include <string>
#include <vector>

#include "BehaviorTreeAsset.h"

class EnemyBoss;

template <typename ActorType>
class BehaviorTree;

// ---------------------------------------------------------------------------
// EnemyBoss のビヘイビアツリーまわりの入り口。
//
// ・C++ で書かれた行動／判定クラスをレジストリへ登録する
// ・JSON（BehaviorTreeAsset）から実行時ツリーを組み立てる
// ・JSON が無いときのために、これまでハードコードしていた構成を返す
// ---------------------------------------------------------------------------
namespace EnemyBossBehavior
{
	// ツリー定義の保存先
	const char* GetTreeAssetPath();

	// Lua 行動スクリプトを置くフォルダ
	const char* GetScriptDirectory();

	// 行動・判定クラスをレジストリに登録する。二度呼んでも安全。
	void RegisterBehaviors();

	// これまで EnemyBoss.cpp に直接書かれていたツリーと同じ構成を返す。
	// JSON がまだ無いときの初期値であり、書き出しの元にもなる。
	BehaviorTreeAsset MakeDefaultTreeAsset();

	// アセットからツリーを組み立てる。
	// 失敗したら false を返し、problems に理由が入る（ツリーは組み立てられない）。
	bool BuildTree(
		BehaviorTree<EnemyBoss>& tree,
		EnemyBoss* owner,
		const BehaviorTreeAsset& asset,
		std::vector<std::string>& problems);

	// アセットが参照している Lua スクリプトをまとめて読み込む
	void PreloadScripts(const BehaviorTreeAsset& asset);
}
