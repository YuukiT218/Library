#pragma once

#include "lua.h"

class LuaAction;

// ---------------------------------------------------------------------------
// EnemyBoss の操作を Lua から呼べるようにするバインディング。
//
// スクリプト側にはグローバルの Boss テーブルとして見える。
//
//   function M:Run(dt)
//       Boss.SetTargetToPlayer()
//       Boss.TurnToTarget(dt, Boss.TURN_INSTANT)
//       if self.step == 0 then
//           Boss.PlayClip(1)
//           self.step = 1
//       end
//       ...
//   end
//
// 呼び出し中はどの行動が動いているかを SetContext で教えてもらい、
// 各関数はそこから owner（EnemyBoss）とノード設定を辿る。
// コンテキストが無いまま Boss.* を呼ぶと Lua のエラーになる。
// ---------------------------------------------------------------------------
class LuaBossBindings
{
public:
	// Lua の初期化と Boss テーブルの登録を必要なぶんだけ行う。
	// 何度呼んでも二重登録にならないので、Lua を使う手前で気軽に呼んでよい。
	static void EnsureRegistered();

	// グローバル Boss テーブルを作る。通常は EnsureRegistered から呼ばれる。
	static void Register(lua_State* L);

	// これから Lua を呼ぶ行動を登録する
	static void SetContext(LuaAction* action);

	// 呼び出しが終わったら必ず解除する
	static void ClearContext();

	static LuaAction* GetContext();
};
