#include <imgui.h>
#include "CameraController.h"
#include "Camera.h"
#include "Input/Input.h"
#include "Math/Mathf.h"
#include "Math/Collision.h"
#include "Stage/StageManager.h"
#include "Graphics/Graphics.h"
#include "Camera/CameraParam.h"
#include "Math/FastNoiseLite.h"
//
//#include	"System/MessageData.h"
//#include	"System/Messenger.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

void CameraController::Update(float elapsedTime)
{
    auto normalizeAngle = [](float angle) {
        return angle >= 0.f
            ? fmodf((angle)+DirectX::XM_PI, DirectX::XM_2PI) - DirectX::XM_PI
            : fmodf((angle)-DirectX::XM_PI, DirectX::XM_2PI) + DirectX::XM_PI;
        };

    oldLockFlag = islockon;
    islockon = CameraParam::Instance().GetIsLockOn();

    // 通常カメラ操作（ロックオンしていない時）
    {
        GamePad& gamePad = Input::Instance().GetGamePad();
        float ax = gamePad.GetAxisRX();
        float ay = gamePad.GetAxisRY();
        float speed = rollSpeed * elapsedTime;

        if (ax) angle.y += ax * speed;
        if (ay) angle.x += (Input::Instance().GetIsLastGamePad() ? -ay : ay) * speed;

#if _DEBUG
        if (isMouseLock)
        {
            MouseCameraController(elapsedTime);
        }
#endif
    }

    angle.x = std::clamp(angle.x, minAngleX, maxAngleX);

    DirectX::XMMATRIX Transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
    DirectX::XMVECTOR Front = Transform.r[2];
    DirectX::XMFLOAT3 front;
    DirectX::XMStoreFloat3(&front, Front);

    if (islockon)
    {
        if (closestEnemy->IsTeleporting())
        {
            lockonpoint = closestEnemy->GetCameraTrackingPosition();
        }
        else
        {
            lockonpoint = closestEnemy->GetPosition();
        }

        // ===== テレポート検知処理 =====
        bool isTeleported = false;

        if (!isFirstLockOn)
        {
            // 前フレームとの距離を計算
            DirectX::XMVECTOR vPrevPos = DirectX::XMLoadFloat3(&previousEnemyPosition);
            DirectX::XMVECTOR vCurrentPos = DirectX::XMLoadFloat3(&lockonpoint);
            DirectX::XMVECTOR vDiff = DirectX::XMVectorSubtract(vCurrentPos, vPrevPos);
            float moveDistance = DirectX::XMVectorGetX(DirectX::XMVector3Length(vDiff));

            // テレポート判定
            if (moveDistance > teleportDetectionThreshold)
            {
                isTeleported = true;
                teleportRecoveryTime = teleportRecoveryDuration;
            }
        }
        else
        {
            // 初回ロックオン時は前の位置を初期化
            isFirstLockOn = false;
            smoothedLockonPoint = lockonpoint;
        }

        // 前フレームの位置を保存
        previousEnemyPosition = lockonpoint;

        // テレポート回復時間の更新
        if (teleportRecoveryTime > 0.0f)
        {
            teleportRecoveryTime -= elapsedTime;
            if (teleportRecoveryTime < 0.0f)
            {
                teleportRecoveryTime = 0.0f;
            }
        }

        // スムーズなロックオン位置の補間
        {
            DirectX::XMVECTOR vSmoothed = DirectX::XMLoadFloat3(&smoothedLockonPoint);
            DirectX::XMVECTOR vTarget = DirectX::XMLoadFloat3(&lockonpoint);

            // テレポート中は補間を遅くする
            float smoothFactor = lockonSmoothSpeed;
            /*if (teleportRecoveryTime > 0.0f)
            {
                smoothFactor *= teleportSlowdownFactor;
            }*/

            float smoothT = (std::min)(smoothFactor * elapsedTime, 1.0f);
            DirectX::XMVECTOR vResult = DirectX::XMVectorLerp(vSmoothed, vTarget, smoothT);
            DirectX::XMStoreFloat3(&smoothedLockonPoint, vResult);
        }

        if (oldLockFlag != islockon)
        {
            sideValue = CalcSide(target, smoothedLockonPoint);

            // ロックオン開始時に現在のカメラ状態を初期化
            Camera& camera = Camera::Instance();
            currentCameraPosition = camera.GetEye();
            currentRotation = camera.GetRotation();
            targetRotation = currentRotation;

            // 初期化フラグをリセット
            isFirstLockOn = true;
        }

        // プレイヤーとターゲットの位置（スムーズ化された位置を使用）
        DirectX::XMFLOAT3 playerPos = target;
        DirectX::XMFLOAT3 enemyPos = smoothedLockonPoint;

        // 距離計算
        DirectX::XMVECTOR vPlayer = DirectX::XMLoadFloat3(&playerPos);
        DirectX::XMVECTOR vEnemy = DirectX::XMLoadFloat3(&enemyPos);
        DirectX::XMVECTOR vDiff = DirectX::XMVectorSubtract(vEnemy, vPlayer);

        float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(vDiff));

        if (distance < 0.1f)
        {
            distance = 0.1f;
            vDiff = DirectX::XMVectorSet(0, 0, 0.1f, 0);
        }

        if (std::isnan(distance) || std::isinf(distance))
        {
            distance = 5.0f;
            vDiff = DirectX::XMVectorSet(0, 0, 5.0f, 0);
        }

        // =============================================
        // 注視点の決定
        // =============================================
        DirectX::XMFLOAT3 focusTarget;

        focusTarget = enemyPos;

        // プレイヤーの位置をスムーズ化（攻撃モーション時の急激な動きを抑制）
        {
            DirectX::XMVECTOR vSmoothedPlayer = DirectX::XMLoadFloat3(&smoothedPlayerPosition);
            DirectX::XMVECTOR vCurrentPlayer = DirectX::XMLoadFloat3(&playerPos);

            // XZ平面のみをスムーズ化（Y軸は維持）
            DirectX::XMVECTOR vPlayerXZ = DirectX::XMVectorSetY(vCurrentPlayer, 0.0f);
            DirectX::XMVECTOR vSmoothedXZ = DirectX::XMVectorSetY(vSmoothedPlayer, 0.0f);

            float playerSmoothT = (std::min)(playerPositionSmoothSpeed * elapsedTime, 1.0f);
            DirectX::XMVECTOR vResultXZ = DirectX::XMVectorLerp(vSmoothedXZ, vPlayerXZ, playerSmoothT);

            // Y座標は即座に反映
            DirectX::XMVECTOR vResult = DirectX::XMVectorSetY(vResultXZ, DirectX::XMVectorGetY(vCurrentPlayer));
            DirectX::XMStoreFloat3(&smoothedPlayerPosition, vResult);
        }
        // =============================================
        // 画角内チェック（Cameraクラスの機能を使用）
        // =============================================
        Camera& camera = Camera::Instance();
        bool playerInView = camera.IsInViewport(playerPos, viewportMargin);
        bool enemyInView = camera.IsInViewport(enemyPos, viewportMargin);
        bothInView = playerInView && enemyInView;

        // =============================================
        // カメラ位置の計算
        // =============================================
        float t = (distance - distanceParamMin) / (distanceParamMax - distanceParamMin);
        t = std::clamp(t, 0.0f, 1.0f);

        float dynamicOffsetY = minOffsetTargetY + (maxOffsetTargetY - minOffsetTargetY) * t;
        float dynamicHeightAngle = minHeightAngle + (maxHeightAngle - minHeightAngle) * t;
        float dynamicFovCorrection = maxFovCorrection - (maxFovCorrection - minFovCorrection) * t;

        DirectX::XMFLOAT3 midPoint;
        midPoint.x = (smoothedPlayerPosition.x + enemyPos.x) * 0.5f;
        midPoint.y = (smoothedPlayerPosition.y + enemyPos.y) * 0.5f + dynamicOffsetY;
        midPoint.z = (smoothedPlayerPosition.z + enemyPos.z) * 0.5f;

        float baseCameraDistance = distance * 0.8f;
        float minCameraDist = lengthLimit[0];
        float maxCameraDist = lengthLimit[1];
        float cameraDistance = std::clamp(baseCameraDistance, minCameraDist, maxCameraDist);
        cameraDistance *= dynamicFovCorrection;
        cameraDistance = std::clamp(cameraDistance, minCameraDist, maxCameraDist);

        DirectX::XMFLOAT3 diffFloat;
        DirectX::XMStoreFloat3(&diffFloat, vDiff);
        DirectX::XMFLOAT3 horizontalDiff = diffFloat;
        horizontalDiff.y = 0;

        DirectX::XMVECTOR forward = DirectX::XMLoadFloat3(&horizontalDiff);
        float horizontalLength = DirectX::XMVectorGetX(DirectX::XMVector3Length(forward));

        DirectX::XMVECTOR right;
        if (horizontalLength < 0.1f)
        {
            right = DirectX::XMLoadFloat3(&lastCameraRight);
            if (horizontalLength < 0.01f)
            {
                forward = DirectX::XMVectorSet(0, 0, 1, 0);
            }
            else
            {
                forward = DirectX::XMVector3Normalize(forward);
            }
        }
        else
        {
            forward = DirectX::XMVector3Normalize(forward);
            DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0, 1, 0, 0);
            right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(worldUp, forward));
            DirectX::XMStoreFloat3(&lastCameraRight, right);
        }

        float heightAngle = DirectX::XMConvertToRadians(dynamicHeightAngle);
        float sideAngle = DirectX::XMConvertToRadians(lockOnSideAngle);

        if (std::isnan(heightAngle) || std::isinf(heightAngle))
        {
            heightAngle = DirectX::XMConvertToRadians(20.0f);
        }
        if (std::isnan(sideAngle) || std::isinf(sideAngle))
        {
            sideAngle = 0.0f;
        }

        DirectX::XMVECTOR cameraOffset = DirectX::XMVectorNegate(forward);
        DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0, 1, 0, 0);
        float heightRatio = tanf(heightAngle);
        DirectX::XMVECTOR heightOffset = DirectX::XMVectorScale(worldUp, cameraDistance * heightRatio);
        DirectX::XMVECTOR sideOffset = DirectX::XMVectorScale(right, sideValue * cameraDistance * tanf(sideAngle));

        DirectX::XMVECTOR vMidPoint = DirectX::XMLoadFloat3(&midPoint);
        DirectX::XMVECTOR newTargetEyePos = vMidPoint;
        float backwardDist = cameraDistance * cosf(heightAngle);
        newTargetEyePos = DirectX::XMVectorMultiplyAdd(cameraOffset,
            DirectX::XMVectorReplicate(backwardDist),
            newTargetEyePos);
        newTargetEyePos = DirectX::XMVectorAdd(newTargetEyePos, heightOffset);
        newTargetEyePos = DirectX::XMVectorAdd(newTargetEyePos, sideOffset);

        float heightAdjust = targetLimit[0] +
            (targetLimit[1] - targetLimit[0]) *
            std::clamp((distance - 5.0f) / 15.0f, 0.0f, 1.0f);
        newTargetEyePos = DirectX::XMVectorAdd(newTargetEyePos,
            DirectX::XMVectorSet(0, heightAdjust, 0, 0));

        DirectX::XMStoreFloat3(&targetCameraPosition, newTargetEyePos);

        // =============================================
        // 目標クォータニオンの計算
        // =============================================
        //{
        //    DirectX::XMVECTOR vEye = newTargetEyePos;
        //    DirectX::XMVECTOR vFocus = DirectX::XMLoadFloat3(&focusTarget);
        //    DirectX::XMVECTOR vUp = DirectX::XMVectorSet(0, 1, 0, 0);

        //    // 注視点もスムーズ化（攻撃時の揺れを抑制）
        //    DirectX::XMVECTOR vSmoothedFocus = DirectX::XMLoadFloat3(&smoothedFocusTarget);
        //    float focusSmoothT = (std::min)(focusSmoothSpeed * elapsedTime, 1.0f);
        //    vFocus = DirectX::XMVectorLerp(vSmoothedFocus, vFocus, focusSmoothT);
        //    DirectX::XMStoreFloat3(&smoothedFocusTarget, vFocus);

        //    DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(
        //        vEye,
        //        vFocus,
        //        vUp
        //    );

        //    DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixInverse(nullptr, viewMatrix);
        //    DirectX::XMVECTOR newTargetQuat = DirectX::XMQuaternionRotationMatrix(worldMatrix);
        //    DirectX::XMStoreFloat4(&targetRotation, newTargetQuat);
        //}
        {
            DirectX::XMVECTOR vEye = newTargetEyePos;
            DirectX::XMVECTOR vFocus = DirectX::XMLoadFloat3(&focusTarget);

            // --- 修正箇所 開始 ---

            // デフォルトの上方向ベクトル
            DirectX::XMVECTOR vUp = DirectX::XMVectorSet(0, 1, 0, 0);

            // 視線ベクトル（正規化）を計算
            DirectX::XMVECTOR vDir = DirectX::XMVectorSubtract(vFocus, vEye);
            vDir = DirectX::XMVector3Normalize(vDir);

            // 視線とUpベクトルの内積を計算（平行具合をチェック）
            float dot = fabsf(DirectX::XMVectorGetX(DirectX::XMVector3Dot(vDir, vUp)));

            // 平行に近い場合（ほぼ真上・真下を見ている場合）
            // 0.99f は約8度以内の垂直状態を意味します
            if (dot > 0.99f)
            {
                // UpベクトルをZ軸方向にずらして特異点を回避する
                // これにより外積計算が安定し、急激なロール回転を防ぎます
                vUp = DirectX::XMVectorSet(0, 0, 1, 0);
            }

            // --- 修正箇所 終了 ---

            // 注視点もスムーズ化（攻撃時の揺れを抑制）
            DirectX::XMVECTOR vSmoothedFocus = DirectX::XMLoadFloat3(&smoothedFocusTarget);
            float focusSmoothT = (std::min)(focusSmoothSpeed * elapsedTime, 1.0f);
            vFocus = DirectX::XMVectorLerp(vSmoothedFocus, vFocus, focusSmoothT);
            DirectX::XMStoreFloat3(&smoothedFocusTarget, vFocus);

            DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookAtLH(
                vEye,
                vFocus,
                vUp
            );

            DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixInverse(nullptr, viewMatrix);
            DirectX::XMVECTOR newTargetQuat = DirectX::XMQuaternionRotationMatrix(worldMatrix);
            DirectX::XMStoreFloat4(&targetRotation, newTargetQuat);
        }

        // =============================================
        // クォータニオンと位置の補間（テレポート対策）
        // =============================================
        DirectX::XMVECTOR currentQuat = DirectX::XMLoadFloat4(&currentRotation);
        DirectX::XMVECTOR targetQuat = DirectX::XMLoadFloat4(&targetRotation);

        // テレポート中は回転速度を制限
        float effectiveRotationSpeed = rotationLerpSpeed;
        if (teleportRecoveryTime > 0.0f)
        {
            // 回復時間に応じて徐々に速度を戻す
            float recoveryRatio = teleportRecoveryTime / teleportRecoveryDuration;
            effectiveRotationSpeed *= (1.0f - recoveryRatio * (1.0f - teleportSlowdownFactor));
        }

        float rotationT = (std::min)(effectiveRotationSpeed * elapsedTime, 1.0f);

        // 最大回転速度の制限
        {
            // 現在のクォータニオンと目標クォータニオンの角度差を計算
            float dotProduct = DirectX::XMVectorGetX(DirectX::XMQuaternionDot(currentQuat, targetQuat));
            dotProduct = std::clamp(dotProduct, -1.0f, 1.0f);
            float angleDifference = 2.0f * acosf(fabsf(dotProduct));

            // 最大回転速度に基づいて補間率を制限
            float maxRotationThisFrame = maxRotationSpeedPerFrame * elapsedTime;
            if (angleDifference > 0.001f)
            {
                float maxT = maxRotationThisFrame / angleDifference;
                rotationT = (std::min)(rotationT, maxT);
            }
        }

        DirectX::XMVECTOR newQuat = DirectX::XMQuaternionSlerp(currentQuat, targetQuat, rotationT);
        DirectX::XMStoreFloat4(&currentRotation, newQuat);

        DirectX::XMVECTOR currentPos = DirectX::XMLoadFloat3(&currentCameraPosition);
        DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&targetCameraPosition);

        // テレポート中は位置の移動速度も調整
        float effectivePositionSpeed = positionLerpSpeed;
        if (teleportRecoveryTime > 0.0f)
        {
            float recoveryRatio = teleportRecoveryTime / teleportRecoveryDuration;
            effectivePositionSpeed *= (1.0f - recoveryRatio * (1.0f - teleportSlowdownFactor));
        }

        float positionT = (std::min)(effectivePositionSpeed * elapsedTime, 1.0f);
        DirectX::XMVECTOR newPos = DirectX::XMVectorLerp(currentPos, targetPos, positionT);
        DirectX::XMStoreFloat3(&currentCameraPosition, newPos);

        // =============================================
        // カメラに設定
        // =============================================
        eye = currentCameraPosition;

        if (eye.y < minCameraHeight)
        {
            eye.y = minCameraHeight;
            currentCameraPosition.y = minCameraHeight;
        }

        if (std::isnan(eye.x) || std::isnan(eye.y) || std::isnan(eye.z))
        {
            eye.x = target.x;
            eye.y = target.y + 2.0f;
            eye.z = target.z - 5.0f;
            currentCameraPosition = eye;
        }

        camera.SetPosition(eye);
        camera.SetRotation(currentRotation);
        camera.UpdateMatrices();
    }
    else
    {
        // ロックオン解除時はスムーズ化された位置をリセット
    	smoothedPlayerPosition = target;
        smoothedFocusTarget = target;

        SetLockonPoint();
        angle.y = normalizeAngle(angle.y);

        // ロックオン解除時は初期化フラグをリセット
        isFirstLockOn = true;
        teleportRecoveryTime = 0.0f;

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

            float yaw = atan2f(vec.m128_f32[0], vec.m128_f32[2]);
            float pitch = asinf(-vec.m128_f32[1]);

            angle.y = yaw;
            angle.x = pitch;
            eye = newEye;
        }

        bothInView = false;

        Camera& camera = Camera::Instance();
        camera.SetLookAt(eye, target, DirectX::XMFLOAT3(0, 1, 0));
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
                Mathf::Lerp(newTarget.x, smoothedLockonPoint.x, lerpSpeed * elapsedTime),
                Mathf::Lerp(newTarget.y, smoothedLockonPoint.y + 0.9f, lerpSpeed * elapsedTime),
                Mathf::Lerp(newTarget.z, smoothedLockonPoint.z, lerpSpeed * elapsedTime)
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
#endif

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
    EnemyBoss& boss = EnemyBoss::Instance();

    closestEnemy = nullptr;

    closestEnemy = dynamic_cast<Enemy*>(&boss); // Use dynamic_cast to convert EnemyBoss to Enemy  

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
    Camera& camera = Camera::Instance();

    if (!camera.GetCameraShakeSwitch()) return;

    if (shakeTime < camera.GetCameraShakeTimer())
    {
        shakeTime = camera.GetCameraShakeTimer();
        camera.SetCameraShakeSwitch(true, 0, camera.GetCameraShakePower());
    }

    // シンプレックスノイズを使用
    static FastNoiseLite noiseGenerator;
    noiseGenerator.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noiseGenerator.SetFrequency(2.0f);  // 周波数（細かさ調整）

    // 時間を利用してスムーズな変化を作る
    static float timeOffset = 0.0f;
    timeOffset += elapsedTime * 5.0f; // 時間を少しずつ進める（速度調整）

    // カメラの揺れの強さ
    float shakePower = camera.GetCameraShakePower() * cameraShakeRange;

    // 各軸ごとに異なるノイズ値を取得
    DirectX::XMFLOAT3 shake;
    shake.x = noiseGenerator.GetNoise(timeOffset, 0.0f) * shakePower;
    shake.y = noiseGenerator.GetNoise(0.0f, timeOffset) * shakePower;
    shake.z = noiseGenerator.GetNoise(timeOffset, timeOffset) * shakePower;

    // 注視点に揺れ値を加える
    DirectX::XMFLOAT3 focus = camera.GetFocus();
    focus.x += shake.x;
    focus.y += shake.y;
    focus.z += shake.z;

    camera.SetLookAt(camera.GetEye(), focus, camera.GetUp());

    shakeTime -= elapsedTime;

    if (shakeTime < 0)
    {
        camera.SetCameraShakeSwitch(false, 0);
    }
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
    if (ImGui::CollapsingHeader("CameraController", ImGuiTreeNodeFlags_DefaultOpen))
    {
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

        float b = DirectX::XMConvertToDegrees(angleOffset);
        ImGui::DragFloat("angleOffset", &b);
        angleOffset = DirectX::XMConvertToRadians(b);

        ImGui::DragFloat("LeapSpeed", &lerpSpeed);
        ImGui::DragFloat("AngleLeapSpeed", &AnglelerpSpeed);
        ImGui::DragFloat("dotOffset", &dotOffset);
        ImGui::DragFloat("turnSpeed", &turnSpeed);

        if (ImGui::TreeNode("Quaternion Lock-On Camera"))
        {
            ImGui::Text("=== Rotation & Position ===");
            ImGui::DragFloat("Rotation Lerp Speed", &rotationLerpSpeed, 0.1f, 0.5f, 20.0f);
            ImGui::DragFloat("Position Lerp Speed", &positionLerpSpeed, 0.1f, 0.5f, 20.0f);

            ImGui::Separator();
            ImGui::Text("=== Viewport Check ===");
            ImGui::SliderFloat("Viewport Margin", &viewportMargin, 0.0f, 0.3f, "%.2f");
            ImGui::Text("Both in View: %s", bothInView ? "YES" : "NO");
            ImGui::TextColored(bothInView ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
                bothInView ? "Camera Rotation Locked" : "Camera Adjusting");

            ImGui::Separator();
            ImGui::Text("=== Focus Settings ===");
            ImGui::DragFloat("Switch Distance", &focusSwitchDistance, 0.1f, 5.0f, 50.0f);
            ImGui::Text("< %.1fm: Focus on Enemy", focusSwitchDistance);
            ImGui::Text(">= %.1fm: Focus on Midpoint", focusSwitchDistance);

            ImGui::Separator();
            ImGui::Text("=== Current Quaternion ===");
            ImGui::Text("X: %.3f, Y: %.3f, Z: %.3f, W: %.3f",
                currentRotation.x, currentRotation.y, currentRotation.z, currentRotation.w);

            ImGui::Separator();
            ImGui::Text("=== Safety Settings ===");
            ImGui::DragFloat("Min Camera Height", &minCameraHeight, 0.1f, 0.0f, 5.0f);

            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Lock-On Camera Settings"))
        {
            ImGui::Text("=== Basic Settings ===");
            ImGui::DragFloat("Side Angle", &lockOnSideAngle, 1.0f, -60.0f, 60.0f);

            ImGui::Separator();
            ImGui::Text("=== Distance Range ===");
            ImGui::DragFloat("Distance Min", &distanceParamMin, 0.5f, 1.0f, distanceParamMax - 1.0f);
            ImGui::DragFloat("Distance Max", &distanceParamMax, 0.5f, distanceParamMin + 1.0f, 50.0f);

            ImGui::Separator();
            ImGui::Text("=== Offset Target Y ===");
            ImGui::DragFloat("Min Offset Y (Near)", &minOffsetTargetY, 0.1f, -5.0f, maxOffsetTargetY);
            ImGui::DragFloat("Max Offset Y (Far)", &maxOffsetTargetY, 0.1f, minOffsetTargetY, 5.0f);

            ImGui::Separator();
            ImGui::Text("=== Height Angle ===");
            ImGui::DragFloat("Min Height Angle (Near)", &minHeightAngle, 0.5f, 0.0f, maxHeightAngle);
            ImGui::DragFloat("Max Height Angle (Far)", &maxHeightAngle, 0.5f, minHeightAngle, 80.0f);

            ImGui::Separator();
            ImGui::Text("=== FOV Correction ===");
            ImGui::DragFloat("Min FOV (Far)", &minFovCorrection, 0.05f, 0.5f, maxFovCorrection);
            ImGui::DragFloat("Max FOV (Near)", &maxFovCorrection, 0.05f, minFovCorrection, 5.0f);

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
    currentCameraPosition = newEye;
    currentRotation = Camera::Instance().GetRotation();

    // スムーズ化された位置を初期化
    smoothedPlayerPosition = newTarget;
    smoothedFocusTarget = newTarget;
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