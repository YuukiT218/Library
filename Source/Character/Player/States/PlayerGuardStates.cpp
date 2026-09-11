#include "PlayerGuardStates.h"
#include "Math/Mathf.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Camera/CameraParam.h"

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
            player->SetGravity(-0.3f);
            player->GetPlayerModel()->PlayRootMotion(guardEndAnimationIndex, false, true, 0.1f, "Character1_Reference");
            player->SetPlayerCounter(false);
            player->SetPlayerGuard(false);
        }
    }
}

// 終了処理
void PlayerGuardIdle::Exit()
{
    isLoop = false;
}

// デバッグ用GUI描画
void PlayerGuardIdle::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"ガード待機"))
    {
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
    player->SetGravity(-0.3f);

    // 距離に応じたアニメーション分岐
    if (dist > 4.0f)
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

    // 先行入力をリセット（前回のカウンター時の入力を持ち越さない）
    nextShiftReady = false;
    nextInput = InputActionType::None;
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
            // 先行入力は一度きり。消費したらクリアしてから遷移する
            InputActionType input = nextInput;
            nextShiftReady = false;
            nextInput = InputActionType::None;

            if (input == InputActionType::LightAttack)
            {
                ChangeState(PlayerStateId::Combo1);
                return;
            }
            else if (input == InputActionType::HeavyAttack)
            {
                ChangeState(PlayerStateId::Heavy1);
                return;
            }
            else if (input == InputActionType::Dodge)
            {
                ChangeState(PlayerStateId::Dodge);
                return;
            }
            else if (input == InputActionType::Guard)
            {
                ChangeState(PlayerStateId::GuardIdle);
                return;
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

    // 先行入力をリセット
    nextShiftReady = false;
    nextInput = InputActionType::None;
}

// デバッグ用GUI描画
void PlayerGuardCounter::DrawDebugGUI()
{
    ImGui::Separator();

    if (ImGui::TreeNode(u8"ガードパリィ"))
    {
        ImGui::DragFloat(u8"アニメーションスピード", &guardParryAnimationSpeed, 0.01f, 0.0f, 5.0f);
        bool parryFlag = player->GetPlayerIsParry();
        ImGui::Checkbox(u8"パリィ判定があるか", &parryFlag);
        ImGui::TreePop();
    }
}
