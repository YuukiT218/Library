#include "Weapon.h"
#include "Graphics/Graphics.h"
#include "Character/Enemy/SilverDragonkin.h"
#include "Math/Collision.h"
#include "System/HitStop.h"
#include <vector>
#include "Graphics/Light.h"

void Weapon::Attach(std::string nodeName, Model* character)
{
    std::string objName = nodeName;

    // 武器のローカル行列を計算する
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
    DirectX::XMMATRIX weaponLocalTransform = S * R * T;

    //for (const Model::Node& node : model->GetNodes())
    //{
    //    if (node.name == "J_Adj_L_FaceEye")
    //    {
    //        // 保存していた頂点座標を１フレーム分ずらす
    //        {
    //            for (int i = weaponRight.MAX_POLYGON - 1; i > 0; i--)
    //            {
    //                weaponRight.trailPositions[0][i] = weaponRight.trailPositions[0][i - 1];
    //                weaponRight.trailPositions[1][i] = weaponRight.trailPositions[1][i - 1];
    //            }
    //        }

    //        // 剣の根本と先端の座標を取得し、最新の頂点座標を保存
    //        {
    //            //剣の原点から根本と先端までのオフセット値
    //            DirectX::XMVECTOR RootOffset = DirectX::XMVectorSet(weaponRight.trailoffset[0].x, weaponRight.trailoffset[0].y, weaponRight.trailoffset[0].z, 0);
    //            DirectX::XMVECTOR TipOffset = DirectX::XMVectorSet(weaponRight.trailoffset[1].x, weaponRight.trailoffset[1].y, weaponRight.trailoffset[1].z, 0);

    //            DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&node.worldTransform);
    //            DirectX::XMVECTOR Root = DirectX::XMVector3TransformCoord(RootOffset, W);
    //            DirectX::XMVECTOR Tip = DirectX::XMVector3TransformCoord(TipOffset, W);

    //            DirectX::XMStoreFloat3(&weaponRight.trailPositions[0][0], Root);
    //            DirectX::XMStoreFloat3(&weaponRight.trailPositions[1][0], Tip);
    //        }
    //    }
    //}

    // キャラクターモデルから右手ノードを検索する
    for (const Model::Node& node : character->GetNodes())
    {
        if (node.name == objName)
        {
            // 右手ノードのワールド行列を取得
            //DirectX::XMMATRIX handGlobalTransform = DirectX::XMLoadFloat4x4(&node.globalTransform);
            DirectX::XMMATRIX handWorldTransform = DirectX::XMLoadFloat4x4(&node.worldTransform);
            //DirectX::XMMATRIX playerWorldTransform = DirectX::XMLoadFloat4x4(&transform);

            // 武器のワールド行列を計算（右手のワールド行列と武器のローカル行列を掛け合わせる）
            // ゲープロⅣのやり方では右手座標系で計算しているが描画エンジンは左手で計算しているのでGlobalを座標変換すると同じコードで書くことができる
            //DirectX::XMMATRIX weaponWorldTransform = weaponLocalTransform * handGlobalTransform * playerWorldTransform;
            DirectX::XMMATRIX weaponWorldTransform = weaponLocalTransform * handWorldTransform;

            DirectX::XMStoreFloat4x4(&transform, weaponWorldTransform);

            //Graphics::Instance().GetDebugRenderer()->DrawSphere(
            //    { object->transform._41, object->transform._42,object->transform._43 },
            //    1.0f, { 1, 0, 0, 1 });

            break;
        }
    }
}

void Weapon::TrailUpdate(float elapsedTime)
{

    DirectX::XMMATRIX weaponWorldTransform = DirectX::XMLoadFloat4x4(&transform);


    // 保存していた頂点座標を１フレーム分ずらす
    {
        for (int i = MAX_POLYGON - 1; i > 0; i--)
        {
            trailPositions[0][i] = trailPositions[0][i - 1];
            trailPositions[1][i] = trailPositions[1][i - 1];
            //trailPositions[2][i] = trailPositions[2][i - 1];
        }
    }

    // 剣の根本と先端の座標を取得し、最新の頂点座標を保存
    {
        //剣の原点から根本と先端までのオフセット値
        DirectX::XMVECTOR RootOffset = DirectX::XMVectorSet(trailoffset[0].x, trailoffset[0].y, trailoffset[0].z, 0);
        //DirectX::XMVECTOR MiddleOffset = DirectX::XMVectorSet(trailoffset[1].x, trailoffset[1].y, trailoffset[1].z, 0);
        //DirectX::XMVECTOR TipOffset = DirectX::XMVectorSet(trailoffset[2].x, trailoffset[2].y, trailoffset[2].z, 0);
        DirectX::XMVECTOR TipOffset = DirectX::XMVectorSet(trailoffset[1].x, trailoffset[1].y, trailoffset[1].z, 0);

        DirectX::XMMATRIX W = weaponWorldTransform;
        DirectX::XMVECTOR Root = DirectX::XMVector3TransformCoord(RootOffset, W);
        //DirectX::XMVECTOR Middle = DirectX::XMVector3TransformCoord(MiddleOffset, W);
        DirectX::XMVECTOR Tip = DirectX::XMVector3TransformCoord(TipOffset, W);

        DirectX::XMStoreFloat3(&trailPositions[0][0], Root);//根本
        // DirectX::XMStoreFloat3(&trailPositions[1][0], Middle);//中間
         //DirectX::XMStoreFloat3(&trailPositions[2][0], Tip);//先端
        DirectX::XMStoreFloat3(&trailPositions[1][0], Tip);//先端
    }

}

void Weapon::CollisionNodeVsEnemies(float nodeRadius, int AttackDamage, float invicibleTime, float leftVibrate, float rightVibrate, float hitStopTime, float hitStopSpeed)
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

        // 当たり判定用球描画
        Graphics::Instance().GetShapeRenderer()->DrawSphere(weaponHitPosition[i], hitSphereRadius, { 1.0f, 0.0f, 0.0f, 1.0f });

        // 指定のノードと全ての敵を総当たりで衝突処理
        //EnemyManager& enemyManager = EnemyManager::Instance();
		SilverDragonkin& dragonkin = SilverDragonkin::Instance();

        // 全てのプレイヤー攻撃判定と全ての敵の総当たりで衝突処理
        //int enemyCount = enemyManager.GetEnemyCount();
        //for (int j = 0; j < enemyCount; ++j)
        //{
        //    Enemy* enemy = enemyManager.GetEnemy(j);

            std::vector<NodeHitSphere> enemyNode = dragonkin.GetNodeHitSpheres();
            for (auto& enemyHitSphere : enemyNode)
            {
                Model* enemyModel = dragonkin.GetModel();
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
                    // ダメージを与える
                    //if (enemy->ApplyDamage(rand() % 200 + config->attackParam.attackDamage, config->attackParam.invisibleTime, true, outHitPoint))
                    {
                        dragonkin.SetDamage(true);
                        //HitStop::Instance().HitStopStart(config->attackParam.attackHitStopTime, config->attackParam.attackHitStopSpeed, config->attackParam.attackHitStopTime, config->attackParam.attackHitStopSpeed);
                        HitStop::Instance().HitStopStart(3.0f, 0.1f, 3.0f, 0.1f);

                        //gamepad.Vibrate(config->attackParam.attackLeftVibrate, config->attackParam.attackRightVibrate);

                        //attackHitEffectHandle = attackHitEffect->Play(outHitPoint, 0.5f);

                        //Camera::Instance().SetCameraShakeSwitch(true, 0.2f, 0.5f);

                        //// 敵を吹っ飛ばすベクトルを算出
                        //DirectX::XMFLOAT3 vec;
                        //vec.x = outPosition.x - object->weaponHitPosition[i].x;
                        //vec.z = outPosition.z - object->weaponHitPosition[i].z;
                        //float length = sqrtf(vec.x * vec.x + vec.z * vec.z);
                        //vec.x /= length;
                        //vec.z /= length;

                        //// XZ平面に吹っ飛ばす力をかける
                        //float power = 15.0f;
                        //vec.x *= power;
                        //vec.z *= power;
                        //// Y方向にも力をかける
                        //vec.y = 5.0f;

                        //// 吹っ飛ばす
                        //enemy->AddImpulse(vec);
                    }
                }
            }
        //}
    }
}

void Weapon::AttackAnimationCollision(Model* character, float animTimeMin, float animTimeMax, int AttackDamage, float invicibleTime, float leftVibrate, float rightVibrate, float hitStopTime, float hitStopSpeed)
{
    GamePad& gamepad = Input::Instance().GetGamePad();

    //任意のアニメーションの再生区間のみ衝突処理をする
    float animationTime = character->GetCurrentAnimationSeconds();
    //if (config->attribute.flag == AnimationFlag::Attack)
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

// パリィ判定と敵の攻撃の衝突処理
void Weapon::CollisionParryVsAttack()
{
    // 当たり判定用オフセットを使い、当たり判定位置を求める
    DirectX::XMMATRIX leftShieldWorldMatrix = DirectX::XMLoadFloat4x4(&transform);
    DirectX::XMVECTOR leftShieldHitOffsetVec = DirectX::XMLoadFloat3(&weaponHitOffset[0]);
    DirectX::XMVECTOR P = DirectX::XMVector3Transform(leftShieldHitOffsetVec, leftShieldWorldMatrix);
    DirectX::XMStoreFloat3(&weaponHitPosition[0], P);

    // 当たり判定用球描画
    Graphics::Instance().GetShapeRenderer()->DrawSphere(weaponHitPosition[0], hitSphereRadius, { 1.0f, 0.0f, 0.0f, 1.0f });

    // 指定のノードと全ての敵を総当たりで衝突処理
    //EnemyManager& enemyManager = EnemyManager::Instance();

    // 全てのプレイヤー攻撃判定と全ての敵の総当たりで衝突処理
    //int enemyConst = enemyManager.GetEnemyCount();
    //for (int j = 0; j < enemyConst; ++j)
    //{
    //    Enemy* enemy = enemyManager.GetEnemy(j);

    //    if (enemy->GetAttackFlg())
    //    {
    //        std::vector<NodeHitSphere> enemyAttackSphere = enemy->GetAttackHitSpheres();
    //        for (auto& enemyAttackHitSphere : enemyAttackSphere)
    //        {
    //            Model* enemyModel = enemy->GetModel();
    //            Model::Node* enemyNode = enemyModel->FindNode(enemyAttackHitSphere.nodeName);

    //            // ノード位置取得
    //            DirectX::XMFLOAT3 enemyNodePosition;
    //            enemyNodePosition = { enemyNode->worldTransform._41, enemyNode->worldTransform._42, enemyNode->worldTransform._43 };

    //            DirectX::XMFLOAT3 outPosition;
    //            DirectX::XMFLOAT3 outHitPoint;
    //            if (Collision::IntersectSphereVsSphere(
    //                weaponHitPosition[0],
    //                hitSphereRadius,
    //                enemyNodePosition,
    //                enemyAttackHitSphere.radius,
    //                outPosition,
    //                outHitPoint))
    //            {
    //                if (IsParry) {}
    //                else if (!enemy->isPlayerInvincible)
    //                {
    //                    if (collTime <= 0.0f)
    //                    {
    //                        ParrySE->Play(false, 1.2f);
    //                        ParryEffectHandle = parryEffect->Play(outPosition, 1.0f);
    //                        IsParry = true;
    //                        MaxParryTime = MAXPARRYTIME;
    //                        collTime = MaxCollTime;
    //                        GamePad& gamepad = Input::Instance().GetGamePad();
    //                        gamepad.Vibrate(0.1f, 0.1f);
    //                        Camera::Instance().SetCameraShakeSwitch(true, 1.0f, 6.0f);
    //                        Camera::Instance().SetCameraFlash(true);
    //                        HitStop::Instance().HitStopStart(1.0f, 0.1f, 3.0f, 0.1f);
    //                        enemy->isPlayerInvincible = true;
    //                    }
    //                }
    //            }
    //        }
    //    }
    //}
}

// パリィの判定を付ける
void Weapon::ParryAnimationCollision(Model* character, float animTimeMin, float animTimeMax)
{
    //// 任意のアニメーションの再生空間のみ衝突処理をする
    float animationTime = character->GetCurrentAnimationSeconds();
    if (animationTime >= animTimeMin && animationTime <= animTimeMax)
    {
    /*if (config->attribute.flag == AnimationFlag::Parry)
    {*/
        //if (config->attribute.IsActive(animationTime))
            CollisionParryVsAttack();
    }
    //}
}

void Weapon::TrailRender(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
    RenderState* renderState = Graphics::Instance().GetRenderState();
    //TrailRenderer* trailRenderer = Graphics::Instance().GetTrailRenderer();

    DirectX::XMFLOAT4 tipcolor = { 1, 0, 0, 1 };
    DirectX::XMFLOAT4 rootcolor = { 1, 0, 0, 1 };

    //// 保存していた頂点バッファでポリゴンを作る
    //for (int i = 0; i < MAX_POLYGON; ++i)
    //{
    //    trailRenderer->AddVertex(trailPositions[0][i], color);
    //    trailRenderer->AddVertex(trailPositions[1][i], color);
    //}
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
        //DirectX::XMVECTOR Tip0 = DirectX::XMLoadFloat3(&trailPositions[2][index0]);
        //DirectX::XMVECTOR Tip1 = DirectX::XMLoadFloat3(&trailPositions[2][index1]);
        //DirectX::XMVECTOR Tip2 = DirectX::XMLoadFloat3(&trailPositions[2][index2]);
        //DirectX::XMVECTOR Tip3 = DirectX::XMLoadFloat3(&trailPositions[2][index3]);    
        //DirectX::XMVECTOR Middle0 = DirectX::XMLoadFloat3(&trailPositions[1][index0]);
        //DirectX::XMVECTOR Middle1 = DirectX::XMLoadFloat3(&trailPositions[1][index1]);
        //DirectX::XMVECTOR Middle2 = DirectX::XMLoadFloat3(&trailPositions[1][index2]);
        //DirectX::XMVECTOR Middle3 = DirectX::XMLoadFloat3(&trailPositions[1][index3]);
        DirectX::XMVECTOR Root0 = DirectX::XMLoadFloat3(&trailPositions[0][index0]);
        DirectX::XMVECTOR Root1 = DirectX::XMLoadFloat3(&trailPositions[0][index1]);
        DirectX::XMVECTOR Root2 = DirectX::XMLoadFloat3(&trailPositions[0][index2]);
        DirectX::XMVECTOR Root3 = DirectX::XMLoadFloat3(&trailPositions[0][index3]);

        DirectX::XMVECTOR Tip = DirectX::XMVectorCatmullRom(Tip0, Tip1, Tip2, Tip3, 0.5f);
        //DirectX::XMVECTOR Middle = DirectX::XMVectorCatmullRom(Middle0, Middle1, Middle2, Middle3, 0.5f);
        DirectX::XMVECTOR Root = DirectX::XMVectorCatmullRom(Root0, Root1, Root2, Root3, 0.5f);

        DirectX::XMVECTOR Alpha0 = DirectX::XMVectorSet(a * (i + 0), 0, 0, 0);
        DirectX::XMVECTOR Alpha1 = DirectX::XMVectorSet(a * (i + 1), 0, 0, 0);
        DirectX::XMVECTOR Alpha2 = DirectX::XMVectorSet(a * (i + 2), 0, 0, 0);
        DirectX::XMVECTOR Alpha3 = DirectX::XMVectorSet(a * (i + 3), 0, 0, 0);

        float dissolve = static_cast<float>(i) / static_cast<float>(MAX_POLYGON - 3);
        //float dissolve = 1.0f - pow(this->dissolve * 1.5f * this->dissolve, 2.0f); // 残像感


        dissolve = 1.0f - (this->dissolve * 2) * (dissolve);


        /*DirectX::XMFLOAT3 root;
        DirectX::XMVECTOR Alpha = DirectX::XMVectorCatmullRom(Alpha0, Alpha1, Alpha2, Alpha3, t);
        DirectX::XMStoreFloat3(&root, Root);
        DirectX::XMFLOAT3 tip;
        DirectX::XMStoreFloat3(&tip, Tip);

        primitiveRenderer->AddVertex(root, color);
        primitiveRenderer->AddVertex(tip, color);*/
        for (int j = 0; j <= 10; ++j)
        {
            float t = j / static_cast<float>(10);

            DirectX::XMVECTOR Tip = DirectX::XMVectorCatmullRom(Tip0, Tip1, Tip2, Tip3, t);
            //DirectX::XMVECTOR Middle = DirectX::XMVectorCatmullRom(Middle0, Middle1, Middle2, Middle3, t);
            DirectX::XMVECTOR Root = DirectX::XMVectorCatmullRom(Root0, Root1, Root2, Root3, t);
            DirectX::XMVECTOR Alpha = DirectX::XMVectorCatmullRom(Alpha0, Alpha1, Alpha2, Alpha3, t);
            DirectX::XMFLOAT3 root, middle, tip;
            DirectX::XMStoreFloat3(&root, Root);
            //DirectX::XMStoreFloat3(&middle, Middle);
            DirectX::XMStoreFloat3(&tip, Tip);

            // ノイズの揺らぎを追加
            //float noiseStrength = 0.05f;
            //float noiseX = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * noiseStrength;
            //float noiseY = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * noiseStrength;
            //float noiseZ = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * noiseStrength;
            //tip.x += noiseX;
            //tip.y += noiseY;
            //tip.z += noiseZ;
            //float rootnoiseX = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * noiseStrength;
            //float rootnoiseY = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * noiseStrength;
            //float rootnoiseZ = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * noiseStrength;
            //root.x += rootnoiseX;
            //root.y += rootnoiseY;
            //root.z += rootnoiseZ;

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

            //消えるピクセルを火の粉みたいに黒くする処理　綺麗にできたら使うかも

            //color.x *=1.0 - static_cast<float>(i - 1) / static_cast<float>(weaponRight.MAX_POLYGON - 3);
            //color.y *=1.0 - static_cast<float>(i - 1) / static_cast<float>(weaponRight.MAX_POLYGON - 3);
            //color.z *=1.0 - static_cast<float>(i) / static_cast<float>(weaponRight.MAX_POLYGON - 3);

            if (i % 2 == 0)
            {
                //// ポイントライト設定
                //PointLight pointLight;
                //pointLight.position.x = tip.x;
                //pointLight.position.y = tip.y;
                //pointLight.position.z = tip.z;
                //pointLight.position.w = attenuation;
                //pointLight.color = pointColor;
                //LightManager::Instance().SetPointLight(pointLight, 50 + i / 2);
            }

            //trailRenderer->AddVertex(root, rootcolor, { alpha, 0 }, dissolve);
            //trailRenderer->AddVertex(middle, { color.x * scale,color.y * scale,color.z * scale,color.w }, { alpha, 1 }, dissolve);
            //trailRenderer->AddVertex(tip, { tipcolor.x * scale,tipcolor.y * scale,tipcolor.z * scale,tipcolor.w * scale }, { alpha, 1 }, dissolve);
        }
    }
}

void Weapon::DrawDebugTrailGui()
{
    /*if (ImGui::CollapsingHeader("Trail", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::TreeNode("Trail "))
        {
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
    ImGui::DragFloat("TrailPAttenuation", &attenuation, 0.1f);*/
}
