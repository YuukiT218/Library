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
