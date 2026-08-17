#include "PlayerAttackStates.h"
#include "Character/Player/Player.h"            
#include "Character/Enemy/EnemyBoss.h" 
#include "Camera/CameraParam.h"
#include "Math/Mathf.h"

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
    attackDamage = 10;
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
    attackDamage = 10;
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
    attackDamage = 10;
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
    attackDamage = 10;
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
    attackDamage = 10;
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
    attackDamage = 10;
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
