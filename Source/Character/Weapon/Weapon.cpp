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


#include <stdlib.h>



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
        DirectX::XMVECTOR RootOffset = DirectX::XMVectorSet(trailoffset[0].x, trailoffset[0].y, trailoffset[0].z, 0);
        DirectX::XMVECTOR TipOffset = DirectX::XMVectorSet(trailoffset[1].x, trailoffset[1].y, trailoffset[1].z, 0);

        DirectX::XMMATRIX W = weaponWorldTransform;
        DirectX::XMVECTOR Root = DirectX::XMVector3TransformCoord(RootOffset, W);
        DirectX::XMVECTOR Tip = DirectX::XMVector3TransformCoord(TipOffset, W);

        DirectX::XMStoreFloat3(&currentRootPos, Root); // 根本
        DirectX::XMStoreFloat3(&currentTipPos, Tip);   // 先端
    }

    if (IsAttack)
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

        // 1フレームあたりの発生数
        int emitCount = 10;

        DirectX::XMVECTOR rootVec = DirectX::XMLoadFloat3(&currentRootPos);
        DirectX::XMVECTOR tipVec = DirectX::XMLoadFloat3(&currentTipPos);

        for (int i = 0; i < emitCount; ++i)
        {
            // 0.0 ～ 1.0 のランダムな割合を作成
            float t = (float)rand() / RAND_MAX;

            // 根本と先端の間でランダムな座標を計算
            DirectX::XMVECTOR emitPosVec = DirectX::XMVectorLerp(rootVec, tipVec, t);
            DirectX::XMFLOAT3 emitPos;
            DirectX::XMStoreFloat3(&emitPos, emitPosVec);

            // 散らばる速度 (ランダムに弾け飛ぶ)
            float vx = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
            float vy = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
            float vz = ((float)rand() / RAND_MAX - 0.5f) * 0.5f;
            DirectX::XMFLOAT3 velocity = { vx, vy, vz };

            // 色 (トレイルの色 TipBegin に合わせてみるのも綺麗です)
            DirectX::XMFLOAT4 color = { TipBegin.x, TipBegin.y, TipBegin.z, 1.0f }; // トレイル先端の色を使用

            float size = 0.05f + ((float)rand() / RAND_MAX) * 0.1f;
            float lifeTime = 0.3f + ((float)rand() / RAND_MAX) ; // 短めでスッと消える

            // パーティクル発生 (behaviorType = 0 を想定)
            EffectManager::Instance().EmitGpuParticle(emitPos, velocity, color, size, lifeTime, 0);
        }
    }
    else
    {
        // 攻撃中でない（待機、移動、怯み、回避など）：
        // 履歴配列の【すべて】を「現在の座標」で上書きする。
        // これにより、トレイルの長さが「0」の状態が維持される。
        // 次に IsAttack が true になったとき、古い場所からの引き延ばしが発生しなくなる。
        for (int i = 0; i < MAX_POLYGON; i++)
        {
            trailPositions[0][i] = currentRootPos;
            trailPositions[1][i] = currentTipPos;
        }
    }
}

void Weapon::CollisionNodeVsEnemies(float nodeRadius, int AttackDamage, float invicibleTime, float leftVibrate, float rightVibrate, float hitStopTime, float hitStopSpeed)
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
                    HitStop::Instance().HitStopStart(3.0f, 0.1f, 3.0f, 0.1f);
                }
            }
        }
    }
}

void Weapon::CollisionNodeVsCharacter(float nodeRadius, AnimationConfig* config, AnimationAttribute* activeAttribute, Character* character)
{
    if (!IsAttack) IsAttack = !IsAttack;

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

void Weapon::AttackAnimationCollision(Model* character, float animTimeMin, float animTimeMax, int AttackDamage, float invicibleTime, float leftVibrate, float rightVibrate, float hitStopTime, float hitStopSpeed)
{
    GamePad& gamepad = Input::Instance().GetGamePad();

    //任意のアニメーションの再生区間のみ衝突処理をする
    float animationTime = character->GetCurrentAnimationSeconds();
    {
        //攻撃当たり判定旧とエネミーの衝突判定
        if (animationTime >= animTimeMin && animationTime <= animTimeMax)
        {
            CollisionNodeVsEnemies(hitSphereRadius, AttackDamage, invicibleTime, leftVibrate, rightVibrate, hitStopTime, hitStopSpeed);
        }
        else
        {
            if (IsAttack) IsAttack = !IsAttack;

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
        if (IsAttack) IsAttack = !IsAttack;
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

        for (int j = 0; j <= 10; ++j)
        {
            float t = j / static_cast<float>(10);

            DirectX::XMVECTOR Tip = DirectX::XMVectorCatmullRom(Tip0, Tip1, Tip2, Tip3, t);
            DirectX::XMVECTOR Root = DirectX::XMVectorCatmullRom(Root0, Root1, Root2, Root3, t);
            DirectX::XMVECTOR Alpha = DirectX::XMVectorCatmullRom(Alpha0, Alpha1, Alpha2, Alpha3, t);
            DirectX::XMFLOAT3 root, middle, tip;
            DirectX::XMStoreFloat3(&root, Root);
            DirectX::XMStoreFloat3(&tip, Tip);

            float alpha = DirectX::XMVectorGetX(Alpha);
            tipcolor.w *= alpha;
            rootcolor.w *= alpha;

            DirectX::XMVECTOR TipBegin = DirectX::XMLoadFloat4(&this->TipBegin);
            DirectX::XMVECTOR TipEnd = DirectX::XMLoadFloat4(&this->TipEnd);
            DirectX::XMVECTOR TipColor = DirectX::XMVectorLerp(TipBegin, TipEnd, alpha);
            DirectX::XMStoreFloat4(&tipcolor, TipColor);

            DirectX::XMVECTOR RootBegin = DirectX::XMLoadFloat4(&this->RootBegin);
            DirectX::XMVECTOR RootEnd = DirectX::XMLoadFloat4(&this->RootEnd);
            DirectX::XMVECTOR RootColor = DirectX::XMVectorLerp(RootBegin, RootEnd, alpha);
            DirectX::XMStoreFloat4(&rootcolor, RootColor);

            float scale = Colorscale;

            trailRenderer->AddVertex(root, rootcolor, { alpha, 0 }, dissolve);
            trailRenderer->AddVertex(tip, { tipcolor.x * scale,tipcolor.y * scale,tipcolor.z * scale,tipcolor.w * scale }, { alpha, 1 }, dissolve);
        }
    }
}

void Weapon::ResetAttackState()
{
    // 攻撃中だった場合のみ処理
    if (IsAttack)
    {
        IsAttack = false;

        // コントローラーの振動が残らないように停止
        Input::Instance().GetGamePad().Vibrate(0.0f, 0.0f);

        // トレイルを即座にリセットするために更新をかける
        // (IsAttack = false にした状態で呼ぶことで、前回の修正コードの else ループが走り、履歴がクリアされる)
        TrailUpdate(0.0f);
    }
}

void Weapon::DrawDebugTrailGui()
{
    if (ImGui::CollapsingHeader("Trail", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::TreeNode("Trail "))
        {
            ImGui::Checkbox("AttackFlag", &IsAttack);
            ImGui::DragFloat3("TrailTip", &trailoffset[1].x, 0.1f);
            ImGui::DragFloat3("TrailRoot", &trailoffset[0].x, 0.1f);

            ImGui::DragFloat("TrailDissolve", &dissolve, 0.01f);

            ImGui::ColorEdit4("TipBeginTrail", &TipBegin.x);
            ImGui::ColorEdit4("TipEndTrail", &TipEnd.x);

            ImGui::ColorEdit4("RootBeginTrail", &RootBegin.x);
            ImGui::ColorEdit4("RootEndTrail", &RootEnd.x);

            ImGui::DragFloat("ColorScale", &Colorscale, 0.1f);

            ImGui::TreePop();
        }
    }
    ImGui::ColorEdit4("TrailPointColor", &pointColor.x);
    ImGui::DragFloat("TrailPAttenuation", &attenuation, 0.1f);
}


