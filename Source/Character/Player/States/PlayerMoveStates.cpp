#include "PlayerMoveStates.h"
#include "Math/Mathf.h"

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
}

// デバッグ用GUI描画
void PlayerWalkState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"歩き"))
    {
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
}

// デバッグ用GUI描画
void PlayerRunState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"走り"))
    {
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
}

// デバッグ用GUI描画
void PlayerJumpState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"ジャンプ"))
    {
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
}

// デバッグ用GUI描画
void PlayerFallState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"落下"))
    {
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
        ImGui::DragFloat(u8"アニメーション遷移時間", &dodgeBackAnimationTime, 0.01f, 0.0f, 5.0f);
        ImGui::DragInt(u8"移動距離", &dodgeBackMovePow);
        ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGui::TreeNode(u8"ローリング回避"))
    {
        ImGui::DragFloat(u8"アニメーション遷移時間", &dodgeAnimationTime, 0.01f, 0.0f, 5.0f);
        ImGui::DragInt(u8"移動距離", &rollingFrontMovePow);
        ImGui::TreePop();
    }
}
