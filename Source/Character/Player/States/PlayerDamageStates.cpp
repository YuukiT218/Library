#include "PlayerDamageStates.h"
#include "Math/Mathf.h"

namespace
{
    // 空中で被弾した時のブレンド時間
    constexpr float AIR_DAMAGE_BLEND_SECONDS = 0.2f;

    // 空中で被弾した時の重力と浮き上がり速度
    constexpr float AIR_DAMAGE_GRAVITY = -0.15f;
    constexpr float AIR_DAMAGE_RISE_VELOCITY = 2.0f;

    // 重攻撃を空中で受けた時の重力
    constexpr float AIR_HEAVY_DAMAGE_GRAVITY = -0.2f;

    // 重攻撃を地上で受けた時の浮き上がり速度
    constexpr float GROUND_HEAVY_DAMAGE_RISE_VELOCITY = 1.5f;

    // 打ち上げ中の重力
    constexpr float LAUNCH_GRAVITY = -0.1f;

    // 打ち上げ初速の計算係数（v = sqrt(係数 * |g| * h)）
    constexpr float LAUNCH_VELOCITY_COEFFICIENT = 80.0f;

    // 打ち上げの最高到達点で停止する時間
    constexpr float LAUNCH_APEX_PAUSE_SECONDS = 0.3f;

    // 叩き落とし中の重力と落下速度
    constexpr float KNOCKDOWN_GRAVITY = -1.0f;
    constexpr float KNOCKDOWN_FALL_VELOCITY = -15.0f;

    // ノックバック目標位置へ寄せる補間率
    constexpr float NORMAL_AIR_KNOCKBACK_LERP = 0.15f;
    constexpr float LIGHT_AIR_KNOCKBACK_LERP = 0.4f;
    constexpr float LIGHT_GROUND_KNOCKBACK_LERP = 0.3f;
    constexpr float HEAVY_AIR_KNOCKBACK_LERP = 0.3f;
    constexpr float HEAVY_GROUND_KNOCKBACK_LERP = 0.2f;
    constexpr float LAUNCH_START_KNOCKBACK_LERP = 0.5f;
    constexpr float KNOCKDOWN_START_KNOCKBACK_LERP = 0.4f;
    constexpr float CONTINUOUS_KNOCKBACK_LERP = 0.3f;

    // 死亡エフェクトの表示位置の高さと大きさ
    constexpr float DEATH_EFFECT_OFFSET_Y = 1.0f;
    constexpr float DEATH_EFFECT_SCALE = 0.5f;
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

// ノックバック目標位置へ水平方向に寄せる
void PlayerDamageState::MoveTowardKnockbackTarget(float lerpRate)
{
    const DirectX::XMFLOAT3& position = player->GetPosition();
    player->SetPosition({
        Mathf::Lerp(position.x, knockbackTargetPosition.x, lerpRate),
        position.y,
        Mathf::Lerp(position.z, knockbackTargetPosition.z, lerpRate)
        });
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
    player->SetKnockdownDamage(false);
    player->GetSword()->ResetAttackState();

    // ノックバック目標位置を計算
    knockbackTargetPosition = GetKnockbackPosition(type);

    switch (type)
    {
    case DamageType::Normal:
        if (!player->IsGround())
        {
            // 空中ダメージ
            player->SetGravity(AIR_DAMAGE_GRAVITY);
            MoveTowardKnockbackTarget(NORMAL_AIR_KNOCKBACK_LERP);
            player->SetVerticalVelocity(AIR_DAMAGE_RISE_VELOCITY);
            player->GetPlayerModel()->PlayRootMotion(anims.airStart, false, true, AIR_DAMAGE_BLEND_SECONDS, HIPS_NODE_NAME);
        }
        else
        {
            // 地上ダメージ
            player->GetPlayerModel()->PlayRootMotion(anims.normalGround, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
        }
        break;

    case DamageType::Light:
        if (!player->IsGround())
        {
            player->SetGravity(AIR_DAMAGE_GRAVITY);
            MoveTowardKnockbackTarget(LIGHT_AIR_KNOCKBACK_LERP);
            player->SetVerticalVelocity(AIR_DAMAGE_RISE_VELOCITY);
            player->GetPlayerModel()->PlayRootMotion(anims.airStart, false, true, AIR_DAMAGE_BLEND_SECONDS, HIPS_NODE_NAME);
        }
        else
        {
            MoveTowardKnockbackTarget(LIGHT_GROUND_KNOCKBACK_LERP);
            player->GetPlayerModel()->PlayRootMotion(anims.lightGround, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
        }
        break;

    case DamageType::Heavy:
        if (!player->IsGround())
        {
            player->SetGravity(AIR_HEAVY_DAMAGE_GRAVITY);
            MoveTowardKnockbackTarget(HEAVY_AIR_KNOCKBACK_LERP);
        }
        else
        {
            MoveTowardKnockbackTarget(HEAVY_GROUND_KNOCKBACK_LERP);
            player->SetVerticalVelocity(GROUND_HEAVY_DAMAGE_RISE_VELOCITY);
        }
        player->GetPlayerModel()->PlayRootMotion(anims.heavyGround, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
        break;

    case DamageType::Launch:
    {
        player->SetGravity(LAUNCH_GRAVITY);
        // 打ち上げアニメーション再生
        player->GetPlayerModel()->PlayRootMotion(anims.airStart, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);

        // 水平方向の位置を補間で移動
        MoveTowardKnockbackTarget(LAUNCH_START_KNOCKBACK_LERP);

        // 目標高度までの距離を計算
        float targetHeight = knockbackTargetPosition.y;
        float currentHeight = player->GetPosition().y;
        float heightDiff = targetHeight - currentHeight;

        // 打ち上げに必要な初速度を計算
        float launchVelocity = sqrtf(LAUNCH_VELOCITY_COEFFICIENT * fabsf(player->GetGravity()) * heightDiff);
        player->SetVerticalVelocity(launchVelocity);
        break;
    }

    case DamageType::Knockdown:
        // 叩き落としは空中でのみ発動
        if (!player->IsGround())
        {
            // 強制的に下方向に加速
            player->SetGravity(KNOCKDOWN_GRAVITY);
            player->SetVerticalVelocity(KNOCKDOWN_FALL_VELOCITY);

            // 水平方向にもノックバック
            MoveTowardKnockbackTarget(KNOCKDOWN_START_KNOCKBACK_LERP);

            player->GetPlayerModel()->PlayRootMotion(anims.airStart, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
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

// 受付時間内の回避入力を先行入力として記録する
void PlayerDamageState::AcceptDodgeInput(float frame, const AnimationConfig* config)
{
    if (InputAction() != InputActionType::Dodge) return;

    if (frame >= config->advanceInputStartFrame && frame <= config->advanceInputEndFrame)
    {
        nextShiftReady = true;
        nextInput = InputAction();
    }
}

// 先行入力を消費する。回避ステートへ遷移した場合はtrueを返す
bool PlayerDamageState::TryConsumeAdvanceInput(float frame, const AnimationConfig* config)
{
    if (!nextShiftReady || frame < config->advanceInputStartFrame) return false;

    step = Step::Start;
    player->SetGravity(Character::DEFAULT_GRAVITY);
    nextShiftReady = false;

    const bool isDodge = (nextInput == InputActionType::Dodge);
    nextInput = InputActionType::None;
    if (isDodge)
    {
        ChangeState(PlayerStateId::Dodge);
    }
    return isDodge;
}

// 開始処理
void PlayerDamageState::Enter()
{
    currentDamageType = GetCurrentDamageType();
    step = Step::Start;

    // 先行入力をリセット（前回の被弾時のキャンセル入力を持ち越さない）
    nextShiftReady = false;
    nextInput = InputActionType::None;
}

// 更新処理
void PlayerDamageState::Update(float elapsedTime)
{
    float frame = player->GetPlayerModel()->GetCurrentAnimationSeconds();
    int index = player->GetPlayerModel()->GetCurrentAnimationIndex();

    AnimationConfig* config = player->GetPlayerModel()->GetAnimationConfig(ANIMATION_CONFIG_OWNER, index);

    // Normal/Lightダメージは地上で回避キャンセルができる
    const bool isLightReaction = (currentDamageType == DamageType::Normal || currentDamageType == DamageType::Light);

    switch (step)
    {
    case Step::Start: // ダメージ開始
        HandleDamageStart(currentDamageType, elapsedTime);
        step = Step::Reaction;
        break;

    case Step::Reaction: // アニメーション再生中
        // Normal/Lightダメージで地上アニメーションが終了した場合
        if (isLightReaction &&
            player->IsGround() &&
            !player->GetPlayerModel()->IsPlayAnimation())
        {
            player->SetGravity(Character::DEFAULT_GRAVITY);
            step = Step::Start;
            ChangeState(PlayerStateId::Idle);
            return;
        }

        if (isLightReaction && player->IsGround())
        {
            AcceptDodgeInput(frame, config);
        }
        if (TryConsumeAdvanceInput(frame, config)) return;

        // Heavyダメージで地上着地した場合、起き上がりへ
        if (currentDamageType == DamageType::Heavy &&
            player->IsGround() &&
            !player->GetPlayerModel()->IsPlayAnimation())
        {
            player->SetVerticalVelocity(0.0f);
            player->GetPlayerModel()->PlayRootMotion(anims.getUp, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
            step = Step::GetUp;
            break;
        }

        // Knockdownダメージの処理
        if (currentDamageType == DamageType::Knockdown)
        {
            // 水平方向の移動を継続
            MoveTowardKnockbackTarget(CONTINUOUS_KNOCKBACK_LERP);

            // 地面に着地したら
            if (player->IsGround())
            {
                player->SetGravity(Character::DEFAULT_GRAVITY);
                player->SetVerticalVelocity(0.0f);
                player->GetPlayerModel()->PlayRootMotion(anims.airEnd, false, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
                step = Step::Landing;
                break;
            }

            // アニメーション終了でループに切り替え
            if (!player->GetPlayerModel()->IsPlayAnimation())
            {
                player->GetPlayerModel()->PlayRootMotion(anims.airLoop, true, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
            }
        }

        // 空中からのダメージでアニメーションが終了
        if (!player->IsGround() && !player->GetPlayerModel()->IsPlayAnimation() &&
            currentDamageType != DamageType::Knockdown)
        {
            player->GetPlayerModel()->PlayRootMotion(anims.airLoop, true, true, AIR_DAMAGE_BLEND_SECONDS, HIPS_NODE_NAME);
            step = Step::AirLoop;
        }

        // Launchダメージの場合の特別処理
        if (currentDamageType == DamageType::Launch)
        {
            // 水平方向の位置を継続的に補間
            MoveTowardKnockbackTarget(CONTINUOUS_KNOCKBACK_LERP);

            // 最高高度に到達したか確認
            if (player->GetVelocity().y <= 0.0f)
            {
                player->SetVerticalVelocity(0.0f);
                player->SetGravity(0.0f);
                pauseTimer = LAUNCH_APEX_PAUSE_SECONDS;
                step = Step::LaunchApex;
            }

            // アニメーション終了でループに切り替え
            if (!player->GetPlayerModel()->IsPlayAnimation())
            {
                player->GetPlayerModel()->PlayRootMotion(anims.airLoop, true, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
            }
        }
        break;

    case Step::AirLoop: // 空中ループ中(着地待ち)
        if (player->IsGround())
        {
            player->GetPlayerModel()->PlayRootMotion(anims.airEnd, false, true, AIR_DAMAGE_BLEND_SECONDS, HIPS_NODE_NAME);
            step = Step::Landing;
        }
        break;

    case Step::Landing: // 着地アニメーション
        if (!player->GetPlayerModel()->IsPlayAnimation())
        {
            player->GetPlayerModel()->PlayRootMotion(anims.getUp, false, true, AIR_DAMAGE_BLEND_SECONDS, HIPS_NODE_NAME);
            step = Step::GetUp;
        }
        break;

    case Step::GetUp: // 起き上がりアニメーション
        if (!player->GetPlayerModel()->IsPlayAnimation())
        {
            step = Step::Start;
            player->SetGravity(Character::DEFAULT_GRAVITY);

            // 先行入力チェック
            if (InputAction() == InputActionType::LightAttack)
            {
                ChangeState(PlayerStateId::Combo1);
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

        AcceptDodgeInput(frame, config);
        if (TryConsumeAdvanceInput(frame, config)) return;
        break;

    case Step::LaunchApex: // 打ち上げ専用:最高高度で一時停止
        pauseTimer -= elapsedTime;

        if (pauseTimer <= 0.0f)
        {
            player->SetGravity(Character::DEFAULT_GRAVITY);
            player->GetPlayerModel()->PlayRootMotion(anims.airLoop, true, true, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
            step = Step::AirLoop; // 落下処理へ
        }
        break;
    }

    // ダメージを受けた場合は即座に遷移
    if (player->IsAnyDamage())
    {
        player->SetGravity(Character::DEFAULT_GRAVITY);
        player->SetVerticalVelocity(0.0f);
        // 新しい被弾で仕切り直すので、溜まっていた先行入力は破棄する
        nextShiftReady = false;
        nextInput = InputActionType::None;
        currentDamageType = GetCurrentDamageType();
        HandleDamageStart(currentDamageType, elapsedTime);
        step = Step::Reaction;
    }
}

// 終了処理
void PlayerDamageState::Exit()
{
    player->SetGravity(Character::DEFAULT_GRAVITY);
    step = Step::Start;

    // 先行入力をリセット
    nextShiftReady = false;
    nextInput = InputActionType::None;

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
        ImGui::Text(u8"ステップ: %d", static_cast<int>(step));
        ImGui::DragFloat(u8"一時停止タイマー", &pauseTimer, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat3(u8"ノックバック目標位置", &knockbackTargetPosition.x, 0.1f);

        DirectX::XMFLOAT3 damageDir = player->GetDamageDirection();
        ImGui::Text(u8"ダメージ方向: (%.2f, %.2f, %.2f)", damageDir.x, damageDir.y, damageDir.z);

        ImGui::TreePop();
    }
}

//-------------------------------------------------------------
// 死亡ステート
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

    player->GetModel()->PlayRootMotion(deadAnimationIndex, false, false, DEFAULT_BLEND_SECONDS, HIPS_NODE_NAME);
    const DirectX::XMFLOAT3& position = player->GetPosition();
    handle = player->deathEffect->Play({ position.x, position.y + DEATH_EFFECT_OFFSET_Y, position.z }, DEATH_EFFECT_SCALE);

    player->SetDeathFlag(true);
}

// 更新処理
void PlayerDeadState::Update(float elapsedTime)
{
    const DirectX::XMFLOAT3& position = player->GetPosition();
    player->deathEffect->SetPosition(handle, { position.x, position.y + DEATH_EFFECT_OFFSET_Y, position.z });
}
