#include "PlayerGuardStates.h"
#include "Math/Mathf.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Camera/CameraParam.h"

namespace
{
    // ガードループへ切り替える時のブレンド時間
    constexpr float GUARD_LOOP_BLEND_SECONDS = 0.2f;

    // ガード受付時間を表すアニメーション属性のインデックス
    constexpr int GUARD_WINDOW_ATTRIBUTE_INDEX = 0;

    // ガード成功エフェクトの大きさ
    constexpr float GUARD_EFFECT_SCALE = 0.2f;

    // カウンター攻撃を突進攻撃にする敵との距離
    constexpr float COUNTER_RUSH_MIN_DISTANCE = 4.0f;

    // カウンター攻撃を密着攻撃にする敵との距離
    constexpr float COUNTER_CLOSE_MAX_DISTANCE = 2.0f;

    // 敵の周りを回り込まないよう、これより近いときは旋回しない
    constexpr float COUNTER_TURN_MIN_DISTANCE = 2.0f;

    // 突進攻撃中、敵にこれ以上離れていれば吸い寄せる
    constexpr float COUNTER_HOMING_MIN_DISTANCE = 6.0f;

    // 突進攻撃中の吸い寄せ速度
    constexpr float COUNTER_HOMING_LERP_SPEED = 5.0f;
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
    player->SetGravity(HOVER_GRAVITY);
    player->SetVerticalVelocity(0.0f);
    player->SetHorizonVelocity(0.0f);

    if (!player->IsGuard())
    {
        player->ResetTurnCompleted();  // プレイヤーの旋回完了フラグをリセット
        player->GetPlayerModel()->PlayRootMotion(guardStartAnimationIndex, false, true, DEFAULT_BLEND_SECONDS, REFERENCE_NODE_NAME);
    }

    player->SetGuard(true);
}

// 更新処理
void PlayerGuardIdle::Update(float elapsedTime)
{
    if (!player->GetPlayerModel()->IsPlayAnimation())
    {
        player->GetPlayerModel()->PlayRootMotion(guardLoopAnimationIndex, false, true, GUARD_LOOP_BLEND_SECONDS, REFERENCE_NODE_NAME);
        index = player->GetPlayerModel()->GetCurrentAnimationIndex();
        config = player->GetPlayerModel()->GetAnimationConfig(ANIMATION_CONFIG_OWNER, index);
        isLoop = true;
        timer = 0.0f;
    }

    if (CameraParam::Instance().IsLockOn())
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
        if (player->IsStandbyCounter() && InputAction() == InputActionType::LightAttack)
        {
            player->SetStandbyCounter(false);
            player->SetGuard(false);
            ChangeState(PlayerStateId::GuardCounter);
        }
        else if (player->IsStandbyCounter() && InputAction() == InputActionType::Dodge)
        {
            player->SetStandbyCounter(false);
            player->SetGuard(false);
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
        else if (config->attributes[GUARD_WINDOW_ATTRIBUTE_INDEX].endTime <= frame)
        {
            player->SetGravity(Character::DEFAULT_GRAVITY);
            player->GetPlayerModel()->PlayRootMotion(guardEndAnimationIndex, false, true, DEFAULT_BLEND_SECONDS, REFERENCE_NODE_NAME);
            player->SetStandbyCounter(false);
            player->SetGuard(false);
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
    player->GetModel()->PlayRootMotion(guardHitAnimationIndex, false, true, DEFAULT_BLEND_SECONDS, REFERENCE_NODE_NAME);

    player->guardEffect->Play(player->GetPosition(), GUARD_EFFECT_SCALE);

    player->SetGuard(true);
    player->SetStandbyCounter(true);
    player->ResetTurnCompleted();
}

void PlayerGuardHit::Update(float elapsedTime)
{
    float frame = player->GetPlayerModel()->GetCurrentAnimationSeconds();
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();
    AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig(ANIMATION_CONFIG_OWNER, index);

    if (!player->IsTurnCompleted())
    {
        player->LockOnTurnToEnemy(elapsedTime);
    }

    if (config->advanceInputEndFrame <= frame)
    {
        // ガードカウンターステートに遷移
        if (InputAction() == InputActionType::LightAttack)
        {
            player->SetStandbyCounter(false);
            player->SetGuard(false);
            ChangeState(PlayerStateId::GuardCounter);
        }
        // 回避ステートに遷移
        else if (InputAction() == InputActionType::Dodge)
        {
            player->SetStandbyCounter(false);
            player->SetGuard(false);
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
    counterMiddleAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Parry_Counter_Attack_R_Seq_0");
    counterCloseAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Combo_Attack_Wave_05_04_Seq_0");
    counterRushAnimationIndex = player->GetPlayerModel()->GetAnimationIndex("Run_Attack_02_Seq_0");
}

void PlayerGuardCounter::Enter()
{
    // 敵との距離を計算（XZ平面距離）
    DirectX::XMFLOAT3 playerPos = player->GetPosition();
    enemyPos = EnemyBoss::Instance().GetPosition();
    float diffX = playerPos.x - enemyPos.x;
    float diffZ = playerPos.z - enemyPos.z;
    float dist = sqrtf(diffX * diffX + diffZ * diffZ);
    player->SetGravity(Character::DEFAULT_GRAVITY);

    // 距離に応じたアニメーション分岐
    if (dist > COUNTER_RUSH_MIN_DISTANCE)
    {
        // 遠距離：突進攻撃 (Run Attack)
        player->GetPlayerModel()->PlayRootMotion(counterRushAnimationIndex, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
    }
    else if (dist < COUNTER_CLOSE_MAX_DISTANCE)
    {
        // 近距離：密着攻撃 (Combo Attack Wave)
        player->GetPlayerModel()->PlayRootMotion(counterCloseAnimationIndex, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
    }
    else
    {
        // 中距離：通常カウンター
        player->GetPlayerModel()->PlayRootMotion(counterMiddleAnimationIndex, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
    }

    player->SetRolling(true);
    player->ResetTurnCompleted();  // プレイヤーの旋回完了フラグをリセット

    // 先行入力をリセット（前回のカウンター時の入力を持ち越さない）
    nextShiftReady = false;
    nextInput = InputActionType::None;
}

void PlayerGuardCounter::Update(float elapsedTime)
{
    float frame = player->GetPlayerModel()->GetCurrentAnimationSeconds();
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();

    AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig(ANIMATION_CONFIG_OWNER, index);

    player->GetSword()->AttackAnimationCollision(player->GetModel(), config, player);

    if (CameraParam::Instance().IsLockOn())
    {
        // 距離計算（XZ平面距離）
        DirectX::XMFLOAT3 playerPos = player->GetPosition();
        float diffX = playerPos.x - enemyPos.x;
        float diffZ = playerPos.z - enemyPos.z;
        float dist = sqrtf(diffX * diffX + diffZ * diffZ);

        // オービット対策
        if (dist >= COUNTER_TURN_MIN_DISTANCE)
        {
            if (!player->IsTurnCompleted())
            {
                player->LockOnTurnToEnemy(elapsedTime);
            }
        }

        // --- 遠距離突進攻撃(Run_Attack_02)の場合のホーミング処理 ---
        if (index == counterRushAnimationIndex && frame <= config->advanceInputEndFrame)
        {
            if (dist >= COUNTER_HOMING_MIN_DISTANCE)
            {
                float lerpRate = COUNTER_HOMING_LERP_SPEED * elapsedTime;
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
    player->SetRolling(false);

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
        bool parryFlag = player->IsParry();
        ImGui::Checkbox(u8"パリィ判定があるか", &parryFlag);
        ImGui::TreePop();
    }
}
