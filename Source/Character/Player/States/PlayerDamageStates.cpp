#include "PlayerDamageStates.h"
#include "Math/Mathf.h"

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
    player->GetSword()->ResetAttackState();

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
}
