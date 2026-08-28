#include "LuaAction.h"

#include "LuaBossBindings.h"
#include "LuaScriptSystem.h"
#include "Character/Projectile/Projectile.h"

namespace
{
	// self に無いキーを、スクリプト本体（現在読み込まれている版）から引く。
	//
	// これで self:OnReset() のようにスクリプト自身の関数を呼べるようになる。
	// パスから毎回引き直しているので、ホットリロード後も新しい実装につながる。
	int SelfIndex(lua_State* L)
	{
		const char* path = lua_tostring(L, lua_upvalueindex(1));
		if (path == nullptr)
		{
			lua_pushnil(L);
			return 1;
		}

		if (!LuaScriptSystem::Instance().PushScriptTable(path))
		{
			lua_pushnil(L);
			return 1;
		}

		lua_pushvalue(L, 2);	// キー
		lua_gettable(L, -2);
		return 1;
	}

	// 行動の戻り値の文字列を State へ変換する
	ActionBase<EnemyBoss>::State ParseResultState(const char* text)
	{
		using State = ActionBase<EnemyBoss>::State;

		if (text == nullptr) return State::Run;

		const std::string result(text);
		if (result == "complete") return State::Complete;
		if (result == "failed")   return State::Failed;
		return State::Run;
	}
}

LuaAction::LuaAction(EnemyBoss* actor, const BehaviorNodeAsset& node)
	: EnemyActionBase<EnemyBoss>(actor)
	, nodeAsset(node)
	, selfRef(LUA_NOREF)
{
	// Boss.* が使える状態にしてからスクリプトを読む
	LuaBossBindings::EnsureRegistered();

	LuaScriptSystem& lua = LuaScriptSystem::Instance();

	if (!nodeAsset.actionScript.empty())
	{
		lua.LoadScript(nodeAsset.actionScript);
	}

	CreateSelfTable();
}

LuaAction::~LuaAction()
{
	LuaScriptSystem& lua = LuaScriptSystem::Instance();

	// lua_State がすでに閉じられている場合は触らない
	if (selfRef != LUA_NOREF && lua.IsReady())
	{
		luaL_unref(lua.GetState(), LUA_REGISTRYINDEX, selfRef);
	}
	selfRef = LUA_NOREF;
}

void LuaAction::CreateSelfTable()
{
	LuaScriptSystem& lua = LuaScriptSystem::Instance();
	if (!lua.IsReady()) return;

	lua_State* L = lua.GetState();

	lua_newtable(L);	// self

	// self.node
	lua_newtable(L);

	lua_pushstring(L, nodeAsset.name.c_str());
	lua_setfield(L, -2, "name");

	lua_pushinteger(L, nodeAsset.id);
	lua_setfield(L, -2, "id");

	// self.node.params
	lua_newtable(L);
	for (const BehaviorParam& param : nodeAsset.params)
	{
		switch (param.type)
		{
		case BehaviorParam::Type::Float:  lua_pushnumber(L, param.floatValue); break;
		case BehaviorParam::Type::Int:    lua_pushinteger(L, param.intValue); break;
		case BehaviorParam::Type::Bool:   lua_pushboolean(L, param.boolValue ? 1 : 0); break;
		case BehaviorParam::Type::String: lua_pushstring(L, param.stringValue.c_str()); break;
		default:                          lua_pushnil(L); break;
		}
		lua_setfield(L, -2, param.name.c_str());
	}
	lua_setfield(L, -2, "params");

	// self.node.clips（Lua に合わせて 1 始まりの配列にする）
	lua_newtable(L);
	for (size_t i = 0; i < nodeAsset.clips.size(); ++i)
	{
		const BehaviorClip& clip = nodeAsset.clips[i];

		lua_newtable(L);
		lua_pushstring(L, clip.clip.c_str());			lua_setfield(L, -2, "clip");
		lua_pushboolean(L, clip.loop ? 1 : 0);			lua_setfield(L, -2, "loop");
		lua_pushboolean(L, clip.rootMotion ? 1 : 0);	lua_setfield(L, -2, "rootMotion");
		lua_pushnumber(L, clip.blendSeconds);			lua_setfield(L, -2, "blendSeconds");
		lua_pushnumber(L, clip.speedScale);				lua_setfield(L, -2, "speedScale");

		lua_rawseti(L, -2, static_cast<lua_Integer>(i) + 1);
	}
	lua_setfield(L, -2, "clips");

	lua_pushinteger(L, static_cast<lua_Integer>(nodeAsset.clips.size()));
	lua_setfield(L, -2, "clipCount");

	lua_setfield(L, -2, "node");	// self.node = ...

	// self からスクリプト本体の関数を辿れるようにする
	lua_newtable(L);
	lua_pushstring(L, nodeAsset.actionScript.c_str());
	lua_pushcclosure(L, SelfIndex, 1);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);

	// self をレジストリに保持する
	selfRef = luaL_ref(L, LUA_REGISTRYINDEX);
}

bool LuaAction::PushSelfTable()
{
	LuaScriptSystem& lua = LuaScriptSystem::Instance();
	if (!lua.IsReady()) return false;

	if (selfRef == LUA_NOREF)
	{
		CreateSelfTable();
		if (selfRef == LUA_NOREF) return false;
	}

	lua_State* L = lua.GetState();
	if (lua_rawgeti(L, LUA_REGISTRYINDEX, selfRef) != LUA_TTABLE)
	{
		lua_pop(L, 1);
		return false;
	}
	return true;
}

void LuaAction::ResolveClipIndices()
{
	clipAnimationIndices.clear();
	clipAnimationIndices.reserve(nodeAsset.clips.size());

	Model* model = GetModel();
	for (const BehaviorClip& clip : nodeAsset.clips)
	{
		clipAnimationIndices.emplace_back(
			(model != nullptr && !clip.clip.empty())
			? model->GetAnimationIndex(clip.clip.c_str())
			: -1);
	}

	clipsResolved = true;
}

int LuaAction::GetClipAnimationIndex(int index) const
{
	if (index < 0 || index >= static_cast<int>(clipAnimationIndices.size())) return -1;
	return clipAnimationIndices[index];
}

bool LuaAction::PlayClip(int index)
{
	if (!clipsResolved) ResolveClipIndices();

	if (index < 0 || index >= static_cast<int>(nodeAsset.clips.size())) return false;

	const int animationIndex = GetClipAnimationIndex(index);
	if (animationIndex < 0) return false;

	Model* model = GetModel();
	if (model == nullptr) return false;

	const BehaviorClip& clip = nodeAsset.clips[index];

	if (clip.rootMotion)
	{
		PlayRootMotion(animationIndex, clip.loop, clip.blendSeconds);
	}
	else
	{
		model->PlayAnimation(animationIndex, clip.loop, clip.blendSeconds);
	}

	// 速度カーブに潰されない側の乗数へ掛ける
	model->SetBaseAnimationSpeed(clip.speedScale);
	return true;
}

float LuaAction::ResolveCancelStart(int index) const
{
	if (index < 0 || index >= static_cast<int>(nodeAsset.clips.size())) return 0.0f;

	const BehaviorClip& clip = nodeAsset.clips[index];
	if (clip.cancelStart >= 0.0f) return clip.cancelStart;

	// ノード側で指定されていなければ、アニメーション共通の設定を使う
	const int animationIndex = GetClipAnimationIndex(index);
	if (animationIndex >= 0)
	{
		if (AnimationConfig* config = GetAnimationConfig(animationIndex))
		{
			return config->advanceInputStartFrame;
		}
	}
	return 0.0f;
}

float LuaAction::ResolveCancelEnd(int index) const
{
	if (index < 0 || index >= static_cast<int>(nodeAsset.clips.size())) return 0.0f;

	const BehaviorClip& clip = nodeAsset.clips[index];
	if (clip.cancelEnd >= 0.0f) return clip.cancelEnd;

	const int animationIndex = GetClipAnimationIndex(index);
	if (animationIndex >= 0)
	{
		if (AnimationConfig* config = GetAnimationConfig(animationIndex))
		{
			return config->advanceInputEndFrame;
		}
	}
	return 0.0f;
}

bool LuaAction::CallScriptFunction(const char* name, int extraArgCount, int resultCount)
{
	LuaScriptSystem& lua = LuaScriptSystem::Instance();
	if (!lua.IsReady()) return false;

	lua_State* L = lua.GetState();

	// 追加引数は呼び出し側がすでに積んでいる。関数と self はその下へ差し込む。
	const int insertAt = lua_gettop(L) - extraArgCount + 1;

	if (!lua.PushScriptTable(nodeAsset.actionScript))
	{
		if (!scriptMissingReported)
		{
			lua.Log(LuaScriptSystem::LogEntry::Level::Error,
				"スクリプトが読み込まれていません: " + nodeAsset.actionScript);
			scriptMissingReported = true;
		}
		return false;
	}

	if (lua_getfield(L, -1, name) != LUA_TFUNCTION)
	{
		lua_pop(L, 2);	// 取り出した値とスクリプトテーブル
		return false;
	}

	lua_remove(L, -2);	// スクリプトテーブルはもう要らない

	if (!PushSelfTable())
	{
		lua_pop(L, 1);	// 関数
		return false;
	}

	// 現在 [args...][func][self] の順。これを [func][self][args...] に直す。
	lua_insert(L, insertAt);	// self
	lua_insert(L, insertAt);	// func

	return lua.ProtectedCall(extraArgCount + 1, resultCount,
		nodeAsset.actionScript + ":" + name + "  (node: " + nodeAsset.name + ")");
}

LuaAction::State LuaAction::Run(float elapsedTime)
{
	LuaScriptSystem& lua = LuaScriptSystem::Instance();
	if (!lua.IsReady()) return State::Failed;

	if (nodeAsset.actionScript.empty()) return State::Failed;

	// スクリプトが壊れている間は行動を成立させない。
	// 直せば次のフレームで読み直されるので、その場で復帰できる。
	if (lua.HasError(nodeAsset.actionScript))
	{
		this->step = 0;
		needsResetAfterError = true;
		return State::Failed;
	}

	// 壊れていた間に中断した場合、スクリプト側の self は途中のままになっている。
	// 直ったこのタイミングで初期化してから動かす。
	if (needsResetAfterError)
	{
		needsResetAfterError = false;
		OnReset();
	}

	if (!clipsResolved) ResolveClipIndices();

	lua_State* L = lua.GetState();

	// スクリプトから Boss.* を呼べるように、今動いている行動を教えておく
	LuaBossBindings::SetContext(this);

	lua_pushnumber(L, elapsedTime);
	const bool called = CallScriptFunction("Run", 1, 1);

	LuaBossBindings::ClearContext();

	if (!called)
	{
		// 呼び出せなかった＝スクリプトが壊れている。行動としては失敗にする。
		return ResetState(State::Failed);
	}

	const State state = lua_isstring(L, -1)
		? ParseResultState(lua_tostring(L, -1))
		: State::Run;
	lua_pop(L, 1);

	if (state != State::Run) return ResetState(state);

	return State::Run;
}

void LuaAction::OnReset()
{
	storedProjectiles.clear();

	// この行動のために掛けていた再生速度を戻す。
	// 残したままだと、次に動く別の行動まで巻き添えになる。
	if (Model* model = GetModel()) model->SetBaseAnimationSpeed(1.0f);

	LuaScriptSystem& lua = LuaScriptSystem::Instance();
	if (!lua.IsReady()) return;
	if (lua.HasError(nodeAsset.actionScript)) return;

	LuaBossBindings::SetContext(this);
	CallScriptFunction("OnReset", 0, 0);
	LuaBossBindings::ClearContext();
}
