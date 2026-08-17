#include "Weapon.h"
#include "Graphics/Graphics.h"
#include "Character/Player/Player.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Math/Collision.h"
#include "System/HitStop.h"
#include <vector>
#include "Graphics/Light.h"
#include "Math/Mathf.h"
#include "Effect/EffectManager.h"
#include "System/Audio/Audio.h"


#include <stdlib.h>

namespace
{
    // 攻撃中に刃から発生させるパーティクルの1フレームあたりの数
    constexpr int TRAIL_PARTICLE_EMIT_COUNT = 10;

    // パーティクルが弾け飛ぶ速度の振れ幅
    constexpr float TRAIL_PARTICLE_SPREAD_SPEED = 0.5f;

    // パーティクルの大きさ（最小値＋ランダム幅）
    constexpr float TRAIL_PARTICLE_MIN_SIZE = 0.05f;
    constexpr float TRAIL_PARTICLE_SIZE_RANGE = 0.1f;

    // パーティクルの寿命（最小値＋ランダム幅）
    constexpr float TRAIL_PARTICLE_MIN_LIFETIME = 0.3f;
    constexpr float TRAIL_PARTICLE_LIFETIME_RANGE = 1.0f;

    // 敵に攻撃が当たった時のヒットストップ
    constexpr float HIT_STOP_TIME = 3.0f;
    constexpr float HIT_STOP_SPEED = 0.1f;

    // トレイルのスプライン曲線を何分割して描くか
    constexpr int TRAIL_SPLINE_DIVISION = 10;

    // 0.0～1.0のランダム値を返す
    float Random01()
    {
        return static_cast<float>(rand()) / RAND_MAX;
    }
}

void Weapon::Attach(std::string nodeName, Model* character)
{
    std::string objName = nodeName;

    // 武器のローカル行列を計算する
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
    DirectX::XMMATRIX weaponLocalTransform = S * R * T;

    // キャラクターモデルから右手ノードを検索する
    for (const Model::Node& node : character->GetNodes())
    {
        if (node.name == objName)
        {
            // 右手ノードのワールド行列を取得
            DirectX::XMMATRIX handWorldTransform = DirectX::XMLoadFloat4x4(&node.worldTransform);

            // 武器のワールド行列を計算（右手のワールド行列と武器のローカル行列を掛け合わせる）
            DirectX::XMMATRIX weaponWorldTransform = weaponLocalTransform * handWorldTransform;

            DirectX::XMStoreFloat4x4(&transform, weaponWorldTransform);
            break;
        }
    }
}

void Weapon::TrailUpdate(float elapsedTime)
{
    DirectX::XMMATRIX weaponWorldTransform = DirectX::XMLoadFloat4x4(&transform);

    DirectX::XMFLOAT3 currentRootPos;
    DirectX::XMFLOAT3 currentTipPos;
    {
        // 剣の原点から根本と先端までのオフセット値
        DirectX::XMVECTOR RootOffset = DirectX::XMVectorSet(trailOffset[0].x, trailOffset[0].y, trailOffset[0].z, 0);
        DirectX::XMVECTOR TipOffset = DirectX::XMVectorSet(trailOffset[1].x, trailOffset[1].y, trailOffset[1].z, 0);

        DirectX::XMMATRIX W = weaponWorldTransform;
        DirectX::XMVECTOR Root = DirectX::XMVector3TransformCoord(RootOffset, W);
        DirectX::XMVECTOR Tip = DirectX::XMVector3TransformCoord(TipOffset, W);

        DirectX::XMStoreFloat3(&currentRootPos, Root); // 根本
        DirectX::XMStoreFloat3(&currentTipPos, Tip);   // 先端
    }

    if (isAttack)
    {
        // 攻撃中：履歴を1つずつ後ろにずらして、先頭に最新座標を入れる
        for (int i = MAX_POLYGON - 1; i > 0; i--)
        {
            trailPositions[0][i] = trailPositions[0][i - 1];
            trailPositions[1][i] = trailPositions[1][i - 1];
        }

        // 最新の座標を保存
        trailPositions[0][0] = currentRootPos;
        trailPositions[1][0] = currentTipPos;

        DirectX::XMVECTOR rootVec = DirectX::XMLoadFloat3(&currentRootPos);
        DirectX::XMVECTOR tipVec = DirectX::XMLoadFloat3(&currentTipPos);

        for (int i = 0; i < TRAIL_PARTICLE_EMIT_COUNT; ++i)
        {
            // 根本と先端の間でランダムな座標を計算
            DirectX::XMVECTOR emitPosVec = DirectX::XMVectorLerp(rootVec, tipVec, Random01());
            DirectX::XMFLOAT3 emitPos;
            DirectX::XMStoreFloat3(&emitPos, emitPosVec);

            // 散らばる速度 (ランダムに弾け飛ぶ)
            DirectX::XMFLOAT3 velocity =
            {
                (Random01() - 0.5f) * TRAIL_PARTICLE_SPREAD_SPEED,
                (Random01() - 0.5f) * TRAIL_PARTICLE_SPREAD_SPEED,
                (Random01() - 0.5f) * TRAIL_PARTICLE_SPREAD_SPEED
            };

            // トレイル先端の色を使用
            DirectX::XMFLOAT4 color = { tipBegin.x, tipBegin.y, tipBegin.z, 1.0f };

            float size = TRAIL_PARTICLE_MIN_SIZE + Random01() * TRAIL_PARTICLE_SIZE_RANGE;
            float lifeTime = TRAIL_PARTICLE_MIN_LIFETIME + Random01() * TRAIL_PARTICLE_LIFETIME_RANGE; // 短めでスッと消える

            // パーティクル発生 (behaviorType = 0 を想定)
            EffectManager::Instance().EmitGpuParticle(emitPos, velocity, color, size, lifeTime, 0);
        }
    }
    else
    {
        // 攻撃中でない（待機、移動、怯み、回避など）：
        // 履歴配列の【すべて】を「現在の座標」で上書きする。
        // これにより、トレイルの長さが「0」の状態が維持される。
        // 次に isAttack が true になったとき、古い場所からの引き延ばしが発生しなくなる。
        for (int i = 0; i < MAX_POLYGON; i++)
        {
            trailPositions[0][i] = currentRootPos;
            trailPositions[1][i] = currentTipPos;
        }
    }
}

void Weapon::CollisionNodeVsEnemies(float nodeRadius, int attackDamage, float invincibleTime, float leftVibrate, float rightVibrate, float hitStopTime, float hitStopSpeed)
{
    GamePad& gamepad = Input::Instance().GetGamePad();

    // 当たり判定用オフセットを使い、当たり判定位置を求める
    DirectX::XMMATRIX weaponWorldMatrix = DirectX::XMLoadFloat4x4(&transform);
    for (int i = 0; i < hitSphereIndex; i++)
    {
        DirectX::XMVECTOR weaponHitOffsetVec = DirectX::XMLoadFloat3(&weaponHitOffset[i]);
        DirectX::XMVECTOR P = DirectX::XMVector3Transform(weaponHitOffsetVec, weaponWorldMatrix);
        DirectX::XMStoreFloat3(&weaponHitPosition[i], P);

        // 当たり判定用球描画
        Graphics::Instance().GetShapeRenderer()->DrawSphere(weaponHitPosition[i], hitSphereRadius, { 1.0f, 0.0f, 0.0f, 1.0f });

        // 指定のノードと敵を総当たりで衝突処理
		EnemyBoss& boss = EnemyBoss::Instance();
    	std::vector<NodeHitSphere> enemyNode = boss.GetNodeHitSpheres();
        for (auto& enemyHitSphere : enemyNode)
        {
            Model* enemyModel = boss.GetModel();
            Model::Node* enemyNode = enemyModel->FindNode(enemyHitSphere.nodeName);

            // ノード位置取得
            DirectX::XMFLOAT3 enemyNodePosition;
            enemyNodePosition = { enemyNode->worldTransform._41, enemyNode->worldTransform._42, enemyNode->worldTransform._43 };

            DirectX::XMFLOAT3 outPosition;
            DirectX::XMFLOAT3 outHitPoint;
            if (Collision::IntersectSphereVsSphere(
                weaponHitPosition[i],
                nodeRadius,
                enemyNodePosition,
                enemyHitSphere.radius,
                outPosition,
                outHitPoint))
            {
                {
                    boss.SetDamage(true);
                    HitStop::Instance().HitStopStart(HIT_STOP_TIME, HIT_STOP_SPEED, HIT_STOP_TIME, HIT_STOP_SPEED);
                }
            }
        }
    }
}

void Weapon::CollisionNodeVsCharacter(float nodeRadius, AnimationConfig* config, AnimationAttribute* activeAttribute, Character* character)
{
    if (!isAttack) isAttack = !isAttack;

    GamePad& gamepad = Input::Instance().GetGamePad();
    
    // 当たり判定用オフセットを使い、当たり判定位置を求める
    DirectX::XMMATRIX weaponWorldMatrix = DirectX::XMLoadFloat4x4(&transform);
    for (int i = 0; i < hitSphereIndex; i++)
    {
        DirectX::XMVECTOR weaponHitOffsetVec = DirectX::XMLoadFloat3(&weaponHitOffset[i]);
        DirectX::XMVECTOR P = DirectX::XMVector3Transform(weaponHitOffsetVec, weaponWorldMatrix);
        DirectX::XMStoreFloat3(&weaponHitPosition[i], P);

        Graphics::Instance().GetShapeRenderer()->DrawSphere(weaponHitPosition[i], hitSphereRadius, { 1.0f, 0.0f, 0.0f, 1.0f });

        Player& player = Player::Instance();
        EnemyBoss& boss = EnemyBoss::Instance();
        std::vector<NodeHitSphere> targetHitSphere;
        Model* targetModel;
        if (character == static_cast<Character*>(&player))
        {
            targetHitSphere = boss.GetNodeHitSpheres();
            targetModel = boss.GetModel();
        }
        else
        {
            targetHitSphere = player.GetNodeHitSpheres();
            targetModel = player.GetModel();
        }
           
        for (auto& hitSphere : targetHitSphere)
        {
            Model::Node* targetNode = targetModel->FindNode(hitSphere.nodeName);

            DirectX::XMFLOAT3 targetNodePosition;
            targetNodePosition = { targetNode->worldTransform._41, targetNode->worldTransform._42, targetNode->worldTransform._43 };

            DirectX::XMFLOAT3 outPosition;
            DirectX::XMFLOAT3 outHitPoint;
            if (Collision::IntersectSphereVsSphere(
                weaponHitPosition[i],
                nodeRadius,
                targetNodePosition,
                hitSphere.radius,
                outPosition,
                outHitPoint))
            {
                // アクティブな攻撃属性のパラメータを使用
                if (character == static_cast<Character*>(&player) && activeAttribute && activeAttribute->flag == AnimationFlag::Attack)
                {
                    auto& ap = activeAttribute->attackParam;
					if (boss.GetInvincibleTimer() > 0.0f || boss.IsInvincible())
					{
						return;
					}
                    if (boss.IsSuperArmor())
                    {
                        if(boss.ApplyDamage(ap.attackDamage, ap.invisibleTime, true, outHitPoint))
                            attackHitEffectHandle = attackHitEffect->Play(outHitPoint, 0.2f);
                        return;
                    }
                    if (ap.knockbackType == KnockbackType::None)
                    {
                        lightSE->Play(false, 0.1f);
	                    boss.SetDamage(true);
                    }
                    else if (ap.knockbackType == KnockbackType::Light)
                    {
                        mediumSE->Play(false, 0.1f);
	                    boss.SetLightKbDamage(true);
                    }
                    else if (ap.knockbackType == KnockbackType::Heavy)
                    {
                        heavySE->Play(false, 0.1f);
	                    boss.SetHeavyKbDamage(true);
                    }
                    else if (ap.knockbackType == KnockbackType::Launch)
                    {
                        heavySE->Play(false, 0.1f);
	                    boss.SetLaunchKbDamage(true);
                    }
                    HitStop::Instance().HitStopStart(
                        ap.attackHitStopTime,
                        ap.attackHitStopSpeed,
                        ap.attackHitStopTime,
                        ap.attackHitStopSpeed
                    );
                    if (Input::Instance().GetIsLastGamePad())
                        gamepad.Vibrate(ap.attackLeftVibrate, ap.attackRightVibrate); 
                    Camera::Instance().SetCameraShakeSwitch(true, 0.2f, 1.0f);
                    // ダメージ適用（コメントアウト部分を有効化する場合）
                    if (boss.ApplyDamage(ap.attackDamage, ap.invisibleTime, true, outHitPoint))
                    {
	                    attackHitEffectHandle = attackHitEffect->Play(outHitPoint, 0.2f);
                        boss.AddRevengeValue(ap.revengeValue);
                    }
                }
                else if (character == static_cast<Character*>(&boss) && activeAttribute && activeAttribute->flag == AnimationFlag::Attack)
                {
                    auto& ap = activeAttribute->attackParam;
                    // プレイヤーが回避中なら処理を抜ける
                    bool isPlayerRolling = Player::Instance().GetPlayerIsRolling();
                    if (isPlayerRolling || player.GetInvincibleTimer() > 0.0f)
                    {
                        return;
                    }
                    else if (player.GetPlayerIsGuard())
                    {
                        player.ChangeState(PlayerStateId::GuardHit);
                    }
                    else
                    {
                        player.SetDamageDirection(boss.GetPosition());

                        // ダメージタイプに応じてフラグを設定
                        if (ap.knockbackType == KnockbackType::None)
                        {
                            lightSE->Play(false, 0.1f);
	                        player.SetDamage(true);
                        }
                        else if (ap.knockbackType == KnockbackType::Light)
                        {
                            mediumSE->Play(false, 0.1f);
	                        player.SetLightDamage(true);
                        }
                        else if (ap.knockbackType == KnockbackType::Heavy)
                        {
                            heavySE->Play(false, 0.1f);
	                        player.SetHeavyDamage(true);
                        }
                        else if (ap.knockbackType == KnockbackType::Launch)
                        {
                            heavySE->Play(false, 0.1f);
	                        player.SetLaunchDamage(true);
                        }
                        else if (ap.knockbackType == KnockbackType::KnockDown)
                        {
                            heavySE->Play(false, 0.1f);
	                        player.SetKnockDownDamage(true);
                        }
                        if (player.ApplyDamage(ap.attackDamage, ap.invisibleTime, true, outHitPoint))
                        {
                            Camera::Instance().SetCameraShakeSwitch(true, 0.2f, 1.0f);
                            attackHitEffectHandle = attackHitEffect->Play(outHitPoint, 0.2f);
                        }
                    }
                }
            }
        }
    }
}

void Weapon::AttackAnimationCollision(Model* character, float animTimeMin, float animTimeMax, int attackDamage, float invincibleTime, float leftVibrate, float rightVibrate, float hitStopTime, float hitStopSpeed)
{
    GamePad& gamepad = Input::Instance().GetGamePad();

    //任意のアニメーションの再生区間のみ衝突処理をする
    float animationTime = character->GetCurrentAnimationSeconds();
    {
        //攻撃当たり判定旧とエネミーの衝突判定
        if (animationTime >= animTimeMin && animationTime <= animTimeMax)
        {
            CollisionNodeVsEnemies(hitSphereRadius, attackDamage, invincibleTime, leftVibrate, rightVibrate, hitStopTime, hitStopSpeed);
        }
        else
        {
            if (isAttack) isAttack = !isAttack;

            gamepad.Vibrate(0.0f, 0.0f);
        }
    }
}

void Weapon::AttackAnimationCollision(Model* model, AnimationConfig* config, Character* character)
{
    GamePad& gamepad = Input::Instance().GetGamePad();
    float animationTime = model->GetCurrentAnimationSeconds();

    bool anyAttackActive = false;
    AnimationAttribute* activeAttackAttribute = nullptr;

    // 複数のAttributeをチェック
    for (auto& attribute : config->attributes)
    {
        if (attribute.flag == AnimationFlag::Attack && attribute.IsActive(animationTime))
        {
            anyAttackActive = true;
            activeAttackAttribute = &attribute;

            // 攻撃当たり判定球とエネミーの衝突判定（アクティブな属性を渡す）
            CollisionNodeVsCharacter(hitSphereRadius, config, activeAttackAttribute, character);

            break; // 最初に見つかったアクティブな攻撃属性のみを処理
        }
    }

    if (!anyAttackActive)
    {
        if (isAttack) isAttack = !isAttack;
        gamepad.Vibrate(0.0f, 0.0f);
    }
}

void Weapon::SetTeleportEffect(bool enable, float progress, float time)
{
    hasTeleportEffect = enable;
    teleportProgress = progress;
    teleportTime = time;
}

void Weapon::ClearTeleportEffect()
{
    hasTeleportEffect = false;
    teleportProgress = 0.0f;
    teleportTime = 0.0f;
}

void Weapon::TrailRender(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
    TrailRenderer* trailRenderer = Graphics::Instance().GetTrailRenderer();

    DirectX::XMFLOAT4 tipcolor = { 1, 0, 0, 1 };
    DirectX::XMFLOAT4 rootcolor = { 1, 0, 0, 1 };

	float a = 1.0f / (MAX_POLYGON - 1);

    // スプライン補完処理による滑らかなポリゴンを描画
    for (int i = 0; i < MAX_POLYGON - 3; ++i)
    {
        int index0 = i;
        int index1 = (index0 + 1) % MAX_POLYGON;
        int index2 = (index1 + 1) % MAX_POLYGON;
        int index3 = (index2 + 1) % MAX_POLYGON;
        DirectX::XMVECTOR Tip0 = DirectX::XMLoadFloat3(&trailPositions[1][index0]);
        DirectX::XMVECTOR Tip1 = DirectX::XMLoadFloat3(&trailPositions[1][index1]);
        DirectX::XMVECTOR Tip2 = DirectX::XMLoadFloat3(&trailPositions[1][index2]);
        DirectX::XMVECTOR Tip3 = DirectX::XMLoadFloat3(&trailPositions[1][index3]);
        DirectX::XMVECTOR Root0 = DirectX::XMLoadFloat3(&trailPositions[0][index0]);
        DirectX::XMVECTOR Root1 = DirectX::XMLoadFloat3(&trailPositions[0][index1]);
        DirectX::XMVECTOR Root2 = DirectX::XMLoadFloat3(&trailPositions[0][index2]);
        DirectX::XMVECTOR Root3 = DirectX::XMLoadFloat3(&trailPositions[0][index3]);

        DirectX::XMVECTOR Tip = DirectX::XMVectorCatmullRom(Tip0, Tip1, Tip2, Tip3, 0.5f);
        DirectX::XMVECTOR Root = DirectX::XMVectorCatmullRom(Root0, Root1, Root2, Root3, 0.5f);

        DirectX::XMVECTOR Alpha0 = DirectX::XMVectorSet(a * (i + 0), 0, 0, 0);
        DirectX::XMVECTOR Alpha1 = DirectX::XMVectorSet(a * (i + 1), 0, 0, 0);
        DirectX::XMVECTOR Alpha2 = DirectX::XMVectorSet(a * (i + 2), 0, 0, 0);
        DirectX::XMVECTOR Alpha3 = DirectX::XMVectorSet(a * (i + 3), 0, 0, 0);

        float dissolve = static_cast<float>(i) / static_cast<float>(MAX_POLYGON - 3);
        dissolve = 1.0f - (this->dissolve * 2) * (dissolve);

        for (int j = 0; j <= TRAIL_SPLINE_DIVISION; ++j)
        {
            float t = j / static_cast<float>(TRAIL_SPLINE_DIVISION);

            DirectX::XMVECTOR Tip = DirectX::XMVectorCatmullRom(Tip0, Tip1, Tip2, Tip3, t);
            DirectX::XMVECTOR Root = DirectX::XMVectorCatmullRom(Root0, Root1, Root2, Root3, t);
            DirectX::XMVECTOR Alpha = DirectX::XMVectorCatmullRom(Alpha0, Alpha1, Alpha2, Alpha3, t);
            DirectX::XMFLOAT3 root, tip;
            DirectX::XMStoreFloat3(&root, Root);
            DirectX::XMStoreFloat3(&tip, Tip);

            float alpha = DirectX::XMVectorGetX(Alpha);
            tipcolor.w *= alpha;
            rootcolor.w *= alpha;

            DirectX::XMVECTOR tipBegin = DirectX::XMLoadFloat4(&this->tipBegin);
            DirectX::XMVECTOR tipEnd = DirectX::XMLoadFloat4(&this->tipEnd);
            DirectX::XMVECTOR TipColor = DirectX::XMVectorLerp(tipBegin, tipEnd, alpha);
            DirectX::XMStoreFloat4(&tipcolor, TipColor);

            DirectX::XMVECTOR rootBegin = DirectX::XMLoadFloat4(&this->rootBegin);
            DirectX::XMVECTOR rootEnd = DirectX::XMLoadFloat4(&this->rootEnd);
            DirectX::XMVECTOR RootColor = DirectX::XMVectorLerp(rootBegin, rootEnd, alpha);
            DirectX::XMStoreFloat4(&rootcolor, RootColor);

            float scale = colorScale;

            trailRenderer->AddVertex(root, rootcolor, { alpha, 0 }, dissolve);
            trailRenderer->AddVertex(tip, { tipcolor.x * scale,tipcolor.y * scale,tipcolor.z * scale,tipcolor.w * scale }, { alpha, 1 }, dissolve);
        }
    }
}

void Weapon::ResetAttackState()
{
    // 攻撃中だった場合のみ処理
    if (isAttack)
    {
        isAttack = false;

        // コントローラーの振動が残らないように停止
        Input::Instance().GetGamePad().Vibrate(0.0f, 0.0f);

        // トレイルを即座にリセットするために更新をかける
        // (isAttack = false にした状態で呼ぶことで、前回の修正コードの else ループが走り、履歴がクリアされる)
        TrailUpdate(0.0f);
    }
}

void Weapon::DrawDebugTrailGUI()
{
    if (ImGui::CollapsingHeader("Trail", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::TreeNode("Trail "))
        {
            ImGui::Checkbox("AttackFlag", &isAttack);
            ImGui::DragFloat3("TrailTip", &trailOffset[1].x, 0.1f);
            ImGui::DragFloat3("TrailRoot", &trailOffset[0].x, 0.1f);

            ImGui::DragFloat("TrailDissolve", &dissolve, 0.01f);

            ImGui::ColorEdit4("TipBeginTrail", &tipBegin.x);
            ImGui::ColorEdit4("TipEndTrail", &tipEnd.x);

            ImGui::ColorEdit4("RootBeginTrail", &rootBegin.x);
            ImGui::ColorEdit4("RootEndTrail", &rootEnd.x);

            ImGui::DragFloat("ColorScale", &colorScale, 0.1f);

            ImGui::TreePop();
        }
    }
    ImGui::ColorEdit4("TrailPointColor", &pointColor.x);
    ImGui::DragFloat("TrailPAttenuation", &attenuation, 0.1f);
}



// 刃に沿った当たり判定球の初期配置（派生クラス共通）
void Weapon::SetupBladeHitSpheres(float sphereRadius)
{
    // 柄元から切っ先に向かって等間隔に球を並べる
    constexpr float bladeOffsetZ[HIT_SPHERE_COUNT] = { 0.0f, 0.6f, 0.85f, 1.1f, 1.35f };

    hitSphereIndex = HIT_SPHERE_COUNT;
    for (int i = 0; i < HIT_SPHERE_COUNT; ++i)
    {
        weaponHitOffset[i] = { 0.0f, 0.0f, bladeOffsetZ[i] };
    }
    hitSphereRadius = sphereRadius;
}

// 武器共通のデバッグGUI（位置・回転・スケール・当たり判定球）
void Weapon::DrawCommonDebugGUI()
{
    ImGui::DragFloat3("WeaponPosition", &position.x, 0.01f);
    ImGui::DragFloat3("WeaponAngle", &angle.x, 0.01f);
    ImGui::DragFloat3("WeaponScale", &scale.x, 0.01f);

    for (int i = 0; i < hitSphereIndex; i++)
    {
        ImGui::PushID(i);
        ImGui::DragFloat3("WeaponHitSpherePos", &weaponHitOffset[i].x, 0.1f);
        ImGui::PopID();
    }
    ImGui::DragFloat("WeaponHitSphereRadius", &hitSphereRadius, 0.01f, 0.0f, 10.0f);
}

// ヒットエフェクトと打撃音の読み込み（派生クラス共通）
void Weapon::LoadCommonResources()
{
    attackHitEffect = std::make_shared<Effect>("Data/Effect/HitEffect.efkefc");
    lightSE = Audio::Instance().LoadAudioSource("Data/Sound/SE/light_punch1.wav");
    mediumSE = Audio::Instance().LoadAudioSource("Data/Sound/SE/medium_punch1.wav");
    heavySE = Audio::Instance().LoadAudioSource("Data/Sound/SE/heavy_punch1.wav");
}

Weapon::~Weapon()
{
    delete lightSE;
    delete mediumSE;
    delete heavySE;
}
