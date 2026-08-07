#include "PlayerState.h"
#include "Camera/CameraParam.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Math/Collision.h"
#include "Math/Mathf.h"


#include <stdlib.h>



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

    return gamepad.GetLAxisPower() > 0.2f && gamepad.GetLAxisPower() < 0.5f;
}

// 走り移動入力
bool PlayerState::InputRunMove() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();

    return gamepad.GetLAxisPower() > 0.5f;
}

// ロックオンしている場合はストレイフ
void PlayerState::LockOnStrafe(int rightIndex, int leftIndex, int frontIndex, int backIndex)
{
    if (CameraParam::Instance().GetIsLockOn())
    {
        const GamePad& gamepad = Input::Instance().GetGamePad();
        if (gamepad.GetLAxisPower() > 0.1f)
        {
            float axisX = gamepad.GetAxisLX();
            float axisY = gamepad.GetAxisLY();

            int newAnimationIndex = -1;

            // 右前
            if (axisY > 0.1f && axisX > 0.1f)
                newAnimationIndex = rightIndex;
            // 左前
            else if (axisY > 0.1f && axisX < -0.1f)
                newAnimationIndex = leftIndex;
            // 右後ろ
            else if (axisY < -0.1f && axisX > 0.1f)
                newAnimationIndex = rightIndex;
            // 左後ろ
            else if (axisY < -0.1f && axisX < -0.1f)
                newAnimationIndex = leftIndex;
            // 前
            else if (axisY > 0.1f)
                newAnimationIndex = frontIndex;
            // 後ろ
            else if (axisY < -0.1f)
                newAnimationIndex = backIndex;
            // 右
            else if (axisX > 0.1f)
                newAnimationIndex = rightIndex;
            // 左
            else if (axisX < -0.1f)
                newAnimationIndex = leftIndex;

            if (newAnimationIndex != -1 &&
                newAnimationIndex != player->GetPlayerModel()->GetCurrentAnimationIndex())
            {
                player->GetPlayerModel()->PlayAnimation(newAnimationIndex, true, 0.1f);
            }
        }
    }
    else
    {
        if (frontIndex != player->GetPlayerModel()->GetCurrentAnimationIndex())
        {
            player->GetPlayerModel()->PlayAnimation(frontIndex, true, 0.1f);
        }
    }
}
