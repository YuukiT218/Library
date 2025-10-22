#include "PlayerState.h"
#include "Camera/CameraParam.h"
#include "Enemy/SilverDragonkin.h"
#include "Math/Collision.h"
#include "Math/Mathf.h"

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
    player->ChangeState(stateId);
}

// コンボ入力
PlayerState::InputComboType PlayerState::InputCombo()
{
    GamePad& gamepad = Input::Instance().GetGamePad();
    const Mouse& mouse = Input::Instance().GetMouse();

    if (gamepad.GetButtonDown() & GamePad::BTN_A) return InputComboType::Heavy;
    if (gamepad.GetButtonDown() & GamePad::BTN_B || mouse.GetButtonDown() & Mouse::BTN_LEFT) return InputComboType::Light;
    return InputComboType::None;
}

// 回避入力
bool PlayerState::InputDodge() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();

    if (gamepad.GetLAxisPower() != 0 && gamepad.GetButtonDown() & GamePad::BTN_X)
    {
        return true;
    }

    return false;
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

// ジャンプ入力
bool PlayerState::InputJump() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();

    if (gamepad.GetButtonDown() & GamePad::BTN_A)
    {
        return true;
    }

    return false;
}

// ガード入力
bool PlayerState::InputGuard() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();

    // LBが押されている間
    if (gamepad.GetLAxisPower() == 0 && gamepad.GetButton() & GamePad::BTN_X)
    {
        return true;
    }

    return false;
}

// パリィ入力
bool PlayerState::InputGuardParry() const
{
    const GamePad& gamepad = Input::Instance().GetGamePad();
    const Mouse& mouse = Input::Instance().GetMouse();

    // Bボタンが押された瞬間
   /* if (gamepad.GetButtonDown() & GamePad::BTN_B || mouse.GetButtonDown() & Mouse::BTN_LEFT)
    {
        return true;
    }*/

    return false;
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
    player->GetPlayerModel()->PlayAnimation(idleAnimationIndex, true, 0.3f);
}

// 更新処理
void PlayerIdleState::Update(float elapsedTime)
{
    // コンボ1ステートに遷移
    if (InputCombo() == InputComboType::Light)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 回避ステートに遷移
    else if (InputDodge())
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
    else if (InputGuard())
    {
        ChangeState(PlayerStateId::GuardIdle);
    }
    else if (InputJump())
    {
        ChangeState(PlayerStateId::Jump);
    }

    //player->GetPlayerModel()->SetBaseAnimationSpeed(idleAnimationSpeed);
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
    player->GetPlayerModel()->PlayRootMotion(walkFrontAnimationIndex, true, true, 0.1f, "Character1_Hips");
}

// 更新処理
void PlayerWalkState::Update(float elapsedTime)
{
    player->PlayerMove(elapsedTime, 0);

    float speed = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(&player->GetMoveVec())));

    float t = std::clamp(speed, 0.0f, 1.0f);

    walkAnimationSpeed = Mathf::Lerp(0.2f, 0.8f, t);

    // コンボ1ステートに遷移
    if (InputCombo() == InputComboType::Light)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 回避ステートに遷移
    else if (InputDodge())
    {
        ChangeState(PlayerStateId::Dodge);
    }
    // ガード待機ステート遷移
    else if (InputGuard())
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
    else if (InputJump())
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
    player->GetPlayerModel()->PlayRootMotion(runStartAnimationIndex, false, true, 0.1f, "Character1_Hips");
}

// 更新処理
void PlayerRunState::Update(float elapsedTime)
{
	player->PlayerMove(elapsedTime, 0);
    if (!player->GetPlayerModel()->IsPlayAnimation())
    {
        player->GetPlayerModel()->PlayRootMotion(runLoopAnimationIndex, true, true, 0.1f, "Character1_Hips");
    }

    // コンボ1ステートに遷移
    if (InputCombo() == InputComboType::Light)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 回避ステートに遷移
    else if (InputDodge())
    {
        ChangeState(PlayerStateId::Dodge);
    }
    // ガード待機ステート遷移
    else if (InputGuard())
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
        ChangeState(PlayerStateId::Idle);
    }
	else if (InputJump())
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
    if (InputCombo() == InputComboType::Light)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 回避ステートに遷移
    else if (InputDodge())
    {
        ChangeState(PlayerStateId::Dodge);
    }
    else if (InputGuard())
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
    if (InputCombo() == InputComboType::Light)
    {
        ChangeState(PlayerStateId::Combo1);
    }
    // 回避ステートに遷移
    else if (InputDodge())
    {
        ChangeState(PlayerStateId::Dodge);
    }
    else if (InputGuard())
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
        player->GetPlayerModel()->SetAnimationSpeed(4.5f);
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
    timer -= elapsedTime;
    if (player->IsGround() && timer < 0.633f || !player->IsGround() && timer <= 0.083f)
    {
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

        if (InputCombo() != InputComboType::None)
        {
            nextShiftReady = true;
        }

        if (nextShiftReady)
        {
            if (player->IsGround() && timer < 0.8f || !player->IsGround() && timer <= 0.083f)
            {
                if (InputDodge())
                {
                    ChangeState(PlayerStateId::Dodge);
                }
                if (InputCombo() == InputComboType::Light)
                {
                    ChangeState(PlayerStateId::Combo1);
                }
            }
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
// 回避攻撃ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerDodgeAttackState::PlayerDodgeAttackState(Player* player)
    : PlayerState(player)
{
    dodgeAttackAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorAttackDodge");
}

// 開始処理
void PlayerDodgeAttackState::Enter()
{
    DirectX::XMVECTOR Vec;

    player->GetPlayerModel()->PlayAnimation(dodgeAttackAnimationIndex, false, 0.1f);
    timer = 0.95f;

    Vec = DirectX::XMLoadFloat3(&player->CharacterForward(player->GetAngle()));
    Vec = DirectX::XMVector3Normalize(Vec);

    // 回避
    Vec = DirectX::XMVectorScale(Vec, 20.0f);

    DirectX::XMFLOAT3 vec;
    DirectX::XMStoreFloat3(&vec, Vec);

    player->SetMovement(vec, 2.0f);
}

// 更新処理
void PlayerDodgeAttackState::Update(float elapsedTime)
{
    /*int index = player->GetPlayerModel()->GetCurrentAnimationIndex();
    AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig("Player", index);
    player->GetWeaponRight()->AttackAnimationCollision(player->GetModel(), config);*/

    timer -= elapsedTime;
    if (timer <= 0.0f)
    {
        // 走りステート遷移
        if (InputRunMove())
        {
            ChangeState(PlayerStateId::Run);
        }
        // 歩きステート遷移
        else if (InputWalkMove())
        {
            ChangeState(PlayerStateId::Walk);
        }
        // 待機ステート遷移
        else
        {
            ChangeState(PlayerStateId::Idle);
        }
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
    if (player->IsGround())
    {
        if (player->calcTargetDist(player->GetPosition(), SilverDragonkin::Instance().GetPosition()) > 5.0f
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
        if (player->calcTargetDist(player->GetPosition(), SilverDragonkin::Instance().GetPosition()) > 5.0f
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
        player->LockOnTurnToEnemy(elapsedTime);
    }

    player->GetSword()->AttackAnimationCollision(player->GetModel(), config);
    //player->GetSword()->AttackAnimationCollision(player->GetModel(), attackCollisionStartFrame, attackCollisionEndFrame, attackDamage, invisibleTime, attackLeftVibrate, attackRightVibrate, attackHitStopTime, attackHitStopSpeed);
    
    InputComboType input = InputCombo();

    // 次のコンボステート処理
    if (inputToNextState.count(input))
    {
        // 先行入力処理
        if (input != InputComboType::None)
        {
            if (input == InputComboType::Heavy && !player->IsGround())
                return;
            if (frame <= nextShiftFrame)
            {
                nextShiftReady = true;
                nextInput = input;
            }
        }
    }

    // 次のコンボステートへ遷移
    if (nextShiftReady)
    {
        if (frame >= nextShiftFrame)
        {
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

    inputToNextState[InputComboType::Light] = PlayerStateId::Combo2;
    inputToNextState[InputComboType::Heavy] = PlayerStateId::Heavy1;
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
    inputToNextState[InputComboType::Light] = PlayerStateId::Combo3;
    inputToNextState[InputComboType::Heavy] = PlayerStateId::Heavy1;
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
    inputToNextState[InputComboType::Light] = PlayerStateId::Combo4;
    inputToNextState[InputComboType::Heavy] = PlayerStateId::Heavy1;
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
    inputToNextState[InputComboType::Light] = PlayerStateId::Heavy1;
    //inputToNextState[InputComboType::Heavy] = PlayerStateId::Heavy1;
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
// 強攻撃1ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerHeavyAttack1State::PlayerHeavyAttack1State(Player* player)
    : PlayerComboState(player)
{
    inputToNextState[InputComboType::Light] = PlayerStateId::Combo1;
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
// 強攻撃2ステート
//-------------------------------------------------------------
// コンストラクタ
PlayerHeavyAttack2State::PlayerHeavyAttack2State(Player* player)
    : PlayerComboState(player)
{
    poseFrame = 0.55f;
    endFrame = 0.816f;
    comboAttackSpeed = 1.0f;
    comboPoseSpeed = 1.0f;
    comboAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorAttackHeavy2");

    forwardFrame = 0.0f;
    forwardPower = 13.0f;
    moveRate = 0.5f;
    turnRate = 0.5f;

    // 攻撃判定必要変数
    attackCollisionStartFrame = 0.23f;
    attackCollisionEndFrame = 0.45f;
    attackDamage = 10.0f;
    invisibleTime = 0.5f;

    // コントローラーの振動変数
    attackLeftVibrate = 1.0f;
    attackRightVibrate = 1.0f;

    // 攻撃時ヒットストップ変数
    attackHitStopTime = 0.0f;
    attackHitStopSpeed = 0.1f;
}

void PlayerHeavyAttack2State::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"強攻撃2"))
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

    //player->GetPlayerModel()->PlayRootMotion(guardStartAnimationIndex, false, true, 0.1f, "Character1_Reference");
    player->GetPlayerModel()->PlayRootMotion(guardLoopAnimationIndex, false, true, 0.2f, "Character1_Reference");

    player->SetPlayerGuard(true);
    timer = 1.7f;
}

// 更新処理
void PlayerGuardIdle::Update(float elapsedTime)
{
    timer -= elapsedTime;

    if (CameraParam::Instance().GetIsLockOn())
    {
        player->LockOnTurnToEnemy(elapsedTime);
    }

    if (timer <= 0.0f)
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
    else if (timer >= 0.8f)
    {
	    player->GetPlayerModel()->PlayRootMotion(guardEndAnimationIndex, false, true, 0.1f, "Character1_Reference");
    	player->SetPlayerGuard(false);
    }
}

// 終了処理
void PlayerGuardIdle::Exit()
{
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
// ガード歩きステート
//-------------------------------------------------------------
// コンストラクタ
PlayerGuardWalk::PlayerGuardWalk(Player* player)
    : PlayerState(player)
{
    guardFrontWalkAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorGuardWalkForward");
    guardBackWalkAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorGuardWalkBackward");
    guardRightWalkAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorGuardWalkRightward");
    guardLeftWalkAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorGuardWalkLeftward");
}

// 開始処理
void PlayerGuardWalk::Enter()
{
    player->SetPlayerGuard(true);
}

// 更新処理
void PlayerGuardWalk::Update(float elapsedTime)
{
    LockOnStrafe(guardRightWalkAnimationIndex, guardLeftWalkAnimationIndex, guardFrontWalkAnimationIndex, guardBackWalkAnimationIndex);

    player->PlayerMove(elapsedTime, guardWalkAnimationMoveRate);

    // 待機ステート遷移
    if (!InputGuard())
    {
        ChangeState(PlayerStateId::Idle);
    }
    // ガードパリィステート遷移
    else if (InputGuard() && InputGuardParry())
    {
        ChangeState(PlayerStateId::GuardParry);
    }
    // ガード待機ステート遷移
    else if (InputGuard() && !InputWalkMove() && !InputRunMove())
    {
        ChangeState(PlayerStateId::GuardIdle);
    }
    // 回避ステート遷移
    else if (InputDodge())
    {
        ChangeState(PlayerStateId::Dodge);
    }

    //player->GetPlayerModel()->SetBaseAnimationSpeed(guardWalkAnimationSpeed);
}

// 終了処理
void PlayerGuardWalk::Exit()
{
    player->SetPlayerGuard(false);
}

// デバッグ用GUI描画
void PlayerGuardWalk::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"ガード歩き"))
    {
        //ImGui::Text(player->GetPlayerModel()->GetAnimationName(guardFrontWalkAnimationIndex));
        //ImGui::SameLine();
        //ImGui::Text(u8"終了フレーム:%.3f", player->GetPlayerModel()->GetAnimationLength(guardFrontWalkAnimationIndex));
        ImGui::DragFloat(u8"ガード歩き移動率", &guardWalkAnimationMoveRate, 0.01f, 0.0f);
        ImGui::DragFloat(u8"アニメーションスピード", &guardWalkAnimationSpeed, 0.01f, 0.0f, 5.0f);
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
    guardHitAnimationIndex = player->GetModel()->GetAnimationIndex("WarriorGuardHit2");
}

void PlayerGuardHit::Enter()
{
    player->GetModel()->PlayAnimation(guardHitAnimationIndex, false, 0.1f);

    player->SetPlayerGuard(true);

    GamePad& gamepad = Input::Instance().GetGamePad();

    gamepad.Vibrate(0.7f, 0.7f);

    timer = 0.567f;
}

void PlayerGuardHit::Update(float elapsedTime)
{
    GamePad& gamepad = Input::Instance().GetGamePad();

    timer -= elapsedTime;

    if (timer <= 0.0f)
    {
        // コンボ1ステートに遷移
        if (InputCombo() == InputComboType::Light)
        {
            ChangeState(PlayerStateId::Combo1);
        }
        // 強攻撃1ステートに遷移
        else if (InputCombo() == InputComboType::Heavy)
        {
            ChangeState(PlayerStateId::Heavy1);
        }
        // 回避ステートに遷移
        else if (InputDodge())
        {
            ChangeState(PlayerStateId::Dodge);
        }
        // ガード待機ステート遷移
        else if (InputGuard())
        {
            ChangeState(PlayerStateId::GuardIdle);
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
        // アイドルステートに遷移
        else if (!InputWalkMove() && !InputRunMove())
        {
            ChangeState(PlayerStateId::Idle);
        }
    }
    else if (timer <= 0.4f)
    {
        gamepad.Vibrate(0.0f, 0.0f);
    }
}

// 終了処理
void PlayerGuardHit::Exit()
{
    player->SetPlayerGuard(false);
}

//-------------------------------------------------------------
// ガードパリィステート
//-------------------------------------------------------------
// コンストラクタ
PlayerGuardParry::PlayerGuardParry(Player* player)
    : PlayerState(player)
{
    guardParryAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorParry");
}

void PlayerGuardParry::Enter()
{
    player->GetPlayerModel()->PlayAnimation(guardParryAnimationIndex, false, 0.1f);
    timer = 0.5f;

    player->SetPlayerParry(true);
}

void PlayerGuardParry::Update(float elapsedTime)
{
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();
    //AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig("Player", index);
    //player->GetLeftShield()->ParryAnimationCollision(player->GetModel(), config);

    timer -= elapsedTime;
    if (timer <= 0.0f)
    {
        player->SetPlayerParry(false);

        // 待機ステート遷移
        if (!InputGuard())
        {
            ChangeState(PlayerStateId::Idle);
        }
        // ガード歩きステート遷移
        else if (InputGuard() && (InputWalkMove() || InputRunMove()))
        {
            ChangeState(PlayerStateId::GuardWalk);
        }
        // ガード待機ステート遷移
        else if (InputGuard())
        {
            ChangeState(PlayerStateId::GuardIdle);
        }
    }

    //  player->GetPlayerModel()->SetBaseAnimationSpeed(guardParryAnimationSpeed);
}

// デバッグ用GUI描画
void PlayerGuardParry::DrawDebugGUI()
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
    damageAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorHit2");
}

// 開始処理
void PlayerDamageState::Enter()
{
    player->GetModel()->PlayAnimation(damageAnimationIndex, false, 0.1f);

    GamePad& gamepad = Input::Instance().GetGamePad();

    gamepad.Vibrate(0.3f, 0.3f);
}

// 更新処理
void PlayerDamageState::Update(float elapsedTime)
{
    GamePad& gamepad = Input::Instance().GetGamePad();

    if (!player->GetModel()->IsPlayAnimation())
    {
        gamepad.Vibrate(0.0f, 0.0f);

        // コンボ1ステートに遷移
        if (InputCombo() == InputComboType::Light)
        {
            ChangeState(PlayerStateId::Combo1);
        }
        // 強攻撃1ステートに遷移
        else if (InputCombo() == InputComboType::Heavy)
        {
            ChangeState(PlayerStateId::Heavy1);
        }
        // 回避ステートに遷移
        else if (InputDodge())
        {
            ChangeState(PlayerStateId::Dodge);
        }
        // ガード待機ステート遷移
        else if (InputGuard())
        {
            ChangeState(PlayerStateId::GuardIdle);
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
        // アイドルステートに遷移
        else if (!InputWalkMove() && !InputRunMove())
        {
            ChangeState(PlayerStateId::Idle);
        }
    }
}

//-------------------------------------------------------------
// 死亡ステートステート
//-------------------------------------------------------------
// コンストラクタ
PlayerDeadState::PlayerDeadState(Player* player)
    : PlayerState(player)
{
    deadAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("WarriorDeath");
}

// 開始処理
void PlayerDeadState::Enter()
{
    GamePad& gamePad = Input::Instance().GetGamePad();
    gamePad.Vibrate(0.0f, 0.0f);

    player->GetModel()->PlayAnimation(deadAnimationIndex, false, 0.1f);

    player->SetDeathFlag(true);
}

// 更新処理
void PlayerDeadState::Update(float elapsedTime)
{
    GamePad& gamePad = Input::Instance().GetGamePad();
    Mouse& mouse = Input::Instance().GetMouse();

    if (gamePad.GetButtonDown() & gamePad.BTN_DOWN)
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
    }
}
