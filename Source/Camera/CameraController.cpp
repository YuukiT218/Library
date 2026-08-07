#include <imgui.h>
#include "CameraController.h"
#include "Camera.h"
#include "Input/Input.h"
#include "Math/Mathf.h"
#include "Math/Collision.h"
#include "Graphics/Graphics.h"
#include "Camera/CameraParam.h"
#include "Math/FastNoiseLite.h"
#include "Character/Player/Player.h"


#include <stdlib.h>



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

#if !_DEBUG
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
        // 最新の敵位置を取得
        if (closestEnemy->IsTeleporting())
        {
            lockonpoint = closestEnemy->GetCameraTrackingPosition();
        }
        else
        {
            lockonpoint = closestEnemy->GetPosition();
        }

        // プレイヤーと敵の位置ベクトル
        DirectX::XMVECTOR vPlayer = DirectX::XMLoadFloat3(&target);
        DirectX::XMVECTOR vEnemy = DirectX::XMLoadFloat3(&lockonpoint);
        DirectX::XMVECTOR vDiff = DirectX::XMVectorSubtract(vEnemy, vPlayer);
        float distToEnemy = DirectX::XMVectorGetX(DirectX::XMVector3Length(vDiff));

        // 高低差と平均の高さ
        float deltaY = lockonpoint.y - target.y; // +なら敵が高い、-ならプレイヤーが高い
        float diffY = fabsf(deltaY);
        float midY = (target.y + lockonpoint.y) * 0.5f;

        float t = (distToEnemy - distanceParamMin) / (distanceParamMax - distanceParamMin);
        t = std::clamp(t, 0.0f, 1.0f);

        // 接地判定の取得
        bool isPlayerGround = Player::Instance().IsGround();
        bool isEnemyGround = closestEnemy->GetDistanceFromGround() <= 0.5f;

        // カメラ操作の自動補正(Yaw: 左右の角度)
        float targetYaw = atan2f(DirectX::XMVectorGetX(vDiff), DirectX::XMVectorGetZ(vDiff));

        float diffYaw = targetYaw - angle.y;
        while (diffYaw < -DirectX::XM_PI) diffYaw += DirectX::XM_2PI;
        while (diffYaw > DirectX::XM_PI) diffYaw -= DirectX::XM_2PI;

        angle.y += diffYaw * autoAimYawSpeed * elapsedTime;

        // 距離に応じて基本となるピッチ角（見下ろし角）を線形補間
        float targetBasePitchDeg = 0.0f;
        float targetOffsetY = 0.0f;
        float targetLengthOffset = 0.0f;
        float targetMaxLengthMultiplier = groundMaxLengthMultiplier;

        if (!isPlayerGround && isEnemyGround)
        {
            // ① プレイヤーのみ空中
            targetBasePitchDeg = playerAirPitchDeg;
            targetOffsetY = playerAirOffsetY;
            targetLengthOffset = playerAirLengthOffset;
            targetMaxLengthMultiplier = airMaxLengthMultiplier;
        }
        else if (isPlayerGround && !isEnemyGround)
        {
            // ② 敵のみ空中
            targetBasePitchDeg = enemyAirPitchDeg;
            targetOffsetY = enemyAirOffsetY;
            targetLengthOffset = enemyAirLengthOffset;
            targetMaxLengthMultiplier = airMaxLengthMultiplier;
        }
        else if (!isPlayerGround && !isEnemyGround)
        {
            // ③ 両方空中
            targetBasePitchDeg = bothAirPitchDeg;
            targetOffsetY = bothAirOffsetY;
            targetLengthOffset = bothAirLengthOffset;
            targetMaxLengthMultiplier = airMaxLengthMultiplier;
        }
        else
        {
            // ④ 両方地上
            targetBasePitchDeg = nearPitchDeg + (farPitchDeg - nearPitchDeg) * t;
            targetOffsetY = minOffsetTargetY + (maxOffsetTargetY - minOffsetTargetY) * t;
            targetLengthOffset = 0.0f;
            targetMaxLengthMultiplier = groundMaxLengthMultiplier;
        }

        // 目標値に向かって滑らかに数値を変化させる（ガクッと切り替わるのを防ぐ）
        currentLerpedPitchDeg += (targetBasePitchDeg - currentLerpedPitchDeg) * stateTransitionSpeed * elapsedTime;
        currentLerpedOffsetY += (targetOffsetY - currentLerpedOffsetY) * stateTransitionSpeed * elapsedTime;
        currentLerpedLengthOffset += (targetLengthOffset - currentLerpedLengthOffset) * stateTransitionSpeed * elapsedTime;
        currentLerpedMaxLengthMultiplier += (targetMaxLengthMultiplier - currentLerpedMaxLengthMultiplier) * stateTransitionSpeed * elapsedTime;

        // 滑らかになった数値をカメラの計算に適用する
        float basePitch = DirectX::XMConvertToRadians(currentLerpedPitchDeg);
        float currentOffsetY = currentLerpedOffsetY;

        // さらに高低差から、見上げる/見下ろすための補正角度を算出
        float pitchCorrection = atan2f(-deltaY, distToEnemy);
        float targetPitch = basePitch + pitchCorrection;

        // ピッチ角が極端になりすぎないよう制限（-25度～60度の範囲）
        targetPitch = std::clamp(targetPitch, DirectX::XMConvertToRadians(-25.0f), DirectX::XMConvertToRadians(60.0f));

        angle.x += (targetPitch - angle.x) * autoAimPitchSpeed * elapsedTime;

        // 距離と高低差に応じたレングスの補間
        float currentCameraLength = lengthLimit[0] + (lengthLimit[1] - lengthLimit[0]) * t;
        currentCameraLength += currentLerpedLengthOffset;

        // 高低差によるズームアウト加算
        float heightZoomBonus = diffY * heightZoomMultiplier;
        currentCameraLength += heightZoomBonus;
        currentCameraLength = (std::min)(currentCameraLength, lengthLimit[1] * currentLerpedMaxLengthMultiplier);

        // カメラ位置(Eye)の計算
        DirectX::XMMATRIX Transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
        DirectX::XMVECTOR vCamForward = Transform.r[2];

        float eyeBaseY = midY + currentOffsetY + (diffY * eyeBaseYMultiplier);

        DirectX::XMVECTOR vEyeTarget = DirectX::XMVectorSet(target.x, eyeBaseY, target.z, 0);
        vEyeTarget = DirectX::XMVectorSubtract(vEyeTarget, DirectX::XMVectorScale(vCamForward, currentCameraLength));

        // 手前・奥の判別と注視点(Focus)の計算
        float distEyeToPlayer = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(vPlayer, vEyeTarget)));
        float distEyeToEnemy = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(vEnemy, vEyeTarget)));

        DirectX::XMVECTOR vFocusTarget;
        if (distEyeToPlayer <= distEyeToEnemy)
        {
            vFocusTarget = DirectX::XMVectorLerp(vPlayer, vEnemy, playerFocusWeight);
        }
        else
        {
            vFocusTarget = DirectX::XMVectorLerp(vEnemy, vPlayer, enemyFocusWeight);
        }

        // 注視点の高さを2人の平均(midY)に合わせる
        float focusY = midY + currentOffsetY + focusYOffset;
        vFocusTarget = DirectX::XMVectorSetY(vFocusTarget, focusY);

        // スムーズな補間処理
        DirectX::XMVECTOR vCurrentEye = DirectX::XMLoadFloat3(&currentCameraPosition);
        DirectX::XMVECTOR vCurrentFocus = DirectX::XMLoadFloat3(&smoothedFocusTarget);

        float eyeLerpT = (std::min)(positionLerpSpeed * elapsedTime, 1.0f);
        float focusLerpT = (std::min)(focusSmoothSpeed * elapsedTime, 1.0f);

        vCurrentEye = DirectX::XMVectorLerp(vCurrentEye, vEyeTarget, eyeLerpT);
        vCurrentFocus = DirectX::XMVectorLerp(vCurrentFocus, vFocusTarget, focusLerpT);

        DirectX::XMStoreFloat3(&currentCameraPosition, vCurrentEye);
        DirectX::XMStoreFloat3(&smoothedFocusTarget, vCurrentFocus);

        // カメラへの適用
        eye = currentCameraPosition;

        // 地面へのめり込み防止
        if (eye.y < minCameraHeight)
        {
            eye.y = minCameraHeight;
            currentCameraPosition.y = minCameraHeight;
        }

        Camera& camera = Camera::Instance();
        camera.SetLookAt(eye, smoothedFocusTarget, DirectX::XMFLOAT3(0, 1, 0));
    }
    else
    {
        // 常に索敵を行い、次回ロックオン可能な敵を探しておく
        SetLockonPoint();

        angle.y = normalizeAngle(angle.y);

        // ロックオンを解除した瞬間の処理（カメラのワープ防止）
        if (oldLockFlag && !islockon)
        {
            // 現在のカメラ位置からプレイヤー(target)へのベクトルを計算
            DirectX::XMVECTOR vTarget = DirectX::XMLoadFloat3(&target);
            DirectX::XMVECTOR vEye = DirectX::XMLoadFloat3(&currentCameraPosition);
            DirectX::XMVECTOR vDir = DirectX::XMVectorSubtract(vTarget, vEye);
            vDir = DirectX::XMVector3Normalize(vDir);

            // カメラのYawとPitchを現在の向きに合わせて上書きし、カメラが急に背面へ飛ぶのを防ぐ
            float yaw = atan2f(DirectX::XMVectorGetX(vDir), DirectX::XMVectorGetZ(vDir));
            float pitch = asinf(-DirectX::XMVectorGetY(vDir));

            angle.y = yaw;
            angle.x = pitch;

            // 位置はロックオン解除時の場所をそのまま引き継ぐ
            eye = currentCameraPosition;
        }
        else
        {
            // 通常時のカメラ位置計算（targetからfront方向へrange分下がる）
            eye = {
                target.x - front.x * range,
                target.y - front.y * range,
                target.z - front.z * range
            };

            // 現在位置の記録を更新
            currentCameraPosition = eye;
        }

        // 次回ロックオンしたときにカメラが荒ぶらないよう、注視点スムーズ変数をリセット
        smoothedFocusTarget = target;

        // カメラに設定を適用
        Camera& camera = Camera::Instance();
        camera.SetLookAt(eye, target, DirectX::XMFLOAT3(0, 1, 0));
    }

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

    // 移動量（delta）の計算
    float dx = 0.0f;
    float dy = 0.0f;

#ifdef _DEBUG
    // 【Debugモード】
    // カーソルは固定されないため、Inputクラスが記録した「前回位置」との差分を使用
    float oldbx = mouse.GetOldPositionX();
    float oldby = mouse.GetOldPositionY();
    dx = bx - oldbx;
    dy = by - oldby;
#else
    // 【Releaseモード】
    // Framework側で毎フレーム中央にリセットされるため、「ウィンドウ中央」からの差分を計算
    HWND hWnd = GetActiveWindow();
    if (hWnd)
    {
        RECT rect;
        GetClientRect(hWnd, &rect);
        float centerX = (rect.right - rect.left) * 0.5f;
        float centerY = (rect.bottom - rect.top) * 0.5f;

        // Inputはクライアント座標を返すと仮定して計算
        dx = bx - centerX;
        dy = by - centerY;
    }
#endif

    float mouseSpeed = mouseRollSpeed * elapsedTime;

    // ベクトルの長さが0でない場合のみ計算（ゼロ除算防止）
    if (dx != 0.0f || dy != 0.0f)
    {
        DirectX::XMFLOAT2 direction;
        direction.x = dx;
        direction.y = dy;

        DirectX::XMVECTOR dir = DirectX::XMLoadFloat2(&direction);

        DirectX::XMStoreFloat2(&direction, dir);

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

        if (ImGui::TreeNode("Soft Lock-On Params"))
        {
            ImGui::Text("=== Auto Aim (Speed & Angle) ===");
            ImGui::DragFloat("Yaw Auto Aim Speed", &autoAimYawSpeed, 0.1f, 0.0f, 10.0f);
            ImGui::DragFloat("Pitch Auto Aim Speed", &autoAimPitchSpeed, 0.1f, 0.0f, 10.0f);
            ImGui::DragFloat("Near Pitch (Deg)", &nearPitchDeg, 0.5f, -45.0f, 60.0f);
            ImGui::DragFloat("Far Pitch (Deg)", &farPitchDeg, 0.5f, -45.0f, 60.0f);

            ImGui::Separator();
            ImGui::Text("=== Distance & Offset Range ===");
            ImGui::DragFloat("Distance Min", &distanceParamMin, 0.5f, 1.0f, distanceParamMax - 1.0f);
            ImGui::DragFloat("Distance Max", &distanceParamMax, 0.5f, distanceParamMin + 1.0f, 50.0f);
            ImGui::DragFloat("Min Offset Y (Near)", &minOffsetTargetY, 0.1f, -5.0f, maxOffsetTargetY);
            ImGui::DragFloat("Max Offset Y (Far)", &maxOffsetTargetY, 0.1f, minOffsetTargetY, 5.0f);
            ImGui::DragFloat("lengthLimit Min", &lengthLimit[0], 0.1f, -10.0f, lengthLimit[1]);
            ImGui::DragFloat("lengthLimit Max", &lengthLimit[1], 0.1f, lengthLimit[0], 500.0f);

            ImGui::Separator();
            ImGui::Text("=== Height & Distance Tuning ===");
            ImGui::DragFloat("Height Zoom Multiplier", &heightZoomMultiplier, 0.1f, 0.0f, 5.0f);
            ImGui::DragFloat("Eye Base Y Multiplier", &eyeBaseYMultiplier, 0.1f, 0.0f, 2.0f);
            ImGui::DragFloat("Focus Y Offset", &focusYOffset, 0.1f, -5.0f, 5.0f);

            ImGui::Separator();
            ImGui::Text("=== Air State Params ===");
            ImGui::DragFloat("State Transition Speed", &stateTransitionSpeed, 0.1f, 0.1f, 20.0f);
            ImGui::DragFloat("Ground Max Length Multiplier", &groundMaxLengthMultiplier, 0.1f, 1.0f, 5.0f);
            ImGui::DragFloat("Air Max Length Multiplier", &airMaxLengthMultiplier, 0.1f, 1.0f, 5.0f);

        	ImGui::DragFloat("Player Air Pitch", &playerAirPitchDeg, 0.5f, -45.0f, 60.0f);
            ImGui::DragFloat("Player Air Offset Y", &playerAirOffsetY, 0.1f, -5.0f, 5.0f);
            ImGui::DragFloat("Player Air Length Offset", &playerAirLengthOffset, 0.1f, -10.0f, 10.0f); // ★追加

            ImGui::DragFloat("Enemy Air Pitch", &enemyAirPitchDeg, 0.5f, -45.0f, 60.0f);
            ImGui::DragFloat("Enemy Air Offset Y", &enemyAirOffsetY, 0.1f, -5.0f, 5.0f);
            ImGui::DragFloat("Enemy Air Length Offset", &enemyAirLengthOffset, 0.1f, -10.0f, 10.0f); // ★追加

            ImGui::DragFloat("Both Air Pitch", &bothAirPitchDeg, 0.5f, -45.0f, 60.0f);
            ImGui::DragFloat("Both Air Offset Y", &bothAirOffsetY, 0.1f, -5.0f, 5.0f);
            ImGui::DragFloat("Both Air Length Offset", &bothAirLengthOffset, 0.1f, -10.0f, 10.0f); // ★追加

            ImGui::Separator();
            ImGui::Text("=== Focus Lerp Weights ===");
            ImGui::SliderFloat("Player Front Weight", &playerFocusWeight, 0.0f, 1.0f);
            ImGui::SliderFloat("Enemy Front Weight", &enemyFocusWeight, 0.0f, 1.0f);

            ImGui::Separator();
            ImGui::Text("=== Interpolation Speeds ===");
            ImGui::DragFloat("Position Lerp Speed", &positionLerpSpeed, 0.1f, 0.1f, 50.0f);
            ImGui::DragFloat("Focus Lerp Speed", &focusSmoothSpeed, 0.1f, 0.1f, 50.0f);

            ImGui::Separator();
            ImGui::Text("=== Safety Settings ===");
            ImGui::DragFloat("Min Camera Height", &minCameraHeight, 0.1f, 0.0f, 5.0f);

            ImGui::TreePop();
        }

        ImGui::Checkbox("FreeCamera", &freeCameraFlag);
    }
}

void CameraController::DrawDebugPrimitive()
{

}

void CameraController::InitCamera()
{
    newEye = Camera::Instance().GetEye();
    newTarget = Camera::Instance().GetFocus();
    currentCameraPosition = newEye;

    // スムーズ化された注視点を初期化
    smoothedFocusTarget = newTarget;
}
