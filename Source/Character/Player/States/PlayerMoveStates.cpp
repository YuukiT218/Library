#include "PlayerMoveStates.h"
#include "Math/Mathf.h"

namespace
{
    // 走り・歩き開始時のブレンド時間
    constexpr float MOVE_START_BLEND_SECONDS = 0.4f;

    // 歩きアニメーション速度の範囲（スティックの傾きで補間）
    constexpr float WALK_ANIMATION_SPEED_MIN = 0.2f;
    constexpr float WALK_ANIMATION_SPEED_MAX = 0.8f;
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
    player->GetPlayerModel()->PlayAnimation(idleAnimationIndex, true, DEFAULT_BLEND_SECONDS);
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
    player->GetPlayerModel()->PlayRootMotion(walkFrontAnimationIndex, true, true, MOVE_START_BLEND_SECONDS, HIPS_NODE_NAME);
}

// 更新処理
void PlayerWalkState::Update(float elapsedTime)
{
    player->PlayerMove(elapsedTime, 0.0f);

    float speed = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(DirectX::XMLoadFloat3(&player->GetMoveVec())));

    float t = std::clamp(speed, 0.0f, 1.0f);

    walkAnimationSpeed = Mathf::Lerp(WALK_ANIMATION_SPEED_MIN, WALK_ANIMATION_SPEED_MAX, t);

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
    player->GetPlayerModel()->PlayRootMotion(runStartAnimationIndex, false, true, MOVE_START_BLEND_SECONDS, HIPS_NODE_NAME);
}

// 更新処理
void PlayerRunState::Update(float elapsedTime)
{
    player->PlayerMove(elapsedTime, 0.0f);
    if (!player->GetPlayerModel()->IsPlayAnimation())
    {
        player->GetPlayerModel()->PlayRootMotion(runLoopAnimationIndex, true, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
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
        player->GetPlayerModel()->PlayRootMotion(runEndAnimationIndex, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
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
        player->GetPlayerModel()->PlayRootMotion(jumpAnimationIndex, false, true, DEFAULT_BLEND_SECONDS, REFERENCE_NODE_NAME);
        player->PlayerJump(jumpPower);
    }
}

// 更新処理
void PlayerJumpState::Update(float elapsedTime)
{
    player->PlayerMove(elapsedTime, jumpAnimationMoveRate);
    if (!player->GetPlayerModel()->IsPlayAnimation())
    {
        player->GetPlayerModel()->PlayRootMotion(fallAnimationIndex, true, true, DEFAULT_BLEND_SECONDS, REFERENCE_NODE_NAME);
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
        player->GetPlayerModel()->PlayRootMotion(fallAnimationIndex, true, true, DEFAULT_BLEND_SECONDS, REFERENCE_NODE_NAME);
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

    // ワールド進行方向を取得
    DirectX::XMFLOAT3 forward = Character::CharacterForward(player->GetAngle());
    DirectX::XMFLOAT3 vec;
    DirectX::XMStoreFloat3(&vec, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&forward)));

    if (player->IsGround())
    {
        player->GetPlayerModel()->PlayRootMotion(dodgeAnimationIndex, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
        timer = dodgeAnimationTime;
    }
    else
    {
        player->SetGravity(HOVER_GRAVITY);
        player->SetVerticalVelocity(0.0f);
        player->GetPlayerModel()->PlayRootMotion(airDodgeAnimationIndex, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
        timer = airDodgeAnimationTime;
    }

    player->SetRolling(true);
    nextShiftReady = false;

    player->SetMovement(vec, static_cast<float>(rollingFrontMovePower));
}


// 更新処理
void PlayerDodgeState::Update(float elapsedTime)
{
    float frame = player->GetPlayerModel()->GetCurrentAnimationSeconds();
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();

    AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig(ANIMATION_CONFIG_OWNER, index);

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
    player->SetGravity(Character::DEFAULT_GRAVITY);
    player->SetRolling(false);
    nextShiftReady = false;
}

// デバッグ用GUI描画
void PlayerDodgeState::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"バックステップ回避"))
    {
        ImGui::DragFloat(u8"アニメーション遷移時間", &dodgeBackAnimationTime, 0.01f, 0.0f, 5.0f);
        ImGui::DragInt(u8"移動距離", &dodgeBackMovePower);
        ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGui::TreeNode(u8"ローリング回避"))
    {
        ImGui::DragFloat(u8"アニメーション遷移時間", &dodgeAnimationTime, 0.01f, 0.0f, 5.0f);
        ImGui::DragInt(u8"移動距離", &rollingFrontMovePower);
        ImGui::TreePop();
    }
}
