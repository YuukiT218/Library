#include "Character.h"
#include "Stage/StageManager.h"
#include <Math/Mathf.h>
//#include "System/MessageData.h"
//#include "System/Messenger.h"

// 行列更新処理
void Character::UpdateTransform()
{
    // スケール行列を作成
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

    // 回転行列を作成
    DirectX::XMMATRIX X = DirectX::XMMatrixRotationX(angle.x);
    DirectX::XMMATRIX Y = DirectX::XMMatrixRotationY(angle.y);
    DirectX::XMMATRIX Z = DirectX::XMMatrixRotationZ(angle.z);
    DirectX::XMMATRIX R = Y * X * Z;

    // 位置行列を作成
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

    // 3つの行列を組み合わせ、ワールド行列を作成
    DirectX::XMMATRIX W = S * R * T;

    // 計算したワールド行列を取り出す
    DirectX::XMStoreFloat4x4(&transform, W);
}

// 行列更新処理
void Character::UpdateTransform(DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 angle, DirectX::XMFLOAT3 position, DirectX::XMFLOAT4X4* transform)
{
    // スケール行列を作成
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

    // 回転行列を作成
    DirectX::XMMATRIX X = DirectX::XMMatrixRotationX(angle.x);
    DirectX::XMMATRIX Y = DirectX::XMMatrixRotationY(angle.y);
    DirectX::XMMATRIX Z = DirectX::XMMatrixRotationZ(angle.z);
    DirectX::XMMATRIX R = Y * X * Z;

    // 位置行列を作成
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

    // 3つの行列を組み合わせ、ワールド行列を作成
    DirectX::XMMATRIX W = S * R * T;

    // 計算したワールド行列を取り出す
    DirectX::XMStoreFloat4x4(transform, W);
}

// 入力値からワールドベクトルを取得
DirectX::XMFLOAT3 Character::ComputeWorldVec(const Camera& camera, float axisX, float axisY) const
{
    // カメラ方向とステッィクの入力値によって進行方向を計算する
    const DirectX::XMFLOAT3& cameraRight = camera.GetRight();
    const DirectX::XMFLOAT3& cameraFront = camera.GetFront();

    // 移動ベクトルはXZ平面に水平なベクトルになるようにする

    // カメラ右方向ベクトルをXZ単位ベクトルに変換
    float cameraRightX = cameraRight.x;
    float cameraRightZ = cameraRight.z;
    float cameraRightLength = sqrtf(cameraRightX * cameraRightX + cameraRightZ * cameraRightZ);
    if (cameraRightLength > 0.0f)
    {
        // 単位ベクトル化
        cameraRightX /= cameraRightLength;
        cameraRightZ /= cameraRightLength;
    }

    // カメラ前方向ベクトルをXZ単位ベクトルに変換
    float cameraFrontX = cameraFront.x;
    float cameraFrontZ = cameraFront.z;
    float cameraFrontLength = sqrtf(cameraFrontX * cameraFrontX + cameraFrontZ * cameraFrontZ);
    if (cameraFrontLength > 0.0f)
    {
        // 単位ベクトル化
        cameraFrontX /= cameraFrontLength;
        cameraFrontZ /= cameraFrontLength;
    }

    // スティックの水平入力値をカメラ右方向に反映し、
    // スティックの垂直入力値をカメラ前方向に反映し、
    // 進行ベクトルを計算する
    DirectX::XMFLOAT3 vec;
    vec.x = (cameraRightX * axisX) + (cameraFrontX * axisY);
    vec.z = (cameraRightZ * axisX) + (cameraFrontZ * axisY);
    // Y軸方向には移動しない
    vec.y = 0.0f;

    return vec;
}

// キャラクター前方向計算
DirectX::XMFLOAT3 Character::CharacterForward(DirectX::XMFLOAT3 angle)
{
    // 前ベクトルを計算
    float forwardX = sinf(angle.y);
    float forwardZ = cosf(angle.y);

    DirectX::XMFLOAT3 vec;
    vec.x = forwardX;
    vec.y = 0.0f;
    vec.z = forwardZ;

    return vec;
}

// キャラクター後ろ方向計算
DirectX::XMFLOAT3 Character::CharacterBack(DirectX::XMFLOAT3 angle)
{
    // 後ろベクトルを計算
    float backX = -sinf(angle.y);
    float backZ = -cosf(angle.y);

    DirectX::XMFLOAT3 vec;
    vec.x = backX;
    vec.y = 0.0f;
    vec.z = backZ;

    return vec;
}

// キャラクター左方向計算
DirectX::XMFLOAT3 Character::CharacterLeft(DirectX::XMFLOAT3 angle)
{
    // 左ベクトルを計算
    float leftX = cosf(angle.y);
    float leftZ = -sinf(angle.y);

    DirectX::XMFLOAT3 vec;
    vec.x = leftX;
    vec.y = 0.0f;
    vec.z = leftZ;

    return vec;
}

// キャラクター右方向計算
DirectX::XMFLOAT3 Character::CharacterRight(DirectX::XMFLOAT3 angle)
{
    // 右ベクトルを計算
    float rightX = -cosf(angle.y);
    float rightZ = sinf(angle.y);

    DirectX::XMFLOAT3 vec;
    vec.x = rightX;
    vec.y = 0.0f;
    vec.z = rightZ;

    return vec;
}

// ダメージを与える
bool Character::ApplyDamage(int damage, float invicibleTime, bool isState, DirectX::XMFLOAT3 HitPosition)
{
    // ダメージが0の場合は健康状態を変更する必要がない
    if (damage == 0) return false;

    // 死亡している場合は健康状態を変更しない
    if (health <= 0)
    {
        health = 0;
        return false;
    }

    // 無敵時間中はダメージを与えない
    if (invincibleTimer > 0.0f) return false;

    // ダメージ処理
    health -= damage;

    // 無敵時間設定
    invincibleTimer = invicibleTime;


    // 通知
    //{
    //    MessageData::POPUPNUMBERDATA	p;
    //    p.position = HitPosition;
    //    p.value = -damage;	//	ダメージなのでマイナス反転しておく
    //    Messenger::Instance().SendData(MessageData::POPUPNUMBEREVENT, &p);
    //}

    if (!isState)
    {
        return true;
    }

    // 死亡通知
    if (health <= 0)
    {
        OnDead();
    }
    else
    {
        OnDamaged();
    }

    // 健康状態が変更した場合はtrueを返す
    return true;
}

// 衝撃を与える
void Character::AddImpulse(const DirectX::XMFLOAT3& impulse)
{
    // 速力に力を加える
    velocity.x += impulse.x;
    velocity.y += impulse.y;
    velocity.z += impulse.z;
}

void Character::initAnimSpeed()
{
    if (model)
    {
        auto animations = model->GetResource()->GetAnimations();
        for (int i = 0; i < animations.size(); ++i)
        {
            animSpeed[i] = 1.0f;  // デフォルトのスピードを1.0fに設定
        }
    }
}

// 移動処理
void Character::Move(float vx, float vz, float speed)
{
    // 移動方向ベクトルを設定
    moveVecX = vx;
    moveVecZ = vz;

    // 最大速度設定
    maxMoveSpeed = speed;
}

// 旋回処理
void Character::Turn(float elapsedTime, float vx, float vz, float speed)
{
    speed *= elapsedTime;

    // 進行ベクトルがゼロベクトルの場合は処理する必要なし
    float len = sqrtf(vx * vx + vz * vz);
    if (len < 0.001f) return;

    // 進行ベクトルを単位ベクトル化
    vx /= len;
    vz /= len;

    // 自身の回転値から前方向を求める
    float frontX = sinf(angle.y);
    float frontZ = cosf(angle.y);

    // 回転角を求めるため、2つの単位ベクトルの内積を計算する
    float dot = frontX * vx + frontZ * vz;

    // 内積値は-1.0f～1.0fで表現されており、2つの単位ベクトルの角度が
    // 小さいほど1.0に近づくという性質を利用して回転速度を調整する
    float rot = 1.0f - dot;

    // 左右判定を行うために2つの単位ベクトルの外積を計算する
    float cross = frontX * vz - frontZ * vx;

    // 2Dの外積値が正の場合か負の場合によって左右判定が行える
    // 左右判定を行うことによって左右回転を選択する
    if (cross < 0.0f)
    {
        angle.y += rot * speed;
    }
    else
    {
        angle.y -= rot * speed;
    }

    if (angle.y > DirectX::XMConvertToRadians(360))
    {
        angle.y = DirectX::XMConvertToRadians(0);
    }
    if (angle.y < DirectX::XMConvertToRadians(0))
    {
        angle.y = DirectX::XMConvertToRadians(360);
    }

}

// ジャンプ処理
void Character::Jump(float speed)
{
    // 上方向の力を設定
    velocity.y = speed;
}

// 速力処理更新
void Character::UpdateVelocity(float elapsedTime)
{
    // 経過フレーム
    float elapsedFrame = 60.0f * elapsedTime;

    // 垂直速力更新処理
    UpdateVerticalVelocity(elapsedFrame);

    // 水平速力更新処理
    UpdateHorizontalVelocity(elapsedFrame);

    // 垂直移動更新処理
    UpdateVerticalMove(elapsedTime);

    // 水平移動更新処理
    UpdateHorizontalMove(elapsedTime);
}

// 速力更新処理
void Character::UpdateVelocity(DirectX::XMFLOAT3* position, DirectX::XMFLOAT3* angle, DirectX::XMFLOAT3* velocity, float elapsedTime)
{
    // 経過フレーム
    float elapsedFrame = 60.0f * elapsedTime;

    // 垂直速力更新処理
    UpdateVerticalVelocity(velocity, elapsedFrame);

    // 垂直移動更新処理
    UpdateVerticalMove(position, angle, velocity, elapsedTime);
}

// 垂直速力更新処理
void Character::UpdateVerticalVelocity(float elapsedTime)
{
    // 重力処理
    velocity.y += gravity * elapsedTime;
}

// 垂直速力更新処理
void Character::UpdateVerticalVelocity(DirectX::XMFLOAT3* velocity, float elapsedTime)
{
    // 重力処理
    velocity->y += gravity * elapsedTime;
}

// 垂直移動更新処理
void Character::UpdateVerticalMove(float elapsedTime)
{
    // 垂直方向の移動量
    float my = velocity.y * elapsedTime;

    // キャラクターのY軸方向となる法線ベクトル
    DirectX::XMFLOAT3 normal = { 0, 1, 0 };

    slopeRate = 0.0f;

    // 落下中
    if (my < 0.0f)
    {
        // レイの開始位置は足元より少し上
        DirectX::XMFLOAT3 start = { position.x, position.y + stepOffset, position.z };
        // レイの終点位置は移動量の位置
        DirectX::XMFLOAT3 end = { position.x, position.y + my, position.z };

#ifdef _DEBUG || DEBUG

        Graphics::Instance().GetShapeRenderer()->DrawSphere(start, 0.05f, { 0,1,0,0 });
        Graphics::Instance().GetShapeRenderer()->DrawSphere(end, 0.05f, { 0,1,0,1 });

#endif // _DEBUG || DEBUG

        // レイキャストによる地面判定
        HitResult hit;
        if (StageManager::Instance().RayCast(start, end, hit))
        {
            // 法線ベクトル取得
            normal = hit.normal;

            // 地面に接している
            position = hit.position;

#ifdef _DEBUG || DEBUG

            Graphics::Instance().GetShapeRenderer()->DrawSphere(hit.position, 0.05f, { 1,0,0,1 });

#endif // _DEBUG || DEBUG


            // 回転
            angle.x += hit.rotation.x;
            angle.y += hit.rotation.y;
            angle.z += hit.rotation.z;

            // 傾斜率の計算
            float normalLengthXZ = sqrtf(hit.normal.x * hit.normal.x + hit.normal.z * hit.normal.z);
            slopeRate = 1.0f - (hit.normal.y / (normalLengthXZ + hit.normal.y));

            // 着地した
            if (!isGround)
            {
                OnLanding();
            }
            isGround = true;
            velocity.y = 0.0f;
        }
        else
        {
            // 空中に浮いている
            position.y += my;
            isGround = false;
        }
    }
    // 上昇中
    else if (my > 0.0f)
    {
        position.y += my;
        isGround = false;
    }

    // 地面の向きに沿うようにXZ軸回転
    {
        // Y軸が法線ベクトル方向に向くオイラー角回転を算出する
        DirectX::XMFLOAT3 test = { angle.x, angle.y, angle.z };
        test.x = atan2f(normal.z, normal.y);
        test.z = -atan2f(normal.x, normal.y);

        // 線形補完で滑らかに回転する
        angle.x = Mathf::Lerp(angle.x, test.x, 0.1f);
        angle.z = Mathf::Lerp(angle.z, test.z, 0.1f);
    }
}

// 垂直移動更新処理
void Character::UpdateVerticalMove(DirectX::XMFLOAT3* position, DirectX::XMFLOAT3* angle, DirectX::XMFLOAT3* velocity, float elapsedTime)
{
    // 垂直方向の移動量
    float my = velocity->y * elapsedTime;

    // キャラクターのY軸方向となる法線ベクトル
    DirectX::XMFLOAT3 normal = { 0, 1, 0 };

    slopeRate = 0.0f;

    // 落下中
    if (my < 0.0f)
    {
        // レイの開始位置は足元より少し上
        DirectX::XMFLOAT3 start = { position->x, position->y + stepOffset, position->z };
        // レイの終点位置は移動量の位置
        DirectX::XMFLOAT3 end = { position->x, position->y + my, position->z };

#ifdef _DEBUG || DEBUG

        Graphics::Instance().GetShapeRenderer()->DrawSphere(start, 0.05f, { 0,1,0,0 });
        Graphics::Instance().GetShapeRenderer()->DrawSphere(end, 0.05f, { 0,1,0,1 });

#endif // _DEBUG || DEBUG

        // レイキャストによる地面判定
        HitResult hit;
        if (StageManager::Instance().RayCast(start, end, hit))
        {
            // 法線ベクトル取得
            normal = hit.normal;

            // 地面に接している
            position->x = hit.position.x;
            position->y = hit.position.y;
            position->z = hit.position.z;

#ifdef _DEBUG || DEBUG

            Graphics::Instance().GetShapeRenderer()->DrawSphere(hit.position, 0.05f, { 1,0,0,1 });

#endif // _DEBUG || DEBUG


            // 回転
            angle->x += hit.rotation.x;
            angle->y += hit.rotation.y;
            angle->z += hit.rotation.z;

            // 傾斜率の計算
            float normalLengthXZ = sqrtf(hit.normal.x * hit.normal.x + hit.normal.z * hit.normal.z);
            slopeRate = 1.0f - (hit.normal.y / (normalLengthXZ + hit.normal.y));

            // 着地した
            if (!isGround)
            {
                OnLanding();
            }
            isGround = true;
            velocity->y = 0.0f;
        }
        else
        {
            // 空中に浮いている
            position->y += my;
            isGround = false;
        }
    }
    // 上昇中
    else if (my > 0.0f)
    {
        position->y += my;
        isGround = false;
    }

    // 地面の向きに沿うようにXZ軸回転
    {
        // Y軸が法線ベクトル方向に向くオイラー角回転を算出する
        DirectX::XMFLOAT3 test = { angle->x, angle->y, angle->z };
        test.x = atan2f(normal.z, normal.y);
        test.z = -atan2f(normal.x, normal.y);

        // 線形補完で滑らかに回転する
        angle->x = Mathf::Lerp(angle->x, test.x, 0.1f);
        angle->z = Mathf::Lerp(angle->z, test.z, 0.1f);
    }
}

// 水平速力更新処理
void Character::UpdateHorizontalVelocity(float elapsedTime)
{
    // XZ平面の速力を減速
    float length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    if (length > 0.0f)
    {
        // 摩擦力
        float friction = this->friction * elapsedTime;

        // 空中にいる時は摩擦力を減らす
        if (isGround == false) friction *= airControl;

        // 摩擦による横方向の減速処理
        if (length > friction)
        {
            // 単位ベクトル化
            float vx = velocity.x / length;
            float vz = velocity.z / length;

            velocity.x -= vx * friction;
            velocity.z -= vz * friction;
        }
        // 横方向の速力が摩擦力以下になったので速力を無効化
        else
        {
            velocity.x = 0.0f;
            velocity.z = 0.0f;
        }
    }

    //// XZ平面の速力を加速する
    //if (length <= maxMoveSpeed)
    //{
    //    // 移動ベクトルがゼロベクトルでないなら加速する
    //    float moveVecLength = sqrtf(moveVecX * moveVecX + moveVecZ * moveVecZ);
    //    if (moveVecLength > 0.0f)
    //    {
    //        // 加速力
    //        float acceleration = this->acceleration * elapsedTime;

    //        // 空中にいる時は摩擦力を減らす
    //        if (isGround == false) acceleration *= airControl;

    //        // 移動ベクトルによる加速処理
    //        velocity.x += moveVecX * acceleration;
    //        velocity.z += moveVecZ * acceleration;

    //        // 最大速度制限
    //        float length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    //        if (length > maxMoveSpeed)
    //        {
    //            float vx = velocity.x / length;
    //            float vz = velocity.z / length;

    //            velocity.x -= vx * acceleration;
    //            velocity.z -= vz * acceleration;
    //        }

    //        // 下り坂でガタガタしないようにする
    //        if (isGround && slopeRate > 0.0f)
    //        {
    //            velocity.y -= length * slopeRate * elapsedTime;
    //        }
    //    }
    //}

    // XZ平面の速力を直接セット（加速なし）
    float moveVecLength = sqrtf(moveVecX * moveVecX + moveVecZ * moveVecZ);
    if (moveVecLength > 0.0f)
    {
        // 移動ベクトルを正規化
        float vx = moveVecX / moveVecLength;
        float vz = moveVecZ / moveVecLength;

        // 即時に最大速度で速度ベクトルを設定
        velocity.x = vx * maxMoveSpeed;
        velocity.z = vz * maxMoveSpeed;
    }

    // 移動ベクトルをリセット
    moveVecX = 0.0f;
    moveVecZ = 0.0f;
}

// 水平移動更新処理
void Character::UpdateHorizontalMove(float elapsedTime)
{
    // レイキャストデバッグ球
    float vx = sinf(angle.y) * 5.f;
    float vz = cosf(angle.y) * 5.f;
    DirectX::XMFLOAT3 pp = { position.x, position.y + 0.5f, position.z };
    DirectX::XMFLOAT3 ee = { pp.x + vx, pp.y, pp.z + vz };
    HitResult hh;
    if (StageManager::Instance().RayCast(pp, ee, hh))
    {
        Graphics::Instance().GetShapeRenderer()->DrawSphere(hh.position, 0.1f, { 1,0,0,1 });
    }


    // 水平速力量計算
    float velocityLengthXZ = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    if (velocityLengthXZ > 0.0f)
    {
        // 水平移動値
        float mx = velocity.x * elapsedTime;
        float mz = velocity.z * elapsedTime;
        DirectX::XMFLOAT3 start = { position.x, position.y + stepOffset, position.z };
        DirectX::XMFLOAT3 end = { position.x + mx, position.y + stepOffset, position.z + mz };
        // レイキャストによる壁判定
        HitResult hit;
        if (StageManager::Instance().RayCast(start, end, hit))
        {
            // 壁までのベクトル
            DirectX::XMVECTOR Start = DirectX::XMLoadFloat3(&hit.position); // 点P
            DirectX::XMVECTOR End = DirectX::XMLoadFloat3(&end); // 点B
            DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(End, Start);
            // 壁の法線
            DirectX::XMVECTOR Normal = DirectX::XMLoadFloat3(&hit.normal);
            // 入射ベクトルを法線ベクトルに射影
            DirectX::XMVECTOR Dot = DirectX::XMVector3Dot(DirectX::XMVectorNegate(Vec), Normal);
            // 補正位置の計算
            float a = DirectX::XMVectorGetX(Dot);
            a += 0.01f;
            DirectX::XMVECTOR R = DirectX::XMVectorAdd(Vec, DirectX::XMVectorScale(Normal, a));
            DirectX::XMVECTOR O = DirectX::XMVectorAdd(Start, R);
            DirectX::XMFLOAT3 o;
            DirectX::XMStoreFloat3(&o, O);
#if 1
            //壁際で壁ずり後の位置がめり込んでいないかレイキャストでチェックする
            if (StageManager::Instance().RayCast(start, o, hit))
            {
                //めり込んでいた場合はプレイヤーの位置に今回レイキャストした交点を設定する
                        //プレイヤーの位置が壁にぴったりくっつかないように補正する
                DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&hit.position);
                DirectX::XMVECTOR S = DirectX::XMLoadFloat3(&start);
                DirectX::XMVECTOR PS = DirectX::XMVectorSubtract(S, Start);
                DirectX::XMVECTOR V = DirectX::XMVector3Normalize(PS);
                Start = DirectX::XMVectorAdd(P, DirectX::XMVectorScale(V, 0.001f));
                DirectX::XMFLOAT3 p;
                DirectX::XMStoreFloat3(&p, P);
                position.x = p.x;
                position.z = p.z;
            }
            else
#endif
            {
                position.x = o.x;
                position.z = o.z;
            }
        }
        else
        {
            // 移動
            position.x += mx;
            position.z += mz;
        }
    }
}

// エリア外に行けないようにする
void Character::KeepAreaLimit(DirectX::XMFLOAT3& position)
{
    float dx = position.x - areaCenter.x;
    float dz = position.z - areaCenter.z;
    float distance = sqrt(dx * dx + dz * dz);

    // 距離が半径を超えた場合の処理
    if (distance > areaSize)
    {
        // 超えた場合、位置を円周上に制限
        float scale = areaSize / distance;  // 必要な縮尺
        position.x = areaCenter.x + dx * scale;
        position.z = areaCenter.z + dz * scale;
    }
}

// 無敵時間更新
void Character::UpdateInvincibleTimer(float elapsedTime)
{
    if (invincibleTimer > 0.0f)
    {
        invincibleTimer -= elapsedTime;
    }
}

// 全身に当たり判定を付与する
void Character::AddCollisionSpheres(std::shared_ptr<Model> model, std::vector<NodeHitSphere> nodeHitSpheres)
{
    for (const auto& node : nodeHitSpheres)
    {
        Model::Node* modelNode = model->FindNode(node.nodeName);
        if (!modelNode) continue;

        // ノードの位置を取得して球を描画
        ShapeRenderer* debugRenderer = Graphics::Instance().GetShapeRenderer();

        debugRenderer->DrawSphere(
            DirectX::XMFLOAT3(
                modelNode->worldTransform._41,
                modelNode->worldTransform._42,
                modelNode->worldTransform._43),
            node.radius,
            DirectX::XMFLOAT4(0, 0, 0, 1)
        );
    }
}
