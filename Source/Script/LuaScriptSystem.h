#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

// Lua 本体は C++ としてコンパイルしている（ldo.c が __cplusplus を見て
// エラー処理を longjmp ではなく C++ 例外にする）。
// そのため extern "C" で包む lua.hpp は使わず、ヘッダを直接取り込む。
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

// ---------------------------------------------------------------------------
// Lua スクリプトの読み込み・ホットリロード・エラー収集をまとめて面倒を見る。
//
// スクリプトは「テーブルを 1 つ return するファイル」として書く決まりにしている。
//
//   local M = {}
//   function M:Run(dt) ... return "complete" end
//   return M
//
// 読み込んだテーブルはパスをキーにして Lua 側に保持され、
// ファイルが更新されると次のフレームで差し替わる（ホットリロード）。
// ---------------------------------------------------------------------------
class LuaScriptSystem
{
public:
	// エディタに出すログ 1 行
	struct LogEntry
	{
		enum class Level { Info, Warning, Error };

		Level		level = Level::Info;
		std::string	message;
	};

	static LuaScriptSystem& Instance();

	// コピー禁止
	LuaScriptSystem(const LuaScriptSystem&) = delete;
	void operator=(const LuaScriptSystem&) = delete;

	// lua_State を作って標準ライブラリを開く。二度呼んでも安全。
	void Initialize();

	// lua_State を破棄する
	void Finalize();

	lua_State* GetState() const { return state; }
	bool IsReady() const { return state != nullptr; }

	//----------------------------------------------------------------
	// スクリプト
	//----------------------------------------------------------------

	// スクリプトを読み込む（すでに読み込み済みなら何もしない）。
	// 成功したら true。失敗理由はログに積まれる。
	bool LoadScript(const std::string& path);

	// ファイルの更新時刻に関係なく読み込み直す
	bool ReloadScript(const std::string& path);

	// 読み込み済みのスクリプトテーブルをスタックに積む。
	// 積めなかった場合は false（スタックは変化しない）。
	bool PushScriptTable(const std::string& path);

	// 読み込み済みのスクリプトのうち、ファイルが更新されたものを読み込み直す。
	// 実際に読み直したパスを返す。
	std::vector<std::string> HotReload();

	// 読み込み済みスクリプトのパス一覧（エディタのファイル一覧用）
	std::vector<std::string> GetLoadedScriptPaths() const;

	// スクリプトが読み込みに失敗した状態かどうか
	bool HasError(const std::string& path) const;

	// そのスクリプトの最後のエラー文（無ければ空）
	std::string GetError(const std::string& path) const;

	//----------------------------------------------------------------
	// 呼び出し
	//----------------------------------------------------------------

	// スタックに積まれた関数と引数を、トレースバック付きで呼ぶ。
	// 失敗したら false を返してログにエラーを積む。
	// context はログに出す呼び出し元の説明（"SlashCombo.lua:Run" など）。
	bool ProtectedCall(int argCount, int resultCount, const std::string& context);

	//----------------------------------------------------------------
	// ログ
	//----------------------------------------------------------------

	void Log(LogEntry::Level level, const std::string& message);
	const std::vector<LogEntry>& GetLog() const { return log; }
	void ClearLog() { log.clear(); }

private:
	LuaScriptSystem() = default;
	~LuaScriptSystem();

	// 読み込みの実体。reload が true なら更新時刻を見ずに読み直す。
	bool LoadScriptInternal(const std::string& path, bool reload);

	// スクリプトテーブルを保持しているレジストリ用テーブルをスタックに積む
	void PushScriptRegistry();

	struct ScriptRecord
	{
		std::filesystem::file_time_type	writeTime{};
		bool							valid = false;
		std::string						lastError;
	};

	lua_State* state = nullptr;
	std::unordered_map<std::string, ScriptRecord> scripts;
	std::vector<LogEntry> log;

	// ログが際限なく伸びないように上限を設ける
	static constexpr size_t MAX_LOG_ENTRIES = 200;

	// Lua レジストリ内でスクリプトテーブル置き場に使うキー
	static constexpr const char* SCRIPT_REGISTRY_KEY = "BehaviorScripts";
};
