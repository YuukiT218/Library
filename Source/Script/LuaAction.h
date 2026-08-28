#pragma once

#include <string>
#include <vector>

#include "Character/Enemy/BehaviorTree/Actions/EnemyActionBase.h"
#include "Character/Enemy/BehaviorTree/BehaviorTreeAsset.h"
#include "Character/Enemy/EnemyBoss.h"

class Projectile;

template <typename ActorType>
class BehaviorData;

// ---------------------------------------------------------------------------
// Lua スクリプトで中身を書く行動クラス。
//
// スクリプト側は次の形のテーブルを return する。
//
//   local M = {}
//   function M:Run(dt)
//       -- self は行動インスタンスごとに用意される。self.step などは自由に使える
//       -- self.node.params / self.node.clips でエディタの設定値を読める
//       return "complete"   -- "run"(省略可) / "complete" / "failed"
//   end
//   function M:OnReset() end   -- 任意。行動が終わって片付けるときに呼ばれる
//   return M
//
// スクリプトが差し替わっても self テーブルはそのまま残るので、
// ホットリロードしても行動の途中経過は失われない。
// ---------------------------------------------------------------------------
class LuaAction : public EnemyActionBase<EnemyBoss>
{
public:
	LuaAction(EnemyBoss* actor, const BehaviorNodeAsset& nodeAsset);
	~LuaAction() override;

	// コピーすると Lua 側の参照が二重解放になるので禁止
	LuaAction(const LuaAction&) = delete;
	void operator=(const LuaAction&) = delete;

	State Run(float elapsedTime) override;

	//----------------------------------------------------------------
	// バインディング（LuaBossBindings）から使う
	//----------------------------------------------------------------

	const BehaviorNodeAsset& GetNodeAsset() const { return nodeAsset; }

	// clips[index] を再生する。index は 0 始まり。
	// 範囲外なら false。
	bool PlayClip(int index);

	// clips[index] のキャンセル受付開始 / 終了時間（秒）を解決して返す。
	// ノード側が負値なら、そのアニメーションの AnimationConfig の値を使う。
	float ResolveCancelStart(int index) const;
	float ResolveCancelEnd(int index) const;

	// clips[index] のアニメーション番号。未解決なら -1。
	int GetClipAnimationIndex(int index) const;

	int GetClipCount() const { return static_cast<int>(nodeAsset.clips.size()); }

	// スクリプトから生成した発射物の置き場。まとめて撃つのに使う。
	std::vector<Projectile*>& GetStoredProjectiles() { return storedProjectiles; }

	// EnemyActionBase の protected な処理をバインディングへ開放する
	using EnemyActionBase<EnemyBoss>::GetModel;
	using EnemyActionBase<EnemyBoss>::GetAnimationFrame;
	using EnemyActionBase<EnemyBoss>::GetCurrentAnimationConfig;
	using EnemyActionBase<EnemyBoss>::GetAnimationConfig;
	using EnemyActionBase<EnemyBoss>::PlayRootMotion;
	using EnemyActionBase<EnemyBoss>::UpdateAttackCollision;
	using EnemyActionBase<EnemyBoss>::UpdateTargetingAndAttack;
	using EnemyActionBase<EnemyBoss>::IsNearPlayer;
	using EnemyActionBase<EnemyBoss>::LerpTowardPlayer;
	using EnemyActionBase<EnemyBoss>::IsHealthBelowRate;
	using EnemyActionBase<EnemyBoss>::IsInterrupted;
	using EnemyActionBase<EnemyBoss>::PlayerPosition;
	using EnemyActionBase<EnemyBoss>::FireAndClear;
	using EnemyActionBase<EnemyBoss>::SpawnLightPillarCircle;
	using EnemyActionBase<EnemyBoss>::SpawnSideLightPillars;

	EnemyBoss* GetOwner() const { return this->owner; }

	// 実行中のシーケンス情報を見るために BehaviorTree 側のデータを開放する
	BehaviorData<EnemyBoss>* GetBehaviorData() const { return this->behaviorData; }

private:
	void OnReset() override;

	// self テーブルを作り、node（name / params / clips）を流し込む
	void CreateSelfTable();

	// self テーブルをスタックに積む。積めなければ false。
	bool PushSelfTable();

	// スクリプトの関数 name を self 付きで呼ぶ。
	// 定義されていなければ何もせず false を返す（エラー扱いにはしない）。
	bool CallScriptFunction(const char* name, int extraArgCount, int resultCount);

	// clips に書かれたアニメーション名を番号へ解決する（モデル読み込み後に一度だけ）
	void ResolveClipIndices();

	BehaviorNodeAsset			nodeAsset;
	std::vector<int>			clipAnimationIndices;
	std::vector<Projectile*>	storedProjectiles;

	// Lua レジストリ内の self テーブルへの参照（LUA_NOREF なら未作成）
	int							selfRef;

	bool						clipsResolved = false;
	bool						scriptMissingReported = false;

	// スクリプトが壊れていた間に中断したので、直り次第 self を初期化し直す
	bool						needsResetAfterError = false;
};
