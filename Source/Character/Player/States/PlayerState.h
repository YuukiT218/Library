#pragma once
#include <unordered_map>

#include "Character/Player/Player.h"

// 前方宣言
class Player;
enum class PlayerStateId;

// ステート基底
class PlayerState
{
public:
	PlayerState(Player* player);
	virtual ~PlayerState() = default;

	// 開始処理
	virtual void Enter() {}

	// 終了処理
	virtual void Exit() {}

	// 更新処理
	virtual void Update(float elapsedTime) {}

	// デバッグ用GUI描画
	virtual void DrawDebugGUI() {}

protected:
	// ルートモーションの基準ノード名
	static constexpr const char* HIPS_NODE_NAME = "Character1_Hips";
	static constexpr const char* REFERENCE_NODE_NAME = "Character1_Reference";

	// アニメーション設定の所有者名
	static constexpr const char* ANIMATION_CONFIG_OWNER = "Player";

	// アニメーション切り替え時の標準ブレンド時間
	static constexpr float DEFAULT_BLEND_SECONDS = 0.1f;

	// 空中アクション中に滞空させるための重力
	static constexpr float HOVER_GRAVITY = -0.0001f;

	// ステート変更
	void ChangeState(PlayerStateId stateId);

	// 歩き移動入力
	bool InputWalkMove() const;

	// 走り移動入力
	bool InputRunMove() const;

	// ロックオンしている場合はストレイフ
	void LockOnStrafe(int rightIndex, int leftIndex, int frontIndex, int backIndex);

	enum class InputActionType
	{
		None,
		LightAttack,
		HeavyAttack,
		Dodge,
		Guard,
		Jump
	};

	InputActionType InputAction();
	InputActionType currentInput;

protected:
	Player* player = nullptr;
	std::unordered_map<InputActionType, PlayerStateId> inputToNextState;
};
