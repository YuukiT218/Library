#include <imgui.h>
#include "CameraController.h"
#include "Camera.h"
#include "Input/Input.h"
#include "Math/Mathf.h"
#include "Math/Collision.h"
#include "Stage/StageManager.h"
#include "Graphics/Graphics.h"
#include "Camera/CameraParam.h"
//#include "Math/FastNoiseLite.h"
//
//#include	"System/MessageData.h"
//#include	"System/Messenger.h"

void CameraController::Update(float elapsedTime)
{
    auto normalizeAngle = [](float angle) {
        return angle >= 0.f
            ? fmodf((angle)+DirectX::XM_PI, DirectX::XM_2PI) - DirectX::XM_PI
            : fmodf((angle)-DirectX::XM_PI, DirectX::XM_2PI) + DirectX::XM_PI;
        };

    oldLockFlag = islockon;
    islockon = CameraParam::Instance().GetIsLockOn();

    {
        GamePad& gamePad = Input::Instance().GetGamePad();
        float ax = gamePad.GetAxisRX();
        float ay = gamePad.GetAxisRY();
        float speed = rollSpeed * elapsedTime;

        if (ax) angle.y += ax * speed;
        if (ay) angle.x += (Input::Instance().GetIsLastGamePad() ? -ay : ay) * speed;

        // マウスでカメラを操作
#if _DEBUG
        if (isMouseLock)
        {
            MouseCameraController(elapsedTime);
        }
#else
        //MouseCameraController(elapsedTime);
#endif
    }

    angle.x = std::clamp(angle.x, minAngleX, maxAngleX);

    DirectX::XMMATRIX Transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
    DirectX::XMVECTOR Front = Transform.r[2];
    DirectX::XMFLOAT3 front;
    DirectX::XMStoreFloat3(&front, Front);
    if (islockon)
    {
        EnemyAlived();
    if (closestEnemy)
    {
        lockonpoint = closestEnemy->GetPosition();
    }
    else SetLockonPoint();

    if (oldLockFlag != islockon)
    {
        sideValue = CalcSide(target, lockonpoint);
    }

    // プレイヤーとターゲットの位置
    DirectX::XMFLOAT3 playerPos = target;
    DirectX::XMFLOAT3 enemyPos = lockonpoint;

    // 両者の中間点を注視点とする
    DirectX::XMFLOAT3 midPoint;
    midPoint.x = (playerPos.x + enemyPos.x) * 0.5f;
    midPoint.y = (playerPos.y + enemyPos.y) * 0.5f + targetYoffset;
    midPoint.z = (playerPos.z + enemyPos.z) * 0.5f;

    // プレイヤーとターゲット間の距離を計算
    DirectX::XMVECTOR vPlayer = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR vEnemy = DirectX::XMLoadFloat3(&enemyPos);
    DirectX::XMVECTOR vDiff = DirectX::XMVectorSubtract(vEnemy, vPlayer);
    
    float distance = 0.0f;
    DirectX::XMStoreFloat(&distance, DirectX::XMVector3Length(vDiff));

    // 距離が極端に近い場合の処理
    if (distance < 0.1f)
    {
        distance = 0.1f;
        vDiff = DirectX::XMVectorSet(0, 0, 0.1f, 0); // デフォルト方向
    }

    // カメラ距離を対象間の距離から算出
    float baseCameraDistance = distance * 0.8f;
    
    // 最小・最大距離でクランプ
    float minCameraDist = lengthLimit[0];
    float maxCameraDist = lengthLimit[1];
    float cameraDistance = std::clamp(baseCameraDistance, minCameraDist, maxCameraDist);

    // FOVを考慮した追加距離
    cameraDistance *= lockOnFovCorrection;
    cameraDistance = std::clamp(cameraDistance, minCameraDist, maxCameraDist);

    // プレイヤーとターゲットを結ぶベクトル（水平面投影版を使用）
    DirectX::XMFLOAT3 diffFloat;
    DirectX::XMStoreFloat3(&diffFloat, vDiff);
    
    // 水平方向のベクトルを作成（Y成分を0にする）
    DirectX::XMFLOAT3 horizontalDiff = diffFloat;
    horizontalDiff.y = 0;
    
    DirectX::XMVECTOR forward = DirectX::XMLoadFloat3(&horizontalDiff);
    float horizontalLength = 0.0f;
    DirectX::XMStoreFloat(&horizontalLength, DirectX::XMVector3Length(forward));
    
    // 水平距離が極端に短い場合（ほぼ真上or真下）
    // 前回の右ベクトルか、デフォルトの方向を使用
    DirectX::XMVECTOR right;
    if (horizontalLength < 0.1f)
    {
        // 前回の右ベクトルを使用（急激な変化を防ぐ）
        right = DirectX::XMLoadFloat3(&lastCameraRight);
        
        // forwardベクトルをデフォルト方向に設定
        if (horizontalLength < 0.01f)
        {
            forward = DirectX::XMVectorSet(0, 0, 1, 0); // Z軸正方向
        }
        else
        {
            forward = DirectX::XMVector3Normalize(forward);
        }
    }
    else
    {
        // 通常時：水平方向ベクトルを正規化
        forward = DirectX::XMVector3Normalize(forward);
        
        // 右方向ベクトルを計算
        DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0, 1, 0, 0);
        right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(worldUp, forward));
        
        // 右ベクトルを保存（次回の参照用）
        DirectX::XMStoreFloat3(&lastCameraRight, right);
    }

    // 斜め上からの視点を作成
    float heightAngle = DirectX::XMConvertToRadians(lockOnHeightAngle);
    float sideAngle = DirectX::XMConvertToRadians(lockOnSideAngle);

    // カメラの基準位置（中間点の後方）
    DirectX::XMVECTOR cameraOffset = DirectX::XMVectorNegate(forward);
    
    // 上方向の成分を追加
    DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0, 1, 0, 0);
    float heightRatio = tanf(heightAngle);
    DirectX::XMVECTOR heightOffset = DirectX::XMVectorScale(worldUp, cameraDistance * heightRatio);
    
    // 横方向の成分を追加
    DirectX::XMVECTOR sideOffset = DirectX::XMVectorScale(right, sideValue * cameraDistance * tanf(sideAngle));

    // 最終的なカメラ位置を計算
    DirectX::XMVECTOR vMidPoint = DirectX::XMLoadFloat3(&midPoint);
    DirectX::XMVECTOR eyePos = vMidPoint;
    
    // 後方への距離
    float backwardDist = cameraDistance * cosf(heightAngle);
    eyePos = DirectX::XMVectorMultiplyAdd(cameraOffset, 
                                          DirectX::XMVectorReplicate(backwardDist), 
                                          eyePos);
    
    // 上方向のオフセット
    eyePos = DirectX::XMVectorAdd(eyePos, heightOffset);
    
    // 横方向のオフセット
    eyePos = DirectX::XMVectorAdd(eyePos, sideOffset);

    DirectX::XMStoreFloat3(&eye, eyePos);
    
    // 注視点を設定
    lockonpoint = midPoint;

    // デバッグ用：カメラ高さの微調整
    float heightAdjust = targetLimit[0] + 
                        (targetLimit[1] - targetLimit[0]) * 
                        std::clamp((distance - 5.0f) / 15.0f, 0.0f, 1.0f);
    eye.y += heightAdjust;
    }
    else
    {
        SetLockonPoint();
        angle.y = normalizeAngle(angle.y);

        // ロックオンを解除した直後であれば、eyeの更新をスキップ
        if (!(oldLockFlag && !islockon)) {
            eye = {
                target.x - front.x * range,
                target.y - front.y * range,
                target.z - front.z * range
            };
        }
        else
        {
            DirectX::XMVECTOR vec = DirectX::XMVectorSubtract(
                DirectX::XMLoadFloat3(&target),
                DirectX::XMLoadFloat3(&eye));
            vec = DirectX::XMVector3Normalize(vec);

            float yaw = atan2f(vec.m128_f32[0], vec.m128_f32[2]);     // y軸回転（左右）
            float pitch = asinf(-vec.m128_f32[1]);                     // x軸回転（上下）

            angle.y = yaw;
            angle.x = pitch;
            eye = newEye;
        }
    }

#ifdef CameraLerp

    newEye.x = Mathf::Lerp(newEye.x, eye.x, lerpSpeed * elapsedTime);
    newEye.y = Mathf::Lerp(newEye.y, eye.y, lerpSpeed * elapsedTime);
    newEye.z = Mathf::Lerp(newEye.z, eye.z, lerpSpeed * elapsedTime);

    if (islockon)
    {
        if (closestEnemy)
        {
            newTarget = {
                Mathf::Lerp(newTarget.x, lockonpoint.x, lerpSpeed * elapsedTime),
                Mathf::Lerp(newTarget.y, lockonpoint.y + 0.9f, lerpSpeed * elapsedTime),
                Mathf::Lerp(newTarget.z, lockonpoint.z, lerpSpeed * elapsedTime)
            };
        }
    }
    else
    {
        newTarget = {
            Mathf::Lerp(newTarget.x, target.x, lerpSpeed * elapsedTime),
            Mathf::Lerp(newTarget.y, target.y + 0.9f, lerpSpeed * elapsedTime),
            Mathf::Lerp(newTarget.z, target.z, lerpSpeed * elapsedTime)
        };
    }

    newTarget = {
            Mathf::Lerp(newTarget.x, target.x, lerpSpeed * elapsedTime),
            Mathf::Lerp(newTarget.y, target.y + 0.9f, lerpSpeed * elapsedTime),
            Mathf::Lerp(newTarget.z, target.z, lerpSpeed * elapsedTime)
    };
#else
    eye.x = target.x - front.x * range;
    eye.y = target.y - front.y * range;
    eye.z = target.z - front.z * range;
#endif

#ifdef CameraLerp
    // レイキャストによる地面判定
    HitResult hit;
    if (StageManager::Instance().RayCast(newTarget, newEye, hit))
    {
        newEye = hit.position;
    }

    // カメラ視点と注視点を設定
    Camera::Instance().SetLookAt(newEye, newTarget, DirectX::XMFLOAT3(0, 1, 0));
#else
    // レイキャストによる地面判定
    HitResult hit;
    if (StageManager::Instance().RayCast(target, eye, hit))
    {
        eye = hit.position;
    }

    // カメラ視点と注視点を設定
    Camera::Instance().SetLookAt(eye, target, DirectX::XMFLOAT3(0, 1, 0));
#endif

    // フリーカメラにするかどうか
    if (freeCameraFlag)
    {
        Camera::Instance().SetFreeCameraFlag(true);
    }
    else
    {
        Camera::Instance().SetFreeCameraFlag(false);
    }

    CameraShake(elapsedTime);
}

void CameraController::SetLockonPoint()
{
    SilverDragonkin& dragonkin = SilverDragonkin::Instance();

    closestEnemy = nullptr;

    closestEnemy = dynamic_cast<Enemy*>(&dragonkin); // Use dynamic_cast to convert SilverDragonkin to Enemy  

    if (closestEnemy)  
    {  
        DirectX::XMFLOAT3 enemyPosition = closestEnemy->GetPosition();  
        lockonpoint = enemyPosition;  
        CameraParam::Instance().SetLockOnEnemy(closestEnemy);  
    }  
    else  
    {  
        CameraParam::Instance().SetIsLockOn(false);  
    }  
}


void CameraController::EnemyAlived()
{
    /*EnemyManager& enemyManager = EnemyManager::Instance();
    int enemyCount = enemyManager.GetEnemyCount();

    bool enemyAlived = false;
    for (int i = 0; i < enemyCount; ++i)
    {
        Enemy* enemy = enemyManager.GetEnemy(i);
        DirectX::XMFLOAT3 enemyPosition = enemy->GetPosition();

        if (enemy == closestEnemy)
        {
            enemyAlived = true;
        }
    }

    if (!enemyAlived)
    {
        closestEnemy = nullptr;
    }*/
}

// カメラシェイク
void CameraController::CameraShake(float elapsedTime)
{
    //Camera& camera = Camera::Instance();

    //if (!camera.GetCameraShakeSwitch()) return;

    //if (shakeTime < camera.GetCameraShakeTimer())
    //{
    //    shakeTime = camera.GetCameraShakeTimer();
    //    camera.SetCameraShakeSwitch(true, 0, camera.GetCameraShakePower());
    //}

    //// シンプレックスノイズを使用
    //static FastNoiseLite noiseGenerator;
    //noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    //noiseGenerator.SetFrequency(2.0f);  // 周波数（細かさ調整）

    //// 時間を利用してスムーズな変化を作る
    //static float timeOffset = 0.0f;
    //timeOffset += elapsedTime * 5.0f; // 時間を少しずつ進める（速度調整）

    //// カメラの揺れの強さ
    //float shakePower = camera.GetCameraShakePower() * cameraShakeRange;

    //// 各軸ごとに異なるノイズ値を取得
    //DirectX::XMFLOAT3 shake;
    //shake.x = noiseGenerator.GetNoise(timeOffset, 0.0f) * shakePower;
    //shake.y = noiseGenerator.GetNoise(0.0f, timeOffset) * shakePower;
    //shake.z = noiseGenerator.GetNoise(timeOffset, timeOffset) * shakePower;

    //// 注視点に揺れ値を加える
    //DirectX::XMFLOAT3 focus = camera.GetFocus();
    //focus.x += shake.x;
    //focus.y += shake.y;
    //focus.z += shake.z;

    //camera.SetLookAt(camera.GetEye(), focus, camera.GetUp());

    //shakeTime -= elapsedTime;

    //if (shakeTime < 0)
    //{
    //    camera.SetCameraShakeSwitch(false, 0);
    //}
}

// マウスカメラ操作
void CameraController::MouseCameraController(float elapsedTime)
{
    Mouse& mouse = Input::Instance().GetMouse();
    float bx = mouse.GetPositionX();
    float by = mouse.GetPositionY();
    float oldbx = mouse.GetOldPositionX();
    float oldby = mouse.GetOldPositionY();

    float mouseSpeed = mouseRollSpeed * elapsedTime;
    {
        DirectX::XMFLOAT2 direction;
        direction.x = bx - oldbx;
        direction.y = by - oldby;
        DirectX::XMVECTOR dir = DirectX::XMLoadFloat2(&direction);
        DirectX::XMVector2Normalize(dir);

        angle.y += direction.x * mouseSpeed;
        angle.x += direction.y * mouseSpeed;
    }
}

// デバッグ用GUI描画
void CameraController::DrawDebugGUI()
{
    // トランスフォーム
    if (ImGui::CollapsingHeader("CameraController", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // 回転
        DirectX::XMFLOAT3 a;
        a.x = DirectX::XMConvertToDegrees(angle.x);
        a.y = DirectX::XMConvertToDegrees(angle.y);
        a.z = DirectX::XMConvertToDegrees(angle.z);
        ImGui::DragFloat3("Angle", &a.x);
        angle.x = DirectX::XMConvertToRadians(a.x);
        angle.y = DirectX::XMConvertToRadians(a.y);
        angle.z = DirectX::XMConvertToRadians(a.z);

        ImGui::DragFloat3("offsetTarget", &offsetTarget.x, 0.1f);

        ImGui::DragFloat("range", &range, 0.1f);

        float b;
        b = DirectX::XMConvertToDegrees(angleOffset);
        ImGui::DragFloat("angleOffset", &b);
        angleOffset = DirectX::XMConvertToRadians(b);

        ImGui::DragFloat("LeapSpeed", &lerpSpeed);
        ImGui::DragFloat("AngleLeapSpeed", &AnglelerpSpeed);


        ImGui::DragFloat("dotOffset", &dotOffset);
        ImGui::DragFloat("turnSpeed", &turnSpeed);

        if (ImGui::TreeNode("Lock-On Camera Settings"))
        {
            ImGui::DragFloat("Height Angle", &lockOnHeightAngle, 1.0f, 10.0f, 80.0f);
            ImGui::DragFloat("Side Angle", &lockOnSideAngle, 1.0f, 0.0f, 60.0f);
            ImGui::DragFloat("FOV Correction", &lockOnFovCorrection, 0.1f, 1.0f, 3.0f);
            ImGui::Text("Height Angle: カメラの見下ろし角度");
            ImGui::Text("Side Angle: カメラの横ズレ角度");
            ImGui::Text("FOV Correction: 画面収まり補正");
            ImGui::TreePop();
        }
        ImGui::DragFloat("TargetY Offset", &targetYoffset, 0.1f);

        ImGui::DragFloat("lengthLimit 1", &lengthLimit[0], 0.1f, -10.0f, lengthLimit[1]);
        ImGui::DragFloat("lengthLimit 2", &lengthLimit[1], 0.1f, lengthLimit[0], 500.0f);

        ImGui::DragFloat("TargetLimit 1", &targetLimit[0], 0.1f);
        ImGui::DragFloat("TargetLimit 2", &targetLimit[1], 0.1f);

        ImGui::DragFloat("Min Target Distance", &minTargetDistance, 0.1f, 1.0f, maxTargetDistance);
        ImGui::DragFloat("Max Target Distance", &maxTargetDistance, 0.1f, minTargetDistance, 50.0f);
        ImGui::DragFloat("Distance Easing", &distanceEasingPower, 0.1f, 0.5f, 3.0f);

        // カメラを自由に動かせるかどうか
        ImGui::Checkbox("FreeCamera", &freeCameraFlag);

        ImGui::Text("DisAngle %3f", desiredAngle.y);
    }
}

void CameraController::DrawDebugPrimitive()
{
    //DebugRenderer* debugRenderer = Graphics::Instance().GetDebugRenderer();

    //if (closestEnemy)
    //{
    //    debugRenderer->DrawLine(target, closestEnemy->GetPosition(), { 1,1,0,1 });

    //    Camera& camera = Camera::Instance();
    //    DirectX::XMFLOAT3 eye = camera.GetEye();
    //    DirectX::XMFLOAT3 front = camera.GetFront();
    //    DirectX::XMVECTOR Front = DirectX::XMLoadFloat3(&front);
    //    DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&eye);
    //    Front = DirectX::XMVectorAdd(Front, Eye);
    //    DirectX::XMStoreFloat3(&front, Front);

    //    front.y -= 0.3f;
    //    debugRenderer->DrawLine(closestEnemy->GetPosition(), front, { 1,0,0,1 });
    //    debugRenderer->DrawLine(target, front, { 1,0,0,1 });







    //    // メッセージで角度情報を送信
    //    //{
    //    //    MessageData::RELATIONALMETRICSDATA p;
    //    //    p.position = mid;
    //    //    p.value = angleDeg;
    //    //    Messenger::Instance().SendData(MessageData::RELATIONALMETRICSEVENT, &p);
    //    //}
    //}
}

void CameraController::InitCamera()
{
    newEye = Camera::Instance().GetEye();
    newTarget = Camera::Instance().GetFocus();
}

float CameraController::CalcSide(DirectX::XMFLOAT3 p1, DirectX::XMFLOAT3 p2)
{
    // 外積を用いて横軸のズレ方向算出
    DirectX::XMFLOAT2	v{};
    v.x = target.x - lockonpoint.x;
    v.y = target.z - lockonpoint.z;
    float	l = sqrtf(v.x * v.x + v.y * v.y);
    v.y /= l;
    v.y /= l;
    DirectX::XMFLOAT2	n;
    n.x = p1.x - p2.x;
    n.y = p1.z - p2.z;
    l = sqrtf(n.x * n.x + n.y * n.y);
    n.x /= l;
    n.y /= l;
    return	((v.x * n.y) - (v.y * n.x) < 0) ? +1.0f : -1.0f;
}