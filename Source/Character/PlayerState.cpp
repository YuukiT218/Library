#include "PlayerState.h"
#include "Camera/CameraParam.h"
#include "Enemy/EnemyBoss.h"
#include "Math/Collision.h"
#include "Math/Mathf.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

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

//-------------------------------------------------------------
// 待機ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerIdleState::PlayerIdleState(Player* player)
    : PlayerState(player)
{
    idleAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Idle_Combat_Seq_0");
}

// 開始処理
void PlayerIdleState::Enter()
{
    player->GetPlayerModel()->PlayAnimation(idleAnimationIndex, true, 0.1f);
}

// 更新処理
void PlayerIdleState::Update(float elapsedTime)
{
    // コンボ1ステートに遷移
    if (InputAction() == InputActionType::LightAttack)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 回避ステートに遷移
    else if (InputAction() == InputActionType::Dodge)
    {
        ChangeState(PlayerStateId::Dodge);
    }
    // 走りステートに遷移
    else if (InputRunMove())
    {
        ChangeState(PlayerStateId::Run);
    }
    // 歩きステートに遷移
    else if (InputWalkMove())
    {
        ChangeState(PlayerStateId::Walk);
    }
    // ガード待機に遷移
    else if (InputAction() == InputActionType::Guard)
    {
        ChangeState(PlayerStateId::GuardIdle);
    }
    else if (InputAction() == InputActionType::HeavyAttack)
    {
        ChangeState(PlayerStateId::Jump);
    }
}

// デバッグ用GUI描画
void PlayerIdleState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"待機"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(idleAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(idleAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &idleAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 歩きステート
//-------------------------------------------------------------
// コンストラクタ
PlayerWalkState::PlayerWalkState(Player* player)
    : PlayerState(player)
{
    walkFrontAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Walk_Combat_Loop_F_0_RM_Seq_0");
    walkBackAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Walk_Combat_Loop_F_0_Seq_0");
    walkRightAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Walk_Combat_Loop_F_0_Seq_0");
    walkLeftAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Walk_Combat_Loop_F_0_Seq_0");
}

// 開始処理
void PlayerWalkState::Enter()
{
    player->GetPlayerModel()->PlayRootMotion(walkFrontAnimationIndex, true, true, 0.4f, "Character1_Hips");
}

// 更新処理
void PlayerWalkState::Update(float elapsedTime)
{
    player->PlayerMove(elapsedTime, 0);

    float speed = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(&player->GetMoveVec())));

    float t = std::clamp(speed, 0.0f, 1.0f);

    walkAnimationSpeed = Mathf::Lerp(0.2f, 0.8f, t);

    // コンボ1ステートに遷移
    if (InputAction() == InputActionType::LightAttack)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 回避ステートに遷移
    else if (InputAction() == InputActionType::Dodge)
    {
        ChangeState(PlayerStateId::Dodge);
    }
    // ガード待機ステート遷移
    else if (InputAction() == InputActionType::Guard)
    {
        ChangeState(PlayerStateId::GuardIdle);
    }
    // 走りステートに遷移
    else if (InputRunMove())
    {
        ChangeState(PlayerStateId::Run);
    }
    // アイドルステートに遷移
    else if (!InputWalkMove() && !InputRunMove())
    {
        ChangeState(PlayerStateId::Idle);
    }
    else if (InputAction() == InputActionType::HeavyAttack)
    {
        ChangeState(PlayerStateId::Jump);
    }

    //player->GetPlayerModel()->SetBaseAnimationSpeed(walkAnimationSpeed);
}

// デバッグ用GUI描画
void PlayerWalkState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"歩き"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(walkFrontAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(walkFrontAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &walkAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"移動率", &walkAnimationMoveRate, 0.01f, 0.0f, 5.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 走りステート
//-------------------------------------------------------------
// コンストラクタ
PlayerRunState::PlayerRunState(Player* player)
    : PlayerState(player)
{
    runStartAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Run_Fast_Combat_Start_RM_Seq_0");
    runLoopAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Run_Fast_Combat_Loop_RM_Seq_0");
    runEndAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Run_Fast_Combat_End_RM_Seq_0");
    runRightAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorRunRightward");
    runLeftAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorRunLeftward");
}

// 開始処理
void PlayerRunState::Enter()
{
    player->GetPlayerModel()->PlayRootMotion(runStartAnimationIndex, false, true, 0.4f, "Character1_Hips");
}

// 更新処理
void PlayerRunState::Update(float elapsedTime)
{
	player->PlayerMove(elapsedTime, 0);
    if (!player->GetPlayerModel()->IsPlayAnimation())
    {
        player->GetPlayerModel()->PlayRootMotion(runLoopAnimationIndex, true, true, 0.1f, "Character1_Hips");
    }
    //
    // コンボ1ステートに遷移
    if (InputAction() == InputActionType::LightAttack)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 回避ステートに遷移
    else if (InputAction() == InputActionType::Dodge)
    {
        ChangeState(PlayerStateId::Dodge);
    }
    // ガード待機ステート遷移
    else if (InputAction() == InputActionType::Guard)
    {
        ChangeState(PlayerStateId::GuardIdle);
    }
    // 走りステートに遷移
    else if (InputWalkMove())
    {
        ChangeState(PlayerStateId::Walk);
    }
    // アイドルステートに遷移
    else if (!InputWalkMove() && !InputRunMove())
    {
        player->GetPlayerModel()->PlayRootMotion(runEndAnimationIndex, false, true, 0.1f, "Character1_Hips");
    	ChangeState(PlayerStateId::Idle);
    }
	else if (InputAction() == InputActionType::HeavyAttack)
	{
		ChangeState(PlayerStateId::Jump);
	}

    //player->GetPlayerModel()->SetBaseAnimationSpeed(runAnimationSpeed);
}

// デバッグ用GUI描画
void PlayerRunState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"走り"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(runFrontAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(runFrontAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &runAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"移動率", &runAnimationMoveRate, 0.01f, 0.0f, 5.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// ジャンプステート
//-------------------------------------------------------------
// コンストラクタ
PlayerJumpState::PlayerJumpState(Player* player)
    : PlayerState(player)
{
    jumpAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Jump_Combat_Start_0_Seq_0");
    fallAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Jump_Combat_Loop_0_Seq_0");
}

// 開始処理
void PlayerJumpState::Enter()
{
    if (player->IsGround())
    {
        player->GetPlayerModel()->PlayRootMotion(jumpAnimationIndex, false, true, 0.1f, "Character1_Reference");
        player->PlayerJump(jumpPower);
    }
}

// 更新処理
void PlayerJumpState::Update(float elapsedTime)
{
    player->PlayerMove(elapsedTime, jumpAnimationMoveRate);
    if (!player->GetPlayerModel()->IsPlayAnimation())
    {
        player->GetPlayerModel()->PlayRootMotion(fallAnimationIndex, true, true, 0.1f, "Character1_Reference");
    }

    // コンボ1ステートに遷移
    if (InputAction() == InputActionType::LightAttack)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 回避ステートに遷移
    else if (InputAction() == InputActionType::Dodge)
    {
        ChangeState(PlayerStateId::Dodge);
    }
    else if (InputAction() == InputActionType::Guard)
    {
        ChangeState(PlayerStateId::GuardIdle);
    }
	else if (player->IsGround())
	{
		ChangeState(PlayerStateId::Idle);
	}

    //player->GetPlayerModel()->SetBaseAnimationSpeed(runAnimationSpeed);
}

// デバッグ用GUI描画
void PlayerJumpState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"ジャンプ"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(runFrontAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(runFrontAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &jumpAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"移動率", &jumpAnimationMoveRate, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"ジャンプ力", &jumpPower, 0.01f, 0.0f, 20.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 落下ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerFallState::PlayerFallState(Player* player)
    : PlayerState(player)
{
    fallAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Jump_Combat_Loop_0_Seq_0");
}

// 開始処理
void PlayerFallState::Enter()
{
    if (!player->IsGround())
    {
        player->GetPlayerModel()->PlayRootMotion(fallAnimationIndex, true, true, 0.1f, "Character1_Reference");
    }
}

// 更新処理
void PlayerFallState::Update(float elapsedTime)
{
    player->PlayerMove(elapsedTime, fallAnimationMoveRate);

    // コンボ1ステートに遷移
    if (InputAction() == InputActionType::LightAttack)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 回避ステートに遷移
    else if (InputAction() == InputActionType::Dodge)
    {
        ChangeState(PlayerStateId::Dodge);
    }
    else if (InputAction() == InputActionType::Guard)
    {
        ChangeState(PlayerStateId::GuardIdle);
    }
    else if (player->IsGround())
    {
        ChangeState(PlayerStateId::Idle);
    }

    //player->GetPlayerModel()->SetBaseAnimationSpeed(runAnimationSpeed);
}

// デバッグ用GUI描画
void PlayerFallState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"落下"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(runFrontAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(runFrontAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &fallAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"移動率", &fallAnimationMoveRate, 0.01f, 0.0f, 5.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 回避ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerDodgeState::PlayerDodgeState(Player* player)
    : PlayerState(player)
{
    dodgeAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Roll_Combat_F_0_Seq_0");
    airDodgeAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Dodge_Air_Combat_F_Seq_0");
}

// 開始処理
void PlayerDodgeState::Enter()
{
    // スティック入力方向を取得
    DirectX::XMFLOAT3 moveVec = player->GetMoveVec();
    float moveVecLength = sqrtf(moveVec.x * moveVec.x + moveVec.z * moveVec.z);

    // スティック入力がある場合、その方向に即座に向く
    if (moveVecLength > 0.0f)
    {
        // 入力方向の角度を計算
        float targetAngle = atan2f(moveVec.x, moveVec.z);

        // プレイヤーの角度を入力方向に即座に設定
        DirectX::XMFLOAT3 angle = player->GetAngle();
        angle.y = targetAngle;
        player->SetAngle(angle);
    }

    const Camera& camera = Camera::Instance();
    const GamePad& gamepad = Input::Instance().GetGamePad();
    
    DirectX::XMVECTOR Vec;
    DirectX::XMFLOAT3 vec;

    // ワールド進行方向を取得
    Vec = DirectX::XMLoadFloat3(&player->CharacterForward(player->GetAngle()));
    Vec = DirectX::XMVector3Normalize(Vec);
    DirectX::XMStoreFloat3(&vec, Vec);

    if (player->IsGround())
    {
        player->GetPlayerModel()->PlayRootMotion(dodgeAnimationIndex, false, true, 0.1f, "Character1_Hips");
        timer = dodgeAnimationTime;
    }
    else
    {
        player->SetGravity(-0.0001f);
        player->SetVerticalVelocity(0);
        player->GetPlayerModel()->PlayRootMotion(airDodgeAnimationIndex, false, true, 0.1f, "Character1_Hips");
        timer = airDodgeAnimationTime;
    }

	player->SetPlayerRolling(true);
    nextShiftReady = false;

    player->SetMovement(vec, 3.0f);
}

    
// 更新処理
void PlayerDodgeState::Update(float elapsedTime)
{
    float frame = player->GetPlayerModel()->GetCurrentAnimationSeconds();
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();

    AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig("Player", index);

    timer -= elapsedTime;
    if (frame >= config->advanceInputStartFrame || frame <= config->advanceInputEndFrame)
    {
        if (InputAction() != InputActionType::None)
        {
            nextShiftReady = true;
            nextInput = InputAction();
        }
    }

    if (frame >= config->advanceInputEndFrame)
    {
        if (nextShiftReady)
        {
            if (nextInput == InputActionType::Dodge)
            {
                ChangeState(PlayerStateId::Dodge);
            }
            if (nextInput == InputActionType::LightAttack)
            {
                ChangeState(PlayerStateId::Combo1);
            }
        }
        // 走りステート遷移
        if (player->IsGround() && InputRunMove())
        {
            ChangeState(PlayerStateId::Run);
        }
        // 歩きステート遷移
        else if (player->IsGround() && InputWalkMove())
        {
            ChangeState(PlayerStateId::Walk);
        }
    }

    if (!player->IsGround() && !player->GetPlayerModel()->IsPlayAnimation())
    {
        ChangeState(PlayerStateId::Fall);
    }
    // 待機ステート遷移
    else if (!player->GetPlayerModel()->IsPlayAnimation())
    {
        ChangeState(PlayerStateId::Idle);
    }
}

// 終了処理
void PlayerDodgeState::Exit()
{
    player->GetPlayerModel()->SetAnimationSpeed(1.0f);
    player->SetGravity(-0.3f);
    player->SetPlayerRolling(false);
}

// デバッグ用GUI描画
void PlayerDodgeState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"バックステップ回避"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(dodgeBackAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(dodgeBackAnimationIndex));
        //ImGui::DragFloat(u8"アニメーションスピード", &dodgeBackAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"アニメーション遷移時間", &dodgeBackAnimationTime, 0.01f, 0.0f, 5.0f);
        ImGui::DragInt(u8"移動距離", &dodgeBackMovePow);
        ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGui::TreeNode(u8"ローリング回避"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(rollingFrontAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(rollingFrontAnimationIndex));
        //ImGui::DragFloat(u8"アニメーションスピード", &dodgeBackAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"アニメーション遷移時間", &dodgeAnimationTime, 0.01f, 0.0f, 5.0f);
        ImGui::DragInt(u8"移動距離", &rollingFrontMovePow);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// コンボステート
//-------------------------------------------------------------
// コンストラクタ
PlayerComboState::PlayerComboState(Player* player)
    : PlayerState(player)
{
}

// 開始処理
void PlayerComboState::Enter()
{
    forwarded = false;
    nextShiftReady = false;
    player->ResetTurnCompleted();  // プレイヤーの旋回完了フラグをリセット
    // 距離判定：Y座標を無視してXZ平面の距離のみで判定する
    DirectX::XMFLOAT3 playerPos = player->GetPosition();
    enemyPos = EnemyBoss::Instance().GetPosition();
    float diffX = playerPos.x - enemyPos.x;
    float diffZ = playerPos.z - enemyPos.z;
    float targetDist = sqrtf(diffX * diffX + diffZ * diffZ);

    // ダッシュ攻撃の判定と再生
    if (player->IsGround())
    {
        if (targetDist > 2.0f && targetDist < 12.5f
            && comboAnimationIndex == player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_04_01_Seq_0") && CameraParam::Instance().GetIsLockOn())
        {
            player->GetPlayerModel()->PlayRootMotion(dashAttackAnimationIndex, false, true, 0.1f, "Character1_Hips");
        }
        else
        {
            player->GetPlayerModel()->PlayRootMotion(comboAnimationIndex, false, isBakeY, 0.1f, "Character1_Hips");
        }
    }
    else
    {
        player->SetGravity(-0.0001f);
        player->SetHorizonVelocity(0);
        player->SetVerticalVelocity(0);
        if (targetDist > 2.0f && targetDist < 12.5f
            && airComboAnimationIndex == player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_Air_06_01_Seq_0") && CameraParam::Instance().GetIsLockOn())
        {
            player->GetPlayerModel()->PlayRootMotion(airDashAttackAnimationIndex, false, true, 0.1f, "Character1_Hips");
        }
        else
        {
            player->GetPlayerModel()->PlayRootMotion(airComboAnimationIndex, false, isBakeY, 0.1f, "Character1_Hips");
        }
    }
}

// 終了処理
void PlayerComboState::Exit()
{
    player->SetGravity(-0.3f);
    nextShiftReady = false;
}

// 更新処理
void PlayerComboState::Update(float elapsedTime)
{
    float frame = player->GetPlayerModel()->GetCurrentAnimationSeconds();
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();

    AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig("Player", index);

    if (CameraParam::Instance().GetIsLockOn())
    {
        if (!player->IsTurnCompleted())
        {
            player->LockOnTurnToEnemy(elapsedTime);
        }

        if (frame <= config->advanceInputEndFrame)
        {
            DirectX::XMFLOAT3 playerPos = player->GetPosition();

            // 距離計算：ここでもXZ平面距離を使用することで、高さがある場合でも正しく吸い付き判定を行う
            float diffX = playerPos.x - enemyPos.x;
            float diffZ = playerPos.z - enemyPos.z;
            float dist = sqrtf(diffX * diffX + diffZ * diffZ);

            bool isUpdated = false;

            // ダッシュ攻撃（地上・空中）の場合のホーミング処理
            if (index == dashAttackAnimationIndex || index == airDashAttackAnimationIndex)
            {
                if (dist >= 6.0f)
                {
                    float lerpRateXZ = 5.0f * elapsedTime;
                    playerPos.x = Mathf::Lerp(playerPos.x, enemyPos.x, lerpRateXZ);
                    playerPos.z = Mathf::Lerp(playerPos.z, enemyPos.z, lerpRateXZ);
                    isUpdated = true;
                }
            }

            // 高さ補正処理
            if (!player->IsGround() || index == airDashAttackAnimationIndex || index == airComboAnimationIndex)
            {
                float yDiff = EnemyBoss::Instance().GetPosition().y - playerPos.y;
                float heightLimit = -2.0f;

                bool isTooHigh = (yDiff > heightLimit);

                if (!isTooHigh)
                {
                    float lerpRateY = 10.0f * elapsedTime;
                    playerPos.y = Mathf::Lerp(playerPos.y, EnemyBoss::Instance().GetPosition().y, lerpRateY);
                    isUpdated = true;
                }
            }

            // 変更があった場合のみ適用
            if (isUpdated)
            {
                player->SetPosition(playerPos);
            }
        }
    }

    player->GetSword()->AttackAnimationCollision(player->GetModel(), config, player);
    
    InputActionType input = InputAction();

    // 次のコンボステート処理
    if (inputToNextState.count(input))
    {
        // 先行入力処理
        if (input != InputActionType::None)
        {
            if (input == InputActionType::HeavyAttack && !player->IsGround())
                return;
            if (frame >= config->advanceInputStartFrame && frame <= config->advanceInputEndFrame)
            {
                nextShiftReady = true;
                nextInput = input;
            }
        }
    }

    if (InputAction() == InputActionType::Dodge || InputAction() == InputActionType::Guard)
    {
        if (frame >= config->advanceInputStartFrame && frame <= config->advanceInputEndFrame)
        {
            nextShiftReady = true;
            nextInput = InputAction();
        }
    }

    // 次のコンボステートへ遷移
    if (nextShiftReady)
    {
        if (frame >= config->advanceInputEndFrame)
        {
            if (nextInput == InputActionType::Dodge)
                ChangeState(PlayerStateId::Dodge);
            if (nextInput == InputActionType::Guard)
                ChangeState(PlayerStateId::GuardIdle);
        	if (nextInput == InputActionType::LightAttack || nextInput == InputActionType::HeavyAttack)
                ChangeState(inputToNextState[nextInput]);
        }
    }

    // 終了後のステート遷移
    if (!player->GetPlayerModel()->IsPlayAnimation() && player->IsGround())
    {
    	ChangeState(PlayerStateId::Idle);
    }

    else if (!player->IsGround() && frame >= endFrame || !player->GetPlayerModel()->IsPlayAnimation())
    {
        player->SetGravity(-0.3f);
        ChangeState(PlayerStateId::Fall);
    }
}

//-------------------------------------------------------------
// コンボ1ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerCombo1State::PlayerCombo1State(Player* player)
    : PlayerComboState(player)
{
	comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_04_01_Seq_0");
    airComboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_Air_06_01_Seq_0");
    dashAttackAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Run_Attack_01_Seq_0");
    airDashAttackAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Dash_Air_Attack_Seq_0");
    isBakeY = true;

    nextShiftFrame = 0.53f;
    poseFrame = 0.53f;
    endFrame = 0.816f;
    comboAttackSpeed = 1.3f;
    comboPoseSpeed = 0.5f;

    forwardFrame = 0.15f;
    forwardPower = 12.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.26f;
    attackCollisionEndFrame = 0.49f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;

    inputToNextState[InputActionType::LightAttack] = PlayerStateId::Combo2;
    inputToNextState[InputActionType::HeavyAttack] = PlayerStateId::Heavy1;
}

// デバッグ用GUI描画
void PlayerCombo1State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"コンボ1"))
    {

        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// コンボ2ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerCombo2State::PlayerCombo2State(Player* player)
    : PlayerComboState(player)
{
    inputToNextState[InputActionType::LightAttack] = PlayerStateId::Combo3;
    inputToNextState[InputActionType::HeavyAttack] = PlayerStateId::Heavy1;
    nextShiftFrame = 0.5f;
    poseFrame = 0.5f;
    endFrame = 0.816f;
    comboAttackSpeed = 1.5f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_04_02_Seq_0");
    airComboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_Air_06_02_Seq_0");
    isBakeY = true;

    forwardFrame = 0.0f;
    forwardPower = 15.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.28f;
    attackCollisionEndFrame = 0.5f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

// デバッグ用GUI描画
void PlayerCombo2State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"コンボ２"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// コンボ3ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerCombo3State::PlayerCombo3State(Player* player)
    : PlayerComboState(player)
{
    inputToNextState[InputActionType::LightAttack] = PlayerStateId::Combo4;
    inputToNextState[InputActionType::HeavyAttack] = PlayerStateId::Heavy1;
    nextShiftFrame = 0.63f;
    poseFrame = 0.63f;
    endFrame = 0.9f;
    comboAttackSpeed = 1.0f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_04_03_Seq_0");
    airComboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_Air_06_03_Seq_0");
    isBakeY = true;

    forwardFrame = 0.46f;
    forwardPower = 13.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.45f;
    attackCollisionEndFrame = 0.65f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

// デバッグ用GUI描画
void PlayerCombo3State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"コンボ3"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// コンボ4ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerCombo4State::PlayerCombo4State(Player* player)
    : PlayerComboState(player)
{
    inputToNextState[InputActionType::LightAttack] = PlayerStateId::Combo5;
	inputToNextState[InputActionType::HeavyAttack] = PlayerStateId::Heavy1;
    nextShiftFrame = 0.7f;
    poseFrame = 0.7f;
    endFrame = 1.016f;
    comboAttackSpeed = 1.0f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_04_03_Seq_0");
    airComboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_Air_06_04_Seq_0");
    isBakeY = true;

    forwardFrame = 0.23f;
    forwardPower = 20.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.37f;
    attackCollisionEndFrame = 0.65f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

// デバッグ用GUI描画
void PlayerCombo4State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"コンボ4"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// コンボ5ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerCombo5State::PlayerCombo5State(Player* player)
    : PlayerComboState(player)
{
    inputToNextState[InputActionType::LightAttack] = PlayerStateId::Combo1;
    inputToNextState[InputActionType::HeavyAttack] = PlayerStateId::Heavy1;
    nextShiftFrame = 0.7f;
    poseFrame = 0.7f;
    endFrame = 1.016f;
    comboAttackSpeed = 1.0f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_04_04_Seq_0");
    airComboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_01_04_Seq_0");
    isBakeY = true;

    forwardFrame = 0.23f;
    forwardPower = 20.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.37f;
    attackCollisionEndFrame = 0.65f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

// デバッグ用GUI描画
void PlayerCombo5State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"コンボ4"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 強攻撃1ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerHeavyAttack1State::PlayerHeavyAttack1State(Player* player)
    : PlayerComboState(player)
{
    inputToNextState[InputActionType::LightAttack] = PlayerStateId::Combo1;
    nextShiftFrame = 0.75f;
    poseFrame = 0.4f;
    endFrame = 0.95f;
    comboAttackSpeed = 1.0f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Attack_Up_Floor_To_Air_02_Seq_0");
    isBakeY = false;

    forwardFrame = 0.0f;
    forwardPower = 17.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.13f;
    attackCollisionEndFrame = 0.25f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

void PlayerHeavyAttack1State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"強攻撃1"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(comboAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(comboAnimationIndex));
        ImGui::Checkbox(u8"先行入力しているか", &nextShiftReady);
        ImGui::DragFloat(u8"攻撃スピード", &comboAttackSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"構えスピード", &comboPoseSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat(u8"先行入力受付開始フレーム", &nextShiftFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"先行入力受付終了フレーム", &endFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動開始フレーム", &forwardFrame, 0.01f, 0.0f);
        ImGui::DragFloat(u8"前移動値", &forwardPower, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの左振動の強さ", &attackLeftVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"コントローラーの右振動の強さ", &attackRightVibrate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時に何秒間止めるか", &attackHitStopTime, 0.01f, 0.0f);
        ImGui::DragFloat(u8"攻撃がヒットした時にどれぐらいの速さにするか", &attackHitStopSpeed, 0.01f, 0.0f);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// ガード待機ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerGuardIdle::PlayerGuardIdle(Player* player)
    : PlayerState(player)
{
    guardStartAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Block_Start_Seq_0");
    guardLoopAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Block_Loop_Seq_0");
    guardEndAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Block_End_Seq_0");
}

// 開始処理
void PlayerGuardIdle::Enter()
{
    player->SetGravity(-0.0001f);
    player->SetVerticalVelocity(0);
    player->SetHorizonVelocity(0);

    if (!player->GetPlayerIsGuard())
    {
	    player->ResetTurnCompleted();  // プレイヤーの旋回完了フラグをリセット
    	player->GetPlayerModel()->PlayRootMotion(guardStartAnimationIndex, false, true, 0.1f, "Character1_Reference");
    }

    player->SetPlayerGuard(true);
}

// 更新処理
void PlayerGuardIdle::Update(float elapsedTime)
{
    if (!player->GetPlayerModel()->IsPlayAnimation())
    {
        player->GetPlayerModel()->PlayRootMotion(guardLoopAnimationIndex, false, true, 0.2f, "Character1_Reference");
        index = player->GetPlayerModel()->GetCurrentAnimationIndex();
        config = player->GetPlayerModel()->GetAnimationConfig("Player", index);
        isLoop = true;
        timer = 0.0f;
    }

    if (CameraParam::Instance().GetIsLockOn())
    {
        // まだ向き終わっていない場合のみ実行
        if (!player->IsTurnCompleted())
        {
            player->LockOnTurnToEnemy(elapsedTime);
        }
    }

    if (isLoop)
	{
        // ガードカウンターステートに遷移
        if (player->GetPlayerIsCounter() && InputAction() == InputActionType::LightAttack)
        {
            player->SetPlayerCounter(false);
            player->SetPlayerGuard(false);
            ChangeState(PlayerStateId::GuardCounter);
        }
        else if (player->GetPlayerIsCounter() && InputAction() == InputActionType::Dodge)
        {
            player->SetPlayerCounter(false);
            player->SetPlayerGuard(false);
            ChangeState(PlayerStateId::Dodge);
        }
        timer += elapsedTime;
        frame = player->GetPlayerModel()->GetCurrentAnimationSeconds();
		if (config->advanceInputEndFrame <= timer)
		{
			if (player->IsGround())
			{
				ChangeState(PlayerStateId::Idle);
			}
			else
			{
				ChangeState(PlayerStateId::Fall);
			}
		}
		else if (config->attributes[0].endTime <= frame)
		{
			player->GetPlayerModel()->PlayRootMotion(guardEndAnimationIndex, false, true, 0.1f, "Character1_Reference");
			player->SetPlayerGuard(false);
		}
	}
}

// 終了処理
void PlayerGuardIdle::Exit()
{
    isLoop = false;
    player->SetGravity(-0.3f);
}

// デバッグ用GUI描画
void PlayerGuardIdle::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"ガード待機"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(guardIdleAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(guardIdleAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &guardIdleAnimationSpeed, 0.01f, 0.0f, 5.0f);
        ImGui::TreePop();
    }
}


//-------------------------------------------------------------
// ガードヒットステート
//-------------------------------------------------------------
// コンストラクタ
PlayerGuardHit::PlayerGuardHit(Player* player)
    : PlayerState(player)
{
    guardHitAnimationIndex = player->GetModel()->GetAnimationIndex("Block_Hit_Seq_0");
}

void PlayerGuardHit::Enter()
{
    player->GetModel()->PlayRootMotion(guardHitAnimationIndex, false, true, 0.1f, "Character1_Reference");

    player->guardEffect->Play(player->GetPosition(), 0.2f);

    player->SetPlayerGuard(true);
    player->SetPlayerCounter(true);
    player->ResetTurnCompleted();

    GamePad& gamepad = Input::Instance().GetGamePad();
}

void PlayerGuardHit::Update(float elapsedTime)
{
    float frame = player->GetPlayerModel()->GetCurrentAnimationSeconds();
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();
    GamePad& gamepad = Input::Instance().GetGamePad();
    AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig("Player", index);

    if (!player->IsTurnCompleted())
    {
        player->LockOnTurnToEnemy(elapsedTime);
    }

    if (config->advanceInputEndFrame <= frame)
    {
        // ガードカウンターステートに遷移
        if (InputAction() == InputActionType::LightAttack)
        {
            player->SetPlayerCounter(false);
            player->SetPlayerGuard(false);
            ChangeState(PlayerStateId::GuardCounter);
        }
        // 回避ステートに遷移
        else if (InputAction() == InputActionType::Dodge)
        {
            player->SetPlayerCounter(false);
            player->SetPlayerGuard(false);
            ChangeState(PlayerStateId::Dodge);
        }
    }
    if (!player->GetPlayerModel()->IsPlayAnimation())
	{
        ChangeState(PlayerStateId::GuardIdle);
	}
}

// 終了処理
void PlayerGuardHit::Exit()
{
    
}

//-------------------------------------------------------------
// ガードカウンターステート
//-------------------------------------------------------------
// コンストラクタ
PlayerGuardCounter::PlayerGuardCounter(Player* player)
    : PlayerState(player)
{
    guardParryAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Parry_Counter_Attack_R_Seq_0");
    guardParryAnimationIndex1 = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_Wave_05_04_Seq_0");
    guardParryAnimationIndex2 = player->GetPlayerModel()->GetAnimationIndex("Run_Attack_02_Seq_0");
}

void PlayerGuardCounter::Enter()
{
    // 敵との距離を計算（XZ平面距離）
    DirectX::XMFLOAT3 playerPos = player->GetPosition();
    enemyPos = EnemyBoss::Instance().GetPosition();
    float diffX = playerPos.x - enemyPos.x;
    float diffZ = playerPos.z - enemyPos.z;
    float dist = sqrtf(diffX * diffX + diffZ * diffZ);

    // 距離に応じたアニメーション分岐
    if (dist > 5.0f)
    {
        // 遠距離：突進攻撃 (Run Attack)
        player->GetPlayerModel()->PlayRootMotion(guardParryAnimationIndex2, false, true, 0.1f, "Character1_Hips");
    }
    else if (dist < 2.0f)
    {
        // 近距離：密着攻撃 (Combo Attack Wave)
        player->GetPlayerModel()->PlayRootMotion(guardParryAnimationIndex1, false, true, 0.1f, "Character1_Hips");
    }
    else
    {
        // 中距離：通常カウンター
        player->GetPlayerModel()->PlayRootMotion(guardParryAnimationIndex, false, true, 0.1f, "Character1_Hips");
    }

    player->SetPlayerRolling(true);
    player->ResetTurnCompleted();  // プレイヤーの旋回完了フラグをリセット
}

void PlayerGuardCounter::Update(float elapsedTime)
{
    float frame = player->GetPlayerModel()->GetCurrentAnimationSeconds();
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();

    AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig("Player", index);
    
    player->GetSword()->AttackAnimationCollision(player->GetModel(), config, player);

    if (CameraParam::Instance().GetIsLockOn())
    {
        // 距離計算（XZ平面距離）
        DirectX::XMFLOAT3 playerPos = player->GetPosition();
        float diffX = playerPos.x - enemyPos.x;
        float diffZ = playerPos.z - enemyPos.z;
        float dist = sqrtf(diffX * diffX + diffZ * diffZ);

        // オービット対策
        if (dist >= 2.0f)
        {
            if (!player->IsTurnCompleted())
            {
                player->LockOnTurnToEnemy(elapsedTime);
            }
        }

        // --- 遠距離突進攻撃(Run_Attack_02)の場合のホーミング処理 ---
        if (index == guardParryAnimationIndex2 && frame <= config->advanceInputEndFrame)
        {
            if (dist >= 6.0f)
            {
                float lerpRate = 5.0f * elapsedTime;
                playerPos.x = Mathf::Lerp(playerPos.x, enemyPos.x, lerpRate);
                playerPos.z = Mathf::Lerp(playerPos.z, enemyPos.z, lerpRate);

                player->SetPosition(playerPos);
            }
        }
    }

    // 先行入力処理
    if (InputAction() != InputActionType::None)
    {
        if (InputAction() == InputActionType::HeavyAttack && !player->IsGround())
            return;
        if (frame >= config->advanceInputStartFrame && frame <= config->advanceInputEndFrame)
        {
            nextShiftReady = true;
            nextInput = InputAction();
        }
    }

    if (frame >= config->advanceInputEndFrame)
    {
	    if (nextShiftReady)
	    {
	    	if (nextInput == InputActionType::LightAttack)
	    	{
	    		ChangeState(PlayerStateId::Combo1);
	    	}
	    	else if (nextInput == InputActionType::HeavyAttack)
	    	{
	    		ChangeState(PlayerStateId::Heavy1);
	    	}
	    	else if (nextInput == InputActionType::Dodge)
	    	{
	    		ChangeState(PlayerStateId::Dodge);
	    	}
	    	else if (nextInput == InputActionType::Guard)
	    	{
	    		ChangeState(PlayerStateId::GuardIdle);
	    	}
	    }
    }

    if (!player->GetPlayerModel()->IsPlayAnimation())
    {
        // 待機ステート遷移
    	ChangeState(PlayerStateId::Idle);
    }
}

void PlayerGuardCounter::Exit()
{
    player->SetPlayerRolling(false);
}

// デバッグ用GUI描画
void PlayerGuardCounter::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"ガードパリィ"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(guardParryAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(guardParryAnimationIndex));
        ImGui::DragFloat(u8"アニメーションスピード", &guardParryAnimationSpeed, 0.01f, 0.0f, 5.0f);
        bool parryFlag = player->GetPlayerIsParry();
        ImGui::Checkbox(u8"パリィ判定があるか", &parryFlag);
        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// ダメージステート
//-------------------------------------------------------------
// コンストラクタ
PlayerDamageState::PlayerDamageState(Player* player)
    : PlayerState(player)
{
    anims.normalGround = player->GetPlayerModel()->GetAnimationIndex("Hit_Combat_F_Seq_0");
    anims.lightGround = player->GetPlayerModel()->GetAnimationIndex("Hit_Large_Combat_F_Seq_0");
    anims.heavyGround = player->GetPlayerModel()->GetAnimationIndex("Hit_Large_Combat_Death_Seq_0");
    anims.airStart = player->GetPlayerModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_Start_Seq_0");
    anims.airLoop = player->GetPlayerModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_Loop_Seq_0");
    anims.airEnd = player->GetPlayerModel()->GetAnimationIndex("Hit_Combat_Air_Large_To_Floor_End_Seq_0");
    anims.getUp = player->GetPlayerModel()->GetAnimationIndex("Get_Up_Combat_Seq_0");
}

// ダメージタイプ別の初期処理
void PlayerDamageState::HandleDamageStart(DamageType type, float elapsedTime)
{
    // 攻撃を受けた方向を向く
    TurnToDamageDirection(elapsedTime);

    // ダメージフラグをリセット
    player->SetDamage(false);
    player->SetLightDamage(false);
    player->SetHeavyDamage(false);
    player->SetLaunchDamage(false);
    player->SetKnockDownDamage(false);

    // ノックバック目標位置を計算
    knockbackTargetPosition = GetKnockbackPosition(type);

    switch (type)
    {
    case DamageType::Normal:
        if (!player->IsGround())
        {
            // 空中ダメージ
            player->SetGravity(-0.15f);
            player->SetPosition({
                Mathf::Lerp(player->GetPosition().x, knockbackTargetPosition.x, 0.15f),
                player->GetPosition().y,
                Mathf::Lerp(player->GetPosition().z, knockbackTargetPosition.z, 0.15f)
                });
            player->SetVerticalVelocity(2.0f);
            player->GetPlayerModel()->PlayRootMotion(anims.airStart, false, true, 0.2f, "Character1_Hips");
        }
        else
        {
            // 地上ダメージ
            /*player->SetPosition({
                Mathf::Lerp(player->GetPosition().x, knockbackTargetPosition.x, 0.25f),
                player->GetPosition().y,
                Mathf::Lerp(player->GetPosition().z, knockbackTargetPosition.z, 0.25f)
                });*/
            player->GetPlayerModel()->PlayRootMotion(anims.normalGround, false, true, 0.1f, "Character1_Hips");
        }
        break;

    case DamageType::Light:
        if (!player->IsGround())
        {
            player->SetGravity(-0.15f);
            player->SetPosition({
                Mathf::Lerp(player->GetPosition().x, knockbackTargetPosition.x, 0.4f),
                player->GetPosition().y,
                Mathf::Lerp(player->GetPosition().z, knockbackTargetPosition.z, 0.4f)
                });
            player->SetVerticalVelocity(2.0f);
            player->GetPlayerModel()->PlayRootMotion(anims.airStart, false, true, 0.2f, "Character1_Hips");
        }
        else
        {
            player->SetPosition({
                Mathf::Lerp(player->GetPosition().x, knockbackTargetPosition.x, 0.3f),
                player->GetPosition().y,
                Mathf::Lerp(player->GetPosition().z, knockbackTargetPosition.z, 0.3f)
                });
            player->GetPlayerModel()->PlayRootMotion(anims.lightGround, false, true, 0.1f, "Character1_Hips");
        }
        break;

    case DamageType::Heavy:
        if (!player->IsGround())
        {
            player->SetGravity(-0.2f);
            player->SetPosition({
                Mathf::Lerp(player->GetPosition().x, knockbackTargetPosition.x, 0.3f),
                player->GetPosition().y,
                Mathf::Lerp(player->GetPosition().z, knockbackTargetPosition.z, 0.3f)
                });
        }
        else
        {
            player->SetPosition({
                Mathf::Lerp(player->GetPosition().x, knockbackTargetPosition.x, 0.2f),
                player->GetPosition().y,
                Mathf::Lerp(player->GetPosition().z, knockbackTargetPosition.z, 0.2f)
                });
            player->SetVerticalVelocity(1.5f);
        }
        player->GetPlayerModel()->PlayRootMotion(anims.heavyGround, false, true, 0.1f, "Character1_Hips");
        break;

    case DamageType::Launch:
    {
        player->SetGravity(-0.1f);
        // 打ち上げアニメーション再生
        player->GetPlayerModel()->PlayRootMotion(anims.airStart, false, true, 0.1f, "Character1_Hips");

        // 水平方向の位置を補間で移動
        player->SetPosition({
            Mathf::Lerp(player->GetPosition().x, knockbackTargetPosition.x, 0.5f),
            player->GetPosition().y,
            Mathf::Lerp(player->GetPosition().z, knockbackTargetPosition.z, 0.5f)
            });

        // 目標高度までの距離を計算
        float targetHeight = knockbackTargetPosition.y;
        float currentHeight = player->GetPosition().y;
        float heightDiff = targetHeight - currentHeight;

        // 打ち上げに必要な初速度を計算
        float launchVelocity = sqrtf(80.0f * fabsf(player->GetGravity()) * heightDiff);
        player->SetVerticalVelocity(launchVelocity);
        break;
    }

    case DamageType::Knockdown:
        // 叩き落としは空中でのみ発動
        if (!player->IsGround())
        {
            // 強制的に下方向に加速
            player->SetGravity(-1.0f);
            player->SetVerticalVelocity(-15.0f);  // 強力な下向きの速度

            // 水平方向にもノックバック
            player->SetPosition({
                Mathf::Lerp(player->GetPosition().x, knockbackTargetPosition.x, 0.4f),
                player->GetPosition().y,
                Mathf::Lerp(player->GetPosition().z, knockbackTargetPosition.z, 0.4f)
                });

            player->GetPlayerModel()->PlayRootMotion(anims.airStart, false, true, 0.1f, "Character1_Hips");
        }
        else
        {
            // 地上にいる場合はHeavyダメージとして処理
            currentDamageType = DamageType::Heavy;
            HandleDamageStart(DamageType::Heavy, elapsedTime);
        }
        break;
    }
}

// 開始処理
void PlayerDamageState::Enter()
{
    currentDamageType = GetCurrentDamageType();
    step = 0;

    GamePad& gamepad = Input::Instance().GetGamePad();
}

// 更新処理
void PlayerDamageState::Update(float elapsedTime)
{
    GamePad& gamepad = Input::Instance().GetGamePad();

    switch (step)
    {
    case 0: // ダメージ開始
        HandleDamageStart(currentDamageType, elapsedTime);
        step++;
        break;

    case 1: // アニメーション再生中
        // Normal/Lightダメージで地上アニメーションが終了した場合
        if ((currentDamageType == DamageType::Normal || currentDamageType == DamageType::Light) &&
            player->IsGround() &&
            !player->GetPlayerModel()->IsPlayAnimation())
        {
            player->SetGravity(-0.3f);
            step = 0;

            // 先行入力チェック
            if (InputAction() == InputActionType::LightAttack)
            {
                ChangeState(PlayerStateId::Combo1);
                return;
            }
            else if (InputAction() == InputActionType::HeavyAttack)
            {
                ChangeState(PlayerStateId::Heavy1);
                return;
            }
            else if (InputAction() == InputActionType::Dodge)
            {
                ChangeState(PlayerStateId::Dodge);
                return;
            }
            else if (InputAction() == InputActionType::Guard)
            {
                ChangeState(PlayerStateId::GuardIdle);
                return;
            }

            ChangeState(PlayerStateId::Idle);
            return;
        }

        // Heavyダメージで地上着地した場合、起き上がりへ
        if (currentDamageType == DamageType::Heavy &&
            player->IsGround() &&
            !player->GetPlayerModel()->IsPlayAnimation())
        {
            player->SetVerticalVelocity(0.0f);
            player->GetPlayerModel()->PlayRootMotion(anims.getUp, false, true, 0.1f, "Character1_Hips");
            step = 4; // 起き上がりステップへ
            break;
        }

        // Knockdownダメージの処理
        if (currentDamageType == DamageType::Knockdown)
        {
            // 水平方向の移動を継続
            player->SetPosition({
                Mathf::Lerp(player->GetPosition().x, knockbackTargetPosition.x, 0.3f),
                player->GetPosition().y,
                Mathf::Lerp(player->GetPosition().z, knockbackTargetPosition.z, 0.3f)
                });

            // 地面に着地したら
            if (player->IsGround())
            {
                player->SetGravity(-0.3f);
                player->SetVerticalVelocity(0.0f);
                player->GetPlayerModel()->PlayRootMotion(anims.airEnd, false, true, 0.1f, "Character1_Hips");
                step = 3; // 着地アニメーションへ
                break;
            }

            // アニメーション終了でループに切り替え
            if (!player->GetPlayerModel()->IsPlayAnimation())
            {
                player->GetPlayerModel()->PlayRootMotion(anims.airLoop, true, true, 0.1f, "Character1_Hips");
            }
        }

        // 空中からのダメージでアニメーションが終了
        if (!player->IsGround() && !player->GetPlayerModel()->IsPlayAnimation() &&
            currentDamageType != DamageType::Knockdown)
        {
            player->GetPlayerModel()->PlayRootMotion(anims.airLoop, true, true, 0.2f, "Character1_Hips");
            step++;
        }

        // Launchダメージの場合の特別処理
        if (currentDamageType == DamageType::Launch)
        {
            // 水平方向の位置を継続的に補間
            player->SetPosition({
                Mathf::Lerp(player->GetPosition().x, knockbackTargetPosition.x, 0.3f),
                player->GetPosition().y,
                Mathf::Lerp(player->GetPosition().z, knockbackTargetPosition.z, 0.3f)
                });

            // 最高高度に到達したか確認
            if (player->GetVelocity().y <= 0.0f)
            {
                player->SetVerticalVelocity(0.0f);
                player->SetGravity(0.0f);
                pauseTimer = 0.3f;
                step = 5; // 打ち上げ専用ステップへ
            }

            // アニメーション終了でループに切り替え
            if (!player->GetPlayerModel()->IsPlayAnimation())
            {
                player->GetPlayerModel()->PlayRootMotion(anims.airLoop, true, true, 0.1f, "Character1_Hips");
            }
        }
        break;

    case 2: // 空中ループ中(着地待ち)
        if (player->IsGround())
        {
            player->GetPlayerModel()->PlayRootMotion(anims.airEnd, false, true, 0.2f, "Character1_Hips");
            step++;
        }
        break;

    case 3: // 着地アニメーション
        if (!player->GetPlayerModel()->IsPlayAnimation())
        {
            player->GetPlayerModel()->PlayRootMotion(anims.getUp, false, true, 0.2f, "Character1_Hips");
            step++;
        }
        break;

    case 4: // 起き上がりアニメーション
        if (!player->GetPlayerModel()->IsPlayAnimation())
        {
            step = 0;
            player->SetGravity(-0.3f);

            // 先行入力チェック
            if (InputAction() == InputActionType::LightAttack)
            {
                ChangeState(PlayerStateId::Combo1);
            }
            else if (InputAction() == InputActionType::HeavyAttack)
            {
                ChangeState(PlayerStateId::Heavy1);
            }
            else if (InputAction() == InputActionType::Dodge)
            {
                ChangeState(PlayerStateId::Dodge);
            }
            else if (InputAction() == InputActionType::Guard)
            {
                ChangeState(PlayerStateId::GuardIdle);
            }
            else if (InputRunMove())
            {
                ChangeState(PlayerStateId::Run);
            }
            else if (InputWalkMove())
            {
                ChangeState(PlayerStateId::Walk);
            }
            else
            {
                ChangeState(PlayerStateId::Idle);
            }
        }
        break;

    case 5: // 打ち上げ専用:最高高度で一時停止
        pauseTimer -= elapsedTime;

        if (pauseTimer <= 0.0f)
        {
            player->SetGravity(-0.3f);
            player->GetPlayerModel()->PlayRootMotion(anims.airLoop, true, true, 0.1f, "Character1_Hips");
            step = 2; // 落下処理へ
        }
        break;
    }

    // ダメージを受けた場合は即座に遷移
    if (player->IsAnyDamage())
    {
        player->SetGravity(-0.3f);
        player->SetVerticalVelocity(0.0f);
        step = 0;
        currentDamageType = GetCurrentDamageType();
        HandleDamageStart(currentDamageType, elapsedTime);
        step = 1;
    }
}

// 終了処理
void PlayerDamageState::Exit()
{
    player->SetGravity(-0.3f);
    step = 0;

    GamePad& gamepad = Input::Instance().GetGamePad();
    gamepad.Vibrate(0.0f, 0.0f);
}

void PlayerDamageState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"統合ダメージ"))
    {
        const char* damageTypeNames[] = { "Normal", "Light", "Heavy", "Launch", "Knockdown" };
        int typeIndex = static_cast<int>(currentDamageType);
        ImGui::Text(u8"現在のダメージタイプ: %s", damageTypeNames[typeIndex]);
        ImGui::Text(u8"ステップ: %d", step);
        ImGui::DragFloat(u8"一時停止タイマー", &pauseTimer, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat3(u8"ノックバック目標位置", &knockbackTargetPosition.x, 0.1f);

        DirectX::XMFLOAT3 damageDir = player->GetDamageDirection();
        ImGui::Text(u8"ダメージ方向: (%.2f, %.2f, %.2f)", damageDir.x, damageDir.y, damageDir.z);

        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 死亡ステートステート
//-------------------------------------------------------------
// コンストラクタ
PlayerDeadState::PlayerDeadState(Player* player)
    : PlayerState(player)
{
    deadAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Hit_Large_Combat_Death_Seq_0");
}

// 開始処理
void PlayerDeadState::Enter()
{
    GamePad& gamePad = Input::Instance().GetGamePad();
    gamePad.Vibrate(0.0f, 0.0f);

    player->GetModel()->PlayRootMotion(deadAnimationIndex, false, false, 0.1f, "Character1_Hips");
    handle = player->deathEffect->Play({ player->GetPosition().x, player->GetPosition().y + 1.0f, player->GetPosition().z }, 0.5f);

    player->SetDeathFlag(true);
}

// 更新処理
void PlayerDeadState::Update(float elapsedTime)
{
    GamePad& gamePad = Input::Instance().GetGamePad();
    Mouse& mouse = Input::Instance().GetMouse();
    player->deathEffect->SetPosition(handle, { player->GetPosition().x, player->GetPosition().y + 1.0f, player->GetPosition().z });

   /* if (gamePad.GetButtonDown() & gamePad.BTN_DOWN)
    {
        player->currentSelection = Player::DeathMenuOption::Continue;
    }
    else if (gamePad.GetButtonDown() & gamePad.BTN_UP)
    {
        player->currentSelection = Player::DeathMenuOption::Exit;
    }

    if (gamePad.GetButtonDown() & gamePad.BTN_A || mouse.GetButtonDown() & mouse.BTN_LEFT)
    {
        switch (player->currentSelection)
        {
        case Player::DeathMenuOption::Continue:
            player->SetDeathFlag(false);
            player->SetHealth(player->GetMaxHealth());
            ChangeState(PlayerStateId::Idle);
            break;

        case Player::DeathMenuOption::Exit:
            if (player->onExitToTitle)
            {
                player->onExitToTitle();
            }
            break;
        default:
            break;
        }
    }*/
}
