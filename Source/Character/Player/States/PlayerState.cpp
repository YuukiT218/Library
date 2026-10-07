#include "PlayerState.h"
#include "Camera/CameraParam.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Math/Collision.h"
#include "Math/Mathf.h"


#include <stdlib.h>

namespace
{
    // スティックの傾きがこれより大きければ歩き入力とみなす
    constexpr float WALK_INPUT_THRESHOLD = 0.2f;

    // スティックの傾きがこれより大きければ走り入力とみなす
    constexpr float RUN_INPUT_THRESHOLD = 0.5f;

    // ストレイフ時に入力ありとみなすスティックの傾き
    constexpr float STRAFE_INPUT_THRESHOLD = 0.1f;

    // アニメーション未選択を表すインデックス
    constexpr int NO_ANIMATION_INDEX = -1;
}

//-------------------------------------------------------------
// ステート基盤
//-------------------------------------------------------------
// コンストラクタ
PlayerState::PlayerState(Player* player)
    : player(player)
{
}

// ステート切り替え
void PlayerState::ChangeState(PlayerStateId stateId)
{
    player->GetSword()->ResetAttackState();

    player->ChangeState(stateId);
}

// アクション入力
PlayerState::InputActionType PlayerState::InputAction()
{
    GamePad& gamepad = Input::Instance().GetGamePad();
    const Mouse& mouse = Input::Instance().GetMouse();
    if (gamepad.GetButtonDown() & GamePad::BTN_A)
    {
        currentInput = InputActionType::HeavyAttack;
	    return InputActionType::HeavyAttack;
    }
    if (gamepad.GetButtonDown() & GamePad::BTN_B || mouse.GetButtonDown() & Mouse::BTN_LEFT)
    {
        currentInput = InputActionType::LightAttack;
	    return InputActionType::LightAttack;
    }
    if (gamepad.GetLAxisPower() != 0 && gamepad.GetButtonDown() & GamePad::BTN_X)
    {
	    return InputActionType::Dodge;
    }
	if (gamepad.GetButtonDown() & GamePad::BTN_A)
	{
		return InputActionType::Jump;
	}
	if (gamepad.GetLAxisPower() == 0 && gamepad.GetButton() & GamePad::BTN_X)
	{
		return InputActionType::Guard;
	}
    return InputActionType::None;
}

// 歩き移動処理
bool PlayerState::InputWalkMove() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();

    return gamepad.GetLAxisPower() > WALK_INPUT_THRESHOLD && gamepad.GetLAxisPower() < RUN_INPUT_THRESHOLD;
}

// 走り移動入力
bool PlayerState::InputRunMove() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();

    return gamepad.GetLAxisPower() > RUN_INPUT_THRESHOLD;
}

// ロックオンしている場合はストレイフ
void PlayerState::LockOnStrafe(int rightIndex, int leftIndex, int frontIndex, int backIndex)
{
    if (CameraParam::Instance().IsLockOn())
    {
        const GamePad& gamepad = Input::Instance().GetGamePad();
        if (gamepad.GetLAxisPower() > STRAFE_INPUT_THRESHOLD)
        {
            float axisX = gamepad.GetAxisLX();
            float axisY = gamepad.GetAxisLY();

            const bool isFront = axisY > STRAFE_INPUT_THRESHOLD;
            const bool isBack = axisY < -STRAFE_INPUT_THRESHOLD;
            const bool isRight = axisX > STRAFE_INPUT_THRESHOLD;
            const bool isLeft = axisX < -STRAFE_INPUT_THRESHOLD;

            int newAnimationIndex = NO_ANIMATION_INDEX;

            // 斜め入力は左右のアニメーションを優先する
            if (isRight)
                newAnimationIndex = rightIndex;
            else if (isLeft)
                newAnimationIndex = leftIndex;
            else if (isFront)
                newAnimationIndex = frontIndex;
            else if (isBack)
                newAnimationIndex = backIndex;

            if (newAnimationIndex != NO_ANIMATION_INDEX &&
                newAnimationIndex != player->GetPlayerModel()->GetCurrentAnimationIndex())
            {
                player->GetPlayerModel()->PlayAnimation(newAnimationIndex, true, DEFAULT_BLEND_SECONDS);
            }
        }
    }
    else
    {
        if (frontIndex != player->GetPlayerModel()->GetCurrentAnimationIndex())
        {
            player->GetPlayerModel()->PlayAnimation(frontIndex, true, DEFAULT_BLEND_SECONDS);
        }
    }
}
