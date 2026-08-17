#include "Character.h"
#include "Stage/StageManager.h"
#include <Math/Mathf.h>

#include <stdlib.h>

namespace
{
    // 1秒あたりの基準フレーム数（速力計算をフレーム基準に揃えるための係数）
    constexpr float REFERENCE_FPS = 60.0f;

    // 接地判定レイの始点を足元からどれだけ持ち上げるか
    // ※完全に足元から始めると床ポリゴンと重なって判定がすり抜けるため
    constexpr float GROUND_RAY_START_OFFSET = 0.5f;

    // 接地判定レイをどれだけ下方向に伸ばすか（これより離れていれば無限遠扱い）
    constexpr float GROUND_RAY_LENGTH = 10.0f;

    // 地面の傾きに追従する際の線形補完係数
    constexpr float SLOPE_ROTATION_LERP_RATE = 0.1f;

    // 旋回処理でゼロベクトルとみなす閾値
    constexpr float TURN_INPUT_EPSILON = 0.001f;

    // 壁ずり後の位置を壁からわずかに離すための補正量
    constexpr float WALL_SLIDE_PUSH_OUT = 0.01f;

    // デバッグ表示用の球の半径
    constexpr float DEBUG_SPHERE_RADIUS = 0.05f;

    // デバッグ用の前方向レイの長さ
    constexpr float DEBUG_FORWARD_RAY_LENGTH = 5.0f;

    // デバッグ用のヒット位置表示球の半径
    constexpr float DEBUG_HIT_SPHERE_RADIUS = 0.1f;

    // 1回転を表す角度
    constexpr float FULL_TURN_RADIAN = DirectX::XM_2PI;

    // アニメーション再生速度の初期値
    constexpr float DEFAULT_ANIM_SPEED = 1.0f;

    // スケール・回転・平行移動からワールド行列を組み立てる
    // 回転はY(ヨー)→X(ピッチ)→Z(ロール)の順で合成する
    DirectX::XMMATRIX ComposeWorldMatrix(const DirectX::XMFLOAT3& scale, const DirectX::XMFLOAT3& angle, const DirectX::XMFLOAT3& position)
    {
        DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
        DirectX::XMMATRIX X = DirectX::XMMatrixRotationX(angle.x);
        DirectX::XMMATRIX Y = DirectX::XMMatrixRotationY(angle.y);
        DirectX::XMMATRIX Z = DirectX::XMMatrixRotationZ(angle.z);
        DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

        return S * (Y * X * Z) * T;
    }
}

// 行列更新処理
void Character::UpdateTransform()
{
    UpdateTransform(scale, angle, position, &transform);
}

// 行列更新処理
void Character::UpdateTransform(DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 angle, DirectX::XMFLOAT3 position, DirectX::XMFLOAT4X4* transform)
{
    DirectX::XMStoreFloat4x4(transform, ComposeWorldMatrix(scale, angle, position));
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
    return { sinf(angle.y), 0.0f, cosf(angle.y) };
}

// キャラクター後ろ方向計算
DirectX::XMFLOAT3 Character::CharacterBack(DirectX::XMFLOAT3 angle)
{
    return { -sinf(angle.y), 0.0f, -cosf(angle.y) };
}

// キャラクター左方向計算
DirectX::XMFLOAT3 Character::CharacterLeft(DirectX::XMFLOAT3 angle)
{
    return { cosf(angle.y), 0.0f, -sinf(angle.y) };
}

// キャラクター右方向計算
DirectX::XMFLOAT3 Character::CharacterRight(DirectX::XMFLOAT3 angle)
{
    return { -cosf(angle.y), 0.0f, sinf(angle.y) };
}

// 地面との距離を取得
float Character::GetDistanceFromGround()
{
    // レイの始点は足元から少しだけ持ち上げ、そこから真下に十分な距離を伸ばす
    DirectX::XMFLOAT3 start = { position.x, position.y + GROUND_RAY_START_OFFSET, position.z };
    DirectX::XMFLOAT3 end = { start.x, start.y - GROUND_RAY_LENGTH, start.z };

    // ステージマネージャーを通してレイキャストを実行
    HitResult hit;
    if (StageManager::Instance().RayCast(start, end, hit))
    {
        // 「現在の足元のY座標」と「ヒットした地面のY座標」の差分を返す
        return position.y - hit.position.y;
    }

    // 地面が見つからない場合（崖の外や、空高くにいる場合）は
    // 判定に引っかからないよう、非常に大きな値を返す
    return FLT_MAX;
}

// ダメージを与える
bool Character::ApplyDamage(int damage, float invincibleTime, bool isState, DirectX::XMFLOAT3 hitPosition)
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

    // 致命傷を受けても倒れない状態なら、体力1で踏みとどまる
    if (health <= 0 && ShouldSurviveLethalDamage())
    {
        health = 1;
    }

    // 無敵時間設定
    invincibleTimer = invincibleTime;

    if (!isState)
    {
        return true;
    }

    // 死亡通知
    // スーパーアーマー中でも体力が尽きたら必ず死亡させる。
    // ここでスーパーアーマーを見てしまうと、体力が0のままOnDeadが呼ばれず、
    // 以降は関数先頭の早期リターンに阻まれて二度と死ねなくなる。
    // なお、被弾リアクションの打ち消しはEnemyBoss側で別途行っている。
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

// ターゲットとの距離を計算
float Character::CalcTargetDist(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 targetPosition)
{
    float vx = targetPosition.x - position.x;
    float vy = targetPosition.y - position.y;
    float vz = targetPosition.z - position.z;

    return sqrtf(vx * vx + vy * vy + vz * vz);
}

// アニメーション再生速度を初期化
void Character::InitAnimSpeed()
{
    if (!model) return;

    const auto& animations = model->GetResource()->GetAnimations();
    for (int i = 0; i < animations.size(); ++i)
    {
        animSpeed[i] = DEFAULT_ANIM_SPEED;
    }
}

// 移動処理
void Character::Move(float vx, float vz, float speed)
{
    // 移動方向ベクトルを設定
    moveVecX = vx;
    moveVecZ = vz;

    // 最大速度設定
    maxSpeed = speed;
}

// 旋回処理
void Character::Turn(float elapsedTime, float vx, float vz, float speed)
{
    speed *= elapsedTime;

    // 進行ベクトルがゼロベクトルの場合は処理する必要なし
    float len = sqrtf(vx * vx + vz * vz);
    if (len < TURN_INPUT_EPSILON) return;

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

    // 回転角を0～360度の範囲に収める
    if (angle.y > FULL_TURN_RADIAN) angle.y = 0.0f;
    if (angle.y < 0.0f) angle.y = FULL_TURN_RADIAN;
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
    float elapsedFrame = REFERENCE_FPS * elapsedTime;

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
    float elapsedFrame = REFERENCE_FPS * elapsedTime;

    // 垂直速力更新処理
    UpdateVerticalVelocity(velocity, elapsedFrame);

    // 垂直移動更新処理
    UpdateVerticalMove(position, angle, velocity, elapsedTime);
}

// 垂直速力更新処理
void Character::UpdateVerticalVelocity(float elapsedTime)
{
    UpdateVerticalVelocity(&velocity, elapsedTime);
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
    UpdateVerticalMove(&position, &angle, &velocity, elapsedTime);
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

#if defined(_DEBUG) || defined(DEBUG)

        Graphics::Instance().GetShapeRenderer()->DrawSphere(start, DEBUG_SPHERE_RADIUS, { 0,1,0,0 });
        Graphics::Instance().GetShapeRenderer()->DrawSphere(end, DEBUG_SPHERE_RADIUS, { 0,1,0,1 });

#endif

        // レイキャストによる地面判定
        HitResult hit;
        if (StageManager::Instance().RayCast(start, end, hit))
        {
            // 法線ベクトル取得
            normal = hit.normal;

            // 地面に接している
            *position = hit.position;

#if defined(_DEBUG) || defined(DEBUG)

            Graphics::Instance().GetShapeRenderer()->DrawSphere(hit.position, DEBUG_SPHERE_RADIUS, { 1,0,0,1 });

#endif

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
        float targetAngleX = atan2f(normal.z, normal.y);
        float targetAngleZ = -atan2f(normal.x, normal.y);

        // 線形補完で滑らかに回転する
        angle->x = Mathf::Lerp(angle->x, targetAngleX, SLOPE_ROTATION_LERP_RATE);
        angle->z = Mathf::Lerp(angle->z, targetAngleZ, SLOPE_ROTATION_LERP_RATE);
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

    // XZ平面の速力を直接セット（加速なし）
    float moveVecLength = sqrtf(moveVecX * moveVecX + moveVecZ * moveVecZ);
    if (moveVecLength > 0.0f)
    {
        // 移動ベクトルを正規化
        float vx = moveVecX / moveVecLength;
        float vz = moveVecZ / moveVecLength;

        // 即時に最大速度で速度ベクトルを設定
        velocity.x = vx * maxSpeed;
        velocity.z = vz * maxSpeed;
    }

    // 移動ベクトルをリセット
    moveVecX = 0.0f;
    moveVecZ = 0.0f;
}

// 水平移動更新処理
void Character::UpdateHorizontalMove(float elapsedTime)
{
#if defined(_DEBUG) || defined(DEBUG)

    // 前方向へのレイキャスト結果をデバッグ球で可視化する
    {
        DirectX::XMFLOAT3 forward = CharacterForward(angle);
        DirectX::XMFLOAT3 debugStart = { position.x, position.y + GROUND_RAY_START_OFFSET, position.z };
        DirectX::XMFLOAT3 debugEnd =
        {
            debugStart.x + forward.x * DEBUG_FORWARD_RAY_LENGTH,
            debugStart.y,
            debugStart.z + forward.z * DEBUG_FORWARD_RAY_LENGTH
        };

        HitResult debugHit;
        if (StageManager::Instance().RayCast(debugStart, debugEnd, debugHit))
        {
            Graphics::Instance().GetShapeRenderer()->DrawSphere(debugHit.position, DEBUG_HIT_SPHERE_RADIUS, { 1,0,0,1 });
        }
    }

#endif

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
            float projection = DirectX::XMVectorGetX(Dot) + WALL_SLIDE_PUSH_OUT;
            DirectX::XMVECTOR R = DirectX::XMVectorAdd(Vec, DirectX::XMVectorScale(Normal, projection));
            DirectX::XMVECTOR O = DirectX::XMVectorAdd(Start, R);
            DirectX::XMFLOAT3 o;
            DirectX::XMStoreFloat3(&o, O);

            // 壁際で壁ずり後の位置がめり込んでいないかレイキャストでチェックする
            if (StageManager::Instance().RayCast(start, o, hit))
            {
                // めり込んでいた場合は今回レイキャストした交点を位置として設定する
                position.x = hit.position.x;
                position.z = hit.position.z;
            }
            else
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

        if (drawCollisionPrimitive)
        {
	        debugRenderer->DrawSphere(
			   DirectX::XMFLOAT3(
				   modelNode->worldTransform._41,
				   modelNode->worldTransform._42,
				   modelNode->worldTransform._43),
			   node.radius,
			   DirectX::XMFLOAT4(0, 0, 0, 1));
        }
    }
}
