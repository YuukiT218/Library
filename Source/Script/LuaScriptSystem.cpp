#include "LuaScriptSystem.h"

#include <system_error>

namespace fs = std::filesystem;

namespace
{
	// エラー時にトレースバックを付け足すためのメッセージハンドラ
	int TracebackHandler(lua_State* L)
	{
		const char* message = lua_tostring(L, 1);
		if (message == nullptr)
		{
			// エラー値が文字列でない場合は __tostring を試す
			if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
			{
				return 1;
			}
			message = lua_pushfstring(L, "(エラー値が文字列ではありません: %s)", luaL_typename(L, 1));
		}
		luaL_traceback(L, L, message, 1);
		return 1;
	}
}

LuaScriptSystem& LuaScriptSystem::Instance()
{
	static LuaScriptSystem instance;
	return instance;
}

LuaScriptSystem::~LuaScriptSystem()
{
	Finalize();
}

void LuaScriptSystem::Initialize()
{
	if (state != nullptr) return;

	state = luaL_newstate();
	if (state == nullptr)
	{
		Log(LogEntry::Level::Error, "lua_State の作成に失敗しました");
		return;
	}

	luaL_openlibs(state);

	// スクリプトテーブルの置き場をレジストリに用意する
	lua_newtable(state);
	lua_setfield(state, LUA_REGISTRYINDEX, SCRIPT_REGISTRY_KEY);

	Log(LogEntry::Level::Info, "Lua " LUA_VERSION_MAJOR "." LUA_VERSION_MINOR " を初期化しました");
}

void LuaScriptSystem::Finalize()
{
	if (state == nullptr) return;

	lua_close(state);
	state = nullptr;
	scripts.clear();
}

void LuaScriptSystem::PushScriptRegistry()
{
	lua_getfield(state, LUA_REGISTRYINDEX, SCRIPT_REGISTRY_KEY);
}

bool LuaScriptSystem::LoadScript(const std::string& path)
{
	const auto it = scripts.find(path);
	if (it != scripts.end() && it->second.valid) return true;

	return LoadScriptInternal(path, false);
}

bool LuaScriptSystem::ReloadScript(const std::string& path)
{
	return LoadScriptInternal(path, true);
}

bool LuaScriptSystem::LoadScriptInternal(const std::string& path, bool /*reload*/)
{
	if (state == nullptr)
	{
		Log(LogEntry::Level::Error, "Lua が初期化されていません: " + path);
		return false;
	}

	ScriptRecord& record = scripts[path];

	// 読み込みの成否に関わらず、次回の更新検出が正しく働くよう時刻を控えておく
	std::error_code ec;
	const fs::file_time_type writeTime = fs::last_write_time(path, ec);
	if (!ec) record.writeTime = writeTime;

	const int stackTop = lua_gettop(state);

	// チャンクをコンパイルする
	if (luaL_loadfile(state, path.c_str()) != LUA_OK)
	{
		record.valid = false;
		record.lastError = lua_tostring(state, -1) ? lua_tostring(state, -1) : "不明なコンパイルエラー";
		Log(LogEntry::Level::Error, "コンパイル失敗 " + path + "\n" + record.lastError);
		lua_settop(state, stackTop);
		return false;
	}

	// チャンクを実行してテーブルを受け取る
	if (!ProtectedCall(0, 1, path))
	{
		record.valid = false;
		record.lastError = log.empty() ? "実行時エラー" : log.back().message;
		lua_settop(state, stackTop);
		return false;
	}

	if (!lua_istable(state, -1))
	{
		record.valid = false;
		record.lastError = "スクリプトはテーブルを return する必要があります";
		Log(LogEntry::Level::Error, path + ": " + record.lastError);
		lua_settop(state, stackTop);
		return false;
	}

	// レジストリ[path] = 受け取ったテーブル
	PushScriptRegistry();		// [table][registry]
	lua_pushvalue(state, -2);	// [table][registry][table]
	lua_setfield(state, -2, path.c_str());
	lua_settop(state, stackTop);

	record.valid = true;
	record.lastError.clear();
	Log(LogEntry::Level::Info, "読み込み成功: " + path);
	return true;
}

bool LuaScriptSystem::PushScriptTable(const std::string& path)
{
	if (state == nullptr) return false;

	PushScriptRegistry();
	if (lua_getfield(state, -1, path.c_str()) != LUA_TTABLE)
	{
		lua_pop(state, 2);	// 取り出した値とレジストリ
		return false;
	}

	// [registry][table] -> [table]
	lua_remove(state, -2);
	return true;
}

std::vector<std::string> LuaScriptSystem::HotReload()
{
	std::vector<std::string> reloaded;
	if (state == nullptr) return reloaded;

	// 走査中に scripts が書き変わるので、パスを先に集める
	std::vector<std::string> paths;
	paths.reserve(scripts.size());
	for (const auto& entry : scripts)
	{
		paths.emplace_back(entry.first);
	}

	for (const std::string& path : paths)
	{
		std::error_code ec;
		const fs::file_time_type writeTime = fs::last_write_time(path, ec);
		if (ec) continue;	// 保存中などで一時的に読めないことがあるので次のフレームに回す

		if (writeTime == scripts[path].writeTime) continue;

		if (LoadScriptInternal(path, true))
		{
			reloaded.emplace_back(path);
		}
	}

	return reloaded;
}

std::vector<std::string> LuaScriptSystem::GetLoadedScriptPaths() const
{
	std::vector<std::string> paths;
	paths.reserve(scripts.size());
	for (const auto& entry : scripts)
	{
		paths.emplace_back(entry.first);
	}
	return paths;
}

bool LuaScriptSystem::HasError(const std::string& path) const
{
	const auto it = scripts.find(path);
	return it != scripts.end() && !it->second.valid;
}

std::string LuaScriptSystem::GetError(const std::string& path) const
{
	const auto it = scripts.find(path);
	return it == scripts.end() ? std::string() : it->second.lastError;
}

bool LuaScriptSystem::ProtectedCall(int argCount, int resultCount, const std::string& context)
{
	if (state == nullptr) return false;

	// 関数と引数の下にメッセージハンドラを差し込む
	const int handlerIndex = lua_gettop(state) - argCount;
	lua_pushcfunction(state, TracebackHandler);
	lua_insert(state, handlerIndex);

	const int result = lua_pcall(state, argCount, resultCount, handlerIndex);

	// ハンドラを取り除く
	lua_remove(state, handlerIndex);

	if (result != LUA_OK)
	{
		const char* message = lua_tostring(state, -1);
		Log(LogEntry::Level::Error,
			context + "\n" + (message != nullptr ? message : "不明なエラー"));
		lua_pop(state, 1);
		return false;
	}

	return true;
}

void LuaScriptSystem::Log(LogEntry::Level level, const std::string& message)
{
	log.push_back({ level, message });

	if (log.size() > MAX_LOG_ENTRIES)
	{
		log.erase(log.begin(), log.begin() + (log.size() - MAX_LOG_ENTRIES));
	}
}
