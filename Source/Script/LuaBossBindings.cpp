#include "LuaBossBindings.h"

#include <cmath>
#include <string>

#include "lauxlib.h"

#include "LuaAction.h"
#include "LuaScriptSystem.h"

#include "Character/Enemy/BehaviorTree/ActionBase.h"
#include "Character/Enemy/BehaviorTree/BehaviorData.h"
#include "Character/Enemy/BehaviorTree/NodeBase.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Character/Player/Player.h"
#include "Character/Projectile/Projectile.h"
#include "Character/Projectile/ProjectileManager.h"
#include "Effect/Effect.h"
#include "Math/Mathf.h"

using namespace DirectX;

namespace
{
	// 今 Lua を実行している行動。Lua の呼び出しは 1 本ずつなので単純な変数で足りる。
	LuaAction* currentAction = nullptr;

	//----------------------------------------------------------------
	// 取り出しヘルパー
	//----------------------------------------------------------------

	LuaAction* RequireAction(lua_State* L)
	{
		if (currentAction == nullptr)
		{
			luaL_error(L, "Boss API は行動スクリプトの実行中しか呼べません");
			return nullptr;
		}
		return currentAction;
	}

	EnemyBoss* RequireBoss(lua_State* L)
	{
		LuaAction* action = RequireAction(L);
		EnemyBoss* boss = (action != nullptr) ? action->GetOwner() : nullptr;
		if (boss == nullptr)
		{
			luaL_error(L, "行動に EnemyBoss が結び付いていません");
			return nullptr;
		}
		return boss;
	}

	Model* RequireModel(lua_State* L)
	{
		LuaAction* action = RequireAction(L);
		Model* model = (action != nullptr) ? action->GetModel() : nullptr;
		if (model == nullptr)
		{
			luaL_error(L, "モデルが読み込まれていません");
			return nullptr;
		}
		return model;
	}

	// Lua は 1 始まりなので、C++ の添字へ直して渡す
	int ToClipIndex(lua_State* L, int argIndex)
	{
		return static_cast<int>(luaL_checkinteger(L, argIndex)) - 1;
	}

	// アニメーション番号を、番号でも名前でも受け取れるようにする
	int ToAnimationIndex(lua_State* L, int argIndex)
	{
		if (lua_type(L, argIndex) == LUA_TSTRING)
		{
			Model* model = RequireModel(L);
			return model->GetAnimationIndex(lua_tostring(L, argIndex));
		}
		return static_cast<int>(luaL_checkinteger(L, argIndex));
	}

	float OptFloat(lua_State* L, int argIndex, float defaultValue)
	{
		return static_cast<float>(luaL_optnumber(L, argIndex, defaultValue));
	}

	bool OptBool(lua_State* L, int argIndex, bool defaultValue)
	{
		if (lua_isnoneornil(L, argIndex)) return defaultValue;
		return lua_toboolean(L, argIndex) != 0;
	}

	//----------------------------------------------------------------
	// テーブル引数の読み取り
	//----------------------------------------------------------------

	float FieldFloat(lua_State* L, int tableIndex, const char* key, float defaultValue)
	{
		float value = defaultValue;
		if (lua_getfield(L, tableIndex, key) == LUA_TNUMBER)
		{
			value = static_cast<float>(lua_tonumber(L, -1));
		}
		lua_pop(L, 1);
		return value;
	}

	int FieldInt(lua_State* L, int tableIndex, const char* key, int defaultValue)
	{
		int value = defaultValue;
		if (lua_getfield(L, tableIndex, key) == LUA_TNUMBER)
		{
			value = static_cast<int>(lua_tointeger(L, -1));
		}
		lua_pop(L, 1);
		return value;
	}

	bool FieldBool(lua_State* L, int tableIndex, const char* key, bool defaultValue)
	{
		bool value = defaultValue;
		if (lua_getfield(L, tableIndex, key) != LUA_TNIL)
		{
			value = lua_toboolean(L, -1) != 0;
		}
		lua_pop(L, 1);
		return value;
	}

	int PushVector3(lua_State* L, const XMFLOAT3& value)
	{
		lua_pushnumber(L, value.x);
		lua_pushnumber(L, value.y);
		lua_pushnumber(L, value.z);
		return 3;
	}

	//----------------------------------------------------------------
	// アニメーション
	//----------------------------------------------------------------

	int Boss_GetAnimationIndex(lua_State* L)
	{
		Model* model = RequireModel(L);
		lua_pushinteger(L, model->GetAnimationIndex(luaL_checkstring(L, 1)));
		return 1;
	}

	int Boss_PlayAnimation(lua_State* L)
	{
		LuaAction* action = RequireAction(L);
		Model* model = RequireModel(L);

		const int animationIndex = ToAnimationIndex(L, 1);
		const bool loop = OptBool(L, 2, false);
		const float blend = OptFloat(L, 3, action->GetOwner()->GetBlendSeconds());

		model->PlayAnimation(animationIndex, loop, blend);
		return 0;
	}

	int Boss_PlayRootMotion(lua_State* L)
	{
		LuaAction* action = RequireAction(L);

		const int animationIndex = ToAnimationIndex(L, 1);
		const bool loop = OptBool(L, 2, false);
		const float blend = OptFloat(L, 3, action->GetOwner()->GetBlendSeconds());

		action->PlayRootMotion(animationIndex, loop, blend);
		return 0;
	}

	// エディタで設定したクリップを再生する（ループ・ブレンド・速度倍率も一緒に反映される）
	int Boss_PlayClip(lua_State* L)
	{
		LuaAction* action = RequireAction(L);
		lua_pushboolean(L, action->PlayClip(ToClipIndex(L, 1)) ? 1 : 0);
		return 1;
	}

	int Boss_GetClipCount(lua_State* L)
	{
		lua_pushinteger(L, RequireAction(L)->GetClipCount());
		return 1;
	}

	int Boss_GetAnimationSeconds(lua_State* L)
	{
		lua_pushnumber(L, RequireAction(L)->GetAnimationFrame());
		return 1;
	}

	int Boss_IsPlayingAnimation(lua_State* L)
	{
		lua_pushboolean(L, RequireModel(L)->IsPlayAnimation() ? 1 : 0);
		return 1;
	}

	// 速度カーブが animationSpeed を毎フレーム上書きするため、
	// スクリプトからの指定はもう一方の乗数へ掛ける
	int Boss_SetAnimationSpeed(lua_State* L)
	{
		RequireModel(L)->SetBaseAnimationSpeed(OptFloat(L, 1, 1.0f));
		return 0;
	}

	//----------------------------------------------------------------
	// キャンセル受付
	//
	// 連続行動で「今のアニメーションをどこで切って次へ移るか」を決めるための値。
	// ノード側に指定があればそれを、無ければアニメーション共通の設定を返す。
	//----------------------------------------------------------------

	int Boss_GetCancelStart(lua_State* L)
	{
		lua_pushnumber(L, RequireAction(L)->ResolveCancelStart(ToClipIndex(L, 1)));
		return 1;
	}

	int Boss_GetCancelEnd(lua_State* L)
	{
		lua_pushnumber(L, RequireAction(L)->ResolveCancelEnd(ToClipIndex(L, 1)));
		return 1;
	}

	int Boss_IsPastCancelStart(lua_State* L)
	{
		LuaAction* action = RequireAction(L);
		const int clipIndex = ToClipIndex(L, 1);
		lua_pushboolean(L, action->GetAnimationFrame() >= action->ResolveCancelStart(clipIndex) ? 1 : 0);
		return 1;
	}

	int Boss_IsPastCancelEnd(lua_State* L)
	{
		LuaAction* action = RequireAction(L);
		const int clipIndex = ToClipIndex(L, 1);
		lua_pushboolean(L, action->GetAnimationFrame() >= action->ResolveCancelEnd(clipIndex) ? 1 : 0);
		return 1;
	}

	// 今再生しているアニメーション自身の設定値（クリップ番号が分からない場面用）
	int Boss_GetCurrentCancelStart(lua_State* L)
	{
		AnimationConfig* config = RequireAction(L)->GetCurrentAnimationConfig();
		lua_pushnumber(L, config != nullptr ? config->advanceInputStartFrame : 0.0f);
		return 1;
	}

	int Boss_GetCurrentCancelEnd(lua_State* L)
	{
		AnimationConfig* config = RequireAction(L)->GetCurrentAnimationConfig();
		lua_pushnumber(L, config != nullptr ? config->advanceInputEndFrame : 0.0f);
		return 1;
	}

	//----------------------------------------------------------------
	// 位置・向き
	//----------------------------------------------------------------

	int Boss_GetPosition(lua_State* L)
	{
		return PushVector3(L, RequireBoss(L)->GetPosition());
	}

	int Boss_SetPosition(lua_State* L)
	{
		const XMFLOAT3 position{
			static_cast<float>(luaL_checknumber(L, 1)),
			static_cast<float>(luaL_checknumber(L, 2)),
			static_cast<float>(luaL_checknumber(L, 3)) };
		RequireBoss(L)->SetPosition(position);
		return 0;
	}

	int Boss_GetAngle(lua_State* L)
	{
		return PushVector3(L, RequireBoss(L)->GetAngle());
	}

	int Boss_GetPlayerPosition(lua_State* L)
	{
		RequireAction(L);
		return PushVector3(L, Player::Instance().GetPosition());
	}

	int Boss_GetDistanceToPlayer(lua_State* L)
	{
		EnemyBoss* boss = RequireBoss(L);
		const XMFLOAT3 self = boss->GetPosition();
		const XMFLOAT3 player = Player::Instance().GetPosition();

		const float dx = player.x - self.x;
		const float dy = player.y - self.y;
		const float dz = player.z - self.z;

		lua_pushnumber(L, std::sqrt(dx * dx + dy * dy + dz * dz));
		return 1;
	}

	int Boss_SetTargetToPlayer(lua_State* L)
	{
		RequireBoss(L)->SetTargetPosition(Player::Instance().GetPosition());
		return 0;
	}

	int Boss_SetTargetPosition(lua_State* L)
	{
		const XMFLOAT3 position{
			static_cast<float>(luaL_checknumber(L, 1)),
			static_cast<float>(luaL_checknumber(L, 2)),
			static_cast<float>(luaL_checknumber(L, 3)) };
		RequireBoss(L)->SetTargetPosition(position);
		return 0;
	}

	int Boss_TurnToTarget(lua_State* L)
	{
		const float elapsedTime = static_cast<float>(luaL_checknumber(L, 1));
		const float speed = OptFloat(L, 2, TurnSpeed::FAST);
		RequireBoss(L)->TurnToTarget(elapsedTime, speed);
		return 0;
	}

	int Boss_MoveToTarget(lua_State* L)
	{
		const float elapsedTime = static_cast<float>(luaL_checknumber(L, 1));
		const float speedRate = OptFloat(L, 2, 1.0f);
		RequireBoss(L)->MoveToTarget(elapsedTime, speedRate);
		return 0;
	}

	int Boss_IsNearPlayer(lua_State* L)
	{
		const DirectX::SimpleMath::Vector3 epsilon(
			OptFloat(L, 1, 1.0f),
			OptFloat(L, 2, 1.0f),
			OptFloat(L, 3, 1.0f));
		lua_pushboolean(L, RequireAction(L)->IsNearPlayer(epsilon) ? 1 : 0);
		return 1;
	}

	int Boss_LerpTowardPlayer(lua_State* L)
	{
		RequireAction(L)->LerpTowardPlayer(static_cast<float>(luaL_checknumber(L, 1)));
		return 0;
	}

	int Boss_GetForward(lua_State* L)
	{
		EnemyBoss* boss = RequireBoss(L);
		return PushVector3(L, Character::CharacterForward(boss->GetAngle()));
	}

	int Boss_GetBack(lua_State* L)
	{
		EnemyBoss* boss = RequireBoss(L);
		return PushVector3(L, Character::CharacterBack(boss->GetAngle()));
	}

	int Boss_GetLeft(lua_State* L)
	{
		EnemyBoss* boss = RequireBoss(L);
		return PushVector3(L, Character::CharacterLeft(boss->GetAngle()));
	}

	int Boss_GetRight(lua_State* L)
	{
		EnemyBoss* boss = RequireBoss(L);
		return PushVector3(L, Character::CharacterRight(boss->GetAngle()));
	}

	int Boss_SetVerticalVelocity(lua_State* L)
	{
		RequireBoss(L)->SetVerticalVelocity(static_cast<float>(luaL_checknumber(L, 1)));
		return 0;
	}

	int Boss_SetWarpPosition(lua_State* L)
	{
		RequireBoss(L)->SetWarpPosition();
		return 0;
	}

	//----------------------------------------------------------------
	// テレポート
	//----------------------------------------------------------------

	// プレイヤーの周囲 distance の位置から、画面内に入るものを 1 つ選んで返す。
	// 第 2 引数を true にすると高さを自分の位置に合わせる（地面から浮かない）。
	int Boss_CalculateTeleportPosition(lua_State* L)
	{
		EnemyBoss* boss = RequireBoss(L);

		const float distance = OptFloat(L, 1, 3.0f);
		const bool bakeY = OptBool(L, 2, true);

		return PushVector3(L, boss->CalculateVisibleTeleportPos(distance, bakeY));
	}

	// 指定位置へテレポートを開始する。fadeSeconds は消えるまでの時間。
	int Boss_StartTeleport(lua_State* L)
	{
		EnemyBoss* boss = RequireBoss(L);

		const XMFLOAT3 target{
			static_cast<float>(luaL_checknumber(L, 1)),
			static_cast<float>(luaL_checknumber(L, 2)),
			static_cast<float>(luaL_checknumber(L, 3)) };

		boss->StartTeleport(target, OptFloat(L, 4, 0.3f));
		return 0;
	}

	int Boss_IsTeleporting(lua_State* L)
	{
		lua_pushboolean(L, RequireBoss(L)->IsTeleporting() ? 1 : 0);
		return 1;
	}

	//----------------------------------------------------------------
	// 戦闘・状態
	//----------------------------------------------------------------

	int Boss_UpdateAttackCollision(lua_State* L)
	{
		RequireAction(L)->UpdateAttackCollision();
		return 0;
	}

	int Boss_IsInterrupted(lua_State* L)
	{
		lua_pushboolean(L, RequireAction(L)->IsInterrupted() ? 1 : 0);
		return 1;
	}

	int Boss_IsAnyDamage(lua_State* L)
	{
		lua_pushboolean(L, RequireBoss(L)->IsAnyDamage() ? 1 : 0);
		return 1;
	}

	int Boss_IsDead(lua_State* L)
	{
		lua_pushboolean(L, RequireBoss(L)->IsDeathFlag() ? 1 : 0);
		return 1;
	}

	int Boss_GetHealth(lua_State* L)
	{
		lua_pushinteger(L, RequireBoss(L)->GetHealth());
		return 1;
	}

	int Boss_GetMaxHealth(lua_State* L)
	{
		lua_pushinteger(L, RequireBoss(L)->GetMaxHealth());
		return 1;
	}

	int Boss_IsHealthBelowRate(lua_State* L)
	{
		const float rate = static_cast<float>(luaL_checknumber(L, 1));
		lua_pushboolean(L, RequireAction(L)->IsHealthBelowRate(rate) ? 1 : 0);
		return 1;
	}

	int Boss_SetSuperArmor(lua_State* L)
	{
		RequireBoss(L)->SetSuperArmor(OptBool(L, 1, true));
		return 0;
	}

	int Boss_IsSuperArmor(lua_State* L)
	{
		lua_pushboolean(L, RequireBoss(L)->IsSuperArmor() ? 1 : 0);
		return 1;
	}

	int Boss_GetRunTimer(lua_State* L)
	{
		lua_pushnumber(L, RequireBoss(L)->GetRunTimer());
		return 1;
	}

	int Boss_SetRunTimer(lua_State* L)
	{
		RequireBoss(L)->SetRunTimer(static_cast<float>(luaL_checknumber(L, 1)));
		return 0;
	}

	int Boss_GetRevengeValue(lua_State* L)
	{
		lua_pushinteger(L, RequireBoss(L)->GetRevengeValue());
		return 1;
	}

	int Boss_ResetRevengeValue(lua_State* L)
	{
		RequireBoss(L)->ResetRevengeValue();
		return 0;
	}

	int Boss_SetRevengeState(lua_State* L)
	{
		RequireBoss(L)->SetRevengeState(OptBool(L, 1, true));
		return 0;
	}

	int Boss_GetRevengeState(lua_State* L)
	{
		lua_pushboolean(L, RequireBoss(L)->GetRevengeState() ? 1 : 0);
		return 1;
	}

	int Boss_SetSpecialReady(lua_State* L)
	{
		RequireBoss(L)->SetSpecialReady(OptBool(L, 1, true));
		return 0;
	}

	int Boss_GetSpecialReady(lua_State* L)
	{
		lua_pushboolean(L, RequireBoss(L)->GetSpecialReady() ? 1 : 0);
		return 1;
	}

	int Boss_SetPlayedEffect(lua_State* L)
	{
		RequireBoss(L)->SetPlayedEffect(OptBool(L, 1, true));
		return 0;
	}

	int Boss_GetPlayedEffect(lua_State* L)
	{
		lua_pushboolean(L, RequireBoss(L)->GetPlayedEffect() ? 1 : 0);
		return 1;
	}

	int Boss_GetAttackRange(lua_State* L)
	{
		lua_pushnumber(L, RequireBoss(L)->GetAttackRange());
		return 1;
	}

	int Boss_GetMoveSpeed(lua_State* L)
	{
		lua_pushnumber(L, RequireBoss(L)->GetMoveSpeed());
		return 1;
	}

	int Boss_GetBlendSeconds(lua_State* L)
	{
		lua_pushnumber(L, RequireBoss(L)->GetBlendSeconds());
		return 1;
	}

	//----------------------------------------------------------------
	// シーケンス（連続行動）の状況
	//----------------------------------------------------------------

	int Boss_IsInSequence(lua_State* L)
	{
		BehaviorData<EnemyBoss>* data = RequireAction(L)->GetBehaviorData();
		lua_pushboolean(L, (data != nullptr && data->IsInSequence()) ? 1 : 0);
		return 1;
	}

	int Boss_IsLastNodeInSequence(lua_State* L)
	{
		BehaviorData<EnemyBoss>* data = RequireAction(L)->GetBehaviorData();
		lua_pushboolean(L, (data != nullptr && data->IsLastNodeInSequence()) ? 1 : 0);
		return 1;
	}

	int Boss_IsInSequenceAndNotLast(lua_State* L)
	{
		BehaviorData<EnemyBoss>* data = RequireAction(L)->GetBehaviorData();
		lua_pushboolean(L, (data != nullptr && data->IsInSequenceAndNotLast()) ? 1 : 0);
		return 1;
	}

	//----------------------------------------------------------------
	// エフェクト
	//----------------------------------------------------------------

	int Boss_PlayEffect(lua_State* L)
	{
		EnemyBoss* boss = RequireBoss(L);
		const std::string name = luaL_checkstring(L, 1);

		Effect* effect = nullptr;
		if (name == "LightBall")        effect = boss->lightBall.get();
		else if (name == "Teleport")    effect = boss->teleportEffect.get();
		else if (name == "AttackSign")  effect = boss->attackSign.get();
		else if (name == "MagicCircle") effect = boss->magicCircle.get();

		if (effect == nullptr)
		{
			luaL_error(L, "そのエフェクトはありません: %s "
				"(LightBall / Teleport / AttackSign / MagicCircle)", name.c_str());
			return 0;
		}

		const XMFLOAT3 selfPosition = boss->GetPosition();
		const XMFLOAT3 position{
			OptFloat(L, 2, selfPosition.x),
			OptFloat(L, 3, selfPosition.y),
			OptFloat(L, 4, selfPosition.z) };
		const float scale = OptFloat(L, 5, 1.0f);

		lua_pushinteger(L, effect->Play(position, scale));
		return 1;
	}

	//----------------------------------------------------------------
	// 発射物
	//----------------------------------------------------------------

	// Boss.LaunchSlashWave{ speed = 20, offset = 1.5, homing = false }
	int Boss_LaunchSlashWave(lua_State* L)
	{
		EnemyBoss* boss = RequireBoss(L);

		ProjectileInfo info = ProjectileManager::GetSlashWaveInfo();
		info.owner = boss;

		float offset = 1.5f;
		bool homing = false;

		if (lua_istable(L, 1))
		{
			info.speed = FieldFloat(L, 1, "speed", info.speed);
			info.scale = FieldFloat(L, 1, "scale", info.scale);
			info.damage = FieldInt(L, 1, "damage", info.damage);
			info.radius = FieldFloat(L, 1, "radius", info.radius);
			info.lifeTime = FieldFloat(L, 1, "lifeTime", info.lifeTime);
			offset = FieldFloat(L, 1, "offset", offset);
			homing = FieldBool(L, 1, "homing", homing);
		}

		const XMFLOAT3 selfPosition = boss->GetPosition();
		const XMFLOAT3 forward = Character::CharacterForward(boss->GetAngle());

		info.spawnPosition = {
			selfPosition.x + forward.x * offset,
			selfPosition.y,
			selfPosition.z + forward.z * offset };
		info.direction = forward;

		Projectile* projectile = ProjectileManager::Instance().Launch(info);
		if (projectile != nullptr)
		{
			projectile->FireAt(Player::Instance().GetPosition(), info.speed, homing);
		}

		lua_pushboolean(L, projectile != nullptr ? 1 : 0);
		return 1;
	}

	// Boss.SpawnLightPillarCircle{ count = 8, radius = 5, scale = 1, ... }
	// 生成した光柱は行動側に溜められ、Boss.FireStoredProjectiles でまとめて撃てる。
	int Boss_SpawnLightPillarCircle(lua_State* L)
	{
		LuaAction* action = RequireAction(L);
		EnemyBoss* boss = action->GetOwner();

		int   count = 8;
		float radius = 5.0f;
		float scale = 1.0f;
		float hitRadius = 0.7f;
		float lifeTime = 5.0f;
		float invincibleTime = 0.5f;
		bool  recordOrbit = false;

		XMFLOAT3 center = boss->GetPosition();

		if (lua_istable(L, 1))
		{
			count = FieldInt(L, 1, "count", count);
			radius = FieldFloat(L, 1, "radius", radius);
			scale = FieldFloat(L, 1, "scale", scale);
			hitRadius = FieldFloat(L, 1, "hitRadius", hitRadius);
			lifeTime = FieldFloat(L, 1, "lifeTime", lifeTime);
			invincibleTime = FieldFloat(L, 1, "invincibleTime", invincibleTime);
			recordOrbit = FieldBool(L, 1, "orbit", recordOrbit);

			center.x = FieldFloat(L, 1, "x", center.x);
			center.y = FieldFloat(L, 1, "y", center.y);
			center.z = FieldFloat(L, 1, "z", center.z);
		}

		action->SpawnLightPillarCircle(
			action->GetStoredProjectiles(), center, count, radius,
			scale, hitRadius, lifeTime, invincibleTime, recordOrbit);

		lua_pushinteger(L, static_cast<lua_Integer>(action->GetStoredProjectiles().size()));
		return 1;
	}

	int Boss_SpawnSideLightPillars(lua_State* L)
	{
		LuaAction* action = RequireAction(L);
		action->SpawnSideLightPillars(action->GetStoredProjectiles());

		lua_pushinteger(L, static_cast<lua_Integer>(action->GetStoredProjectiles().size()));
		return 1;
	}

	int Boss_FireStoredProjectiles(lua_State* L)
	{
		LuaAction* action = RequireAction(L);

		const float speed = OptFloat(L, 1, 20.0f);
		const bool homing = OptBool(L, 2, false);

		action->FireAndClear(action->GetStoredProjectiles(), speed, homing);
		return 0;
	}

	int Boss_GetStoredProjectileCount(lua_State* L)
	{
		lua_pushinteger(L,
			static_cast<lua_Integer>(RequireAction(L)->GetStoredProjectiles().size()));
		return 1;
	}

	//----------------------------------------------------------------
	// その他
	//----------------------------------------------------------------

	int Boss_Log(lua_State* L)
	{
		LuaScriptSystem::Instance().Log(
			LuaScriptSystem::LogEntry::Level::Info,
			luaL_checkstring(L, 1));
		return 0;
	}

	const luaL_Reg BOSS_FUNCTIONS[] =
	{
		// アニメーション
		{ "GetAnimationIndex",		Boss_GetAnimationIndex },
		{ "PlayAnimation",			Boss_PlayAnimation },
		{ "PlayRootMotion",			Boss_PlayRootMotion },
		{ "PlayClip",				Boss_PlayClip },
		{ "GetClipCount",			Boss_GetClipCount },
		{ "GetAnimationSeconds",	Boss_GetAnimationSeconds },
		{ "IsPlayingAnimation",		Boss_IsPlayingAnimation },
		{ "SetAnimationSpeed",		Boss_SetAnimationSpeed },

		// キャンセル受付
		{ "GetCancelStart",			Boss_GetCancelStart },
		{ "GetCancelEnd",			Boss_GetCancelEnd },
		{ "IsPastCancelStart",		Boss_IsPastCancelStart },
		{ "IsPastCancelEnd",		Boss_IsPastCancelEnd },
		{ "GetCurrentCancelStart",	Boss_GetCurrentCancelStart },
		{ "GetCurrentCancelEnd",	Boss_GetCurrentCancelEnd },

		// 位置・向き
		{ "GetPosition",			Boss_GetPosition },
		{ "SetPosition",			Boss_SetPosition },
		{ "GetAngle",				Boss_GetAngle },
		{ "GetPlayerPosition",		Boss_GetPlayerPosition },
		{ "GetDistanceToPlayer",	Boss_GetDistanceToPlayer },
		{ "SetTargetToPlayer",		Boss_SetTargetToPlayer },
		{ "SetTargetPosition",		Boss_SetTargetPosition },
		{ "TurnToTarget",			Boss_TurnToTarget },
		{ "MoveToTarget",			Boss_MoveToTarget },
		{ "IsNearPlayer",			Boss_IsNearPlayer },
		{ "LerpTowardPlayer",		Boss_LerpTowardPlayer },
		{ "GetForward",				Boss_GetForward },
		{ "GetBack",				Boss_GetBack },
		{ "GetLeft",				Boss_GetLeft },
		{ "GetRight",				Boss_GetRight },
		{ "SetVerticalVelocity",	Boss_SetVerticalVelocity },
		{ "SetWarpPosition",		Boss_SetWarpPosition },
		// EnemyBoss::SetKnockBackPosition は宣言のみで実装が無いため公開していない

		// テレポート
		{ "CalculateTeleportPosition", Boss_CalculateTeleportPosition },
		{ "StartTeleport",			Boss_StartTeleport },
		{ "IsTeleporting",			Boss_IsTeleporting },

		// 戦闘・状態
		{ "UpdateAttackCollision",	Boss_UpdateAttackCollision },
		{ "IsInterrupted",			Boss_IsInterrupted },
		{ "IsAnyDamage",			Boss_IsAnyDamage },
		{ "IsDead",					Boss_IsDead },
		{ "GetHealth",				Boss_GetHealth },
		{ "GetMaxHealth",			Boss_GetMaxHealth },
		{ "IsHealthBelowRate",		Boss_IsHealthBelowRate },
		{ "SetSuperArmor",			Boss_SetSuperArmor },
		{ "IsSuperArmor",			Boss_IsSuperArmor },
		{ "GetRunTimer",			Boss_GetRunTimer },
		{ "SetRunTimer",			Boss_SetRunTimer },
		{ "GetRevengeValue",		Boss_GetRevengeValue },
		{ "ResetRevengeValue",		Boss_ResetRevengeValue },
		{ "SetRevengeState",		Boss_SetRevengeState },
		{ "GetRevengeState",		Boss_GetRevengeState },
		{ "SetSpecialReady",		Boss_SetSpecialReady },
		{ "GetSpecialReady",		Boss_GetSpecialReady },
		{ "SetPlayedEffect",		Boss_SetPlayedEffect },
		{ "GetPlayedEffect",		Boss_GetPlayedEffect },
		{ "GetAttackRange",			Boss_GetAttackRange },
		{ "GetMoveSpeed",			Boss_GetMoveSpeed },
		{ "GetBlendSeconds",		Boss_GetBlendSeconds },

		// シーケンス
		{ "IsInSequence",			Boss_IsInSequence },
		{ "IsLastNodeInSequence",	Boss_IsLastNodeInSequence },
		{ "IsInSequenceAndNotLast",	Boss_IsInSequenceAndNotLast },

		// エフェクト・発射物
		{ "PlayEffect",				Boss_PlayEffect },
		{ "LaunchSlashWave",		Boss_LaunchSlashWave },
		{ "SpawnLightPillarCircle",	Boss_SpawnLightPillarCircle },
		{ "SpawnSideLightPillars",	Boss_SpawnSideLightPillars },
		{ "FireStoredProjectiles",	Boss_FireStoredProjectiles },
		{ "GetStoredProjectileCount", Boss_GetStoredProjectileCount },

		// その他
		{ "Log",					Boss_Log },

		{ nullptr, nullptr },
	};
}

void LuaBossBindings::EnsureRegistered()
{
	// どの lua_State に対して登録したかを覚えておき、
	// 作り直されたときだけ登録し直す。
	static lua_State* registeredState = nullptr;

	LuaScriptSystem& lua = LuaScriptSystem::Instance();
	lua.Initialize();

	lua_State* L = lua.GetState();
	if (L == nullptr || L == registeredState) return;

	Register(L);
	registeredState = L;
}

void LuaBossBindings::Register(lua_State* L)
{
	if (L == nullptr) return;

	lua_newtable(L);
	luaL_setfuncs(L, BOSS_FUNCTIONS, 0);

	// 旋回速度の目安。C++ 側の TurnSpeed と同じ値。
	lua_pushnumber(L, TurnSpeed::INSTANT);	lua_setfield(L, -2, "TURN_INSTANT");
	lua_pushnumber(L, TurnSpeed::FAST);		lua_setfield(L, -2, "TURN_FAST");
	lua_pushnumber(L, TurnSpeed::SLOW);		lua_setfield(L, -2, "TURN_SLOW");

	lua_setglobal(L, "Boss");
}

void LuaBossBindings::SetContext(LuaAction* action)
{
	currentAction = action;
}

void LuaBossBindings::ClearContext()
{
	currentAction = nullptr;
}

LuaAction* LuaBossBindings::GetContext()
{
	return currentAction;
}
