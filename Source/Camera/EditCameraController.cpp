#include <imgui.h>
#include "EditCameraController.h"
#include "Camera.h"
#include "Input/Input.h"
#include "Math/Mathf.h"
#include "Math/Collision.h"
#include "Stage/StageManager.h"
#include "Graphics/Graphics.h"
#include "Camera/CameraParam.h"
#include "Character/Player/Player.h"


#include <stdlib.h>



void EditCameraController::Update(float elapsedTime)
{
    Player& player = Player::Instance();
    Model* model = player.GetModel();
    int currentIndex = model->GetCurrentAnimationIndex();

    if (currentIndex >= 0)
    {
        AnimationConfig* config = model->GetAnimationConfig("Player", currentIndex);
        float animationSeconds = model->GetCurrentAnimationSeconds();
        float secondsLength = model->GetAnimationLength(currentIndex);

        bool cameraEventActive = false;
        bool isRangeOnly = false;

        for (const auto& evt : config->events)
        {
            if (evt.eventType == EventType::Camera && evt.IsActive(animationSeconds))
            {
                cameraEventActive = true;
                if (evt.eventName == "RangeOnly")
                {
                    isRangeOnly = true;
                }
                break;
            }
        }

        if (isRangeOnly && !config->cameraKeyframes.empty())
        {
            RangeOnryEvent(config, animationSeconds, secondsLength);
        }
        else if (cameraEventActive && !config->cameraKeyframes.empty())
        {
            HandleCameraEvent(config, animationSeconds, secondsLength, player.GetAngle().y);
        }
    }

    // 通常カメラ制御
    DirectX::XMMATRIX transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
    DirectX::XMVECTOR frontVec = transform.r[2];
    DirectX::XMFLOAT3 front;
    DirectX::XMStoreFloat3(&front, frontVec);

    DirectX::XMFLOAT3 actualTarget = GetTargetPosition();

    DirectX::XMFLOAT3 eye = {
        actualTarget.x - front.x * range,
        actualTarget.y - front.y * range,
        actualTarget.z - front.z * range
    };

    Camera::Instance().SetLookAt(eye, actualTarget, DirectX::XMFLOAT3(0, 1, 0));
}

void EditCameraController::ControllUpdate(float elapsedTime)
{
    auto normalizeAngle = [](float angle) {
        return angle >= 0.f
            ? fmodf((angle)+DirectX::XM_PI, DirectX::XM_2PI) - DirectX::XM_PI
            : fmodf((angle)-DirectX::XM_PI, DirectX::XM_2PI) + DirectX::XM_PI;
        };

    GamePad& gamePad = Input::Instance().GetGamePad();
    float ax = gamePad.GetAxisRX();
    float ay = gamePad.GetAxisRY();
    float speed = rollSpeed * elapsedTime;

    if (ax) angle.y += ax * speed;
    if (ay) angle.x += (Input::Instance().GetIsLastGamePad() ? -ay : ay) * speed;

    angle.x = std::clamp(angle.x, minAngleX, maxAngleX);

    ImGuiIO io = ImGui::GetIO();

    float moveX = io.MouseDelta.x * 0.02f;
    float moveY = io.MouseDelta.y * 0.02f;
    float wheel = io.MouseWheel;

    if (io.MouseDown[ImGuiMouseButton_Left]) {
        angle.y += moveX;
        angle.x += moveY;
        angle.x = std::clamp(angle.x, minAngleX, maxAngleX);
    }
    else if (io.MouseDown[ImGuiMouseButton_Middle]) {
        angle.x = DirectX::XMConvertToRadians(30);
        angle.y = DirectX::XMConvertToRadians(120);
    }

    if (wheel != 0.0f) {
        range -= wheel * 2.0f;
        range = std::clamp(range, 3.f, 50.f);
    }

    angle.y = normalizeAngle(angle.y);
}

DirectX::XMFLOAT3 EditCameraController::GetDirection() const
{
    DirectX::XMFLOAT3 targetPos = GetTargetPosition();
    DirectX::XMFLOAT3 eyePos = GetEyePosition();

    DirectX::XMFLOAT3 dir = {
        targetPos.x - eyePos.x,
        targetPos.y - eyePos.y,
        targetPos.z - eyePos.z
    };

    // 正規化
    DirectX::XMVECTOR v = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&dir));
    DirectX::XMFLOAT3 result;
    DirectX::XMStoreFloat3(&result, v);
    return result;
}

DirectX::XMFLOAT3 EditCameraController::GetEyePosition() const
{
    DirectX::XMFLOAT3 target = GetTargetPosition();

    DirectX::XMMATRIX transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
    DirectX::XMVECTOR frontVec = transform.r[2];

    DirectX::XMFLOAT3 front;
    DirectX::XMStoreFloat3(&front, frontVec);

    DirectX::XMFLOAT3 eye;
    eye.x = target.x - front.x * range;
    eye.y = target.y - front.y * range;
    eye.z = target.z - front.z * range;

    return eye;
}

void EditCameraController::DrawDebugGui()
{
    if (ImGui::Begin("Edit Camera Controller"))
    {
        // ターゲット位置オフセット
        ImGui::DragFloat3("OffsetTarget", &offsetTarget.x, 0.1f);

        // 角度（ラジアン）表示と調整（デグリーで表示）
        DirectX::XMFLOAT3 angleDeg = {
            DirectX::XMConvertToDegrees(angle.x),
            DirectX::XMConvertToDegrees(angle.y),
            DirectX::XMConvertToDegrees(angle.z)
        };
        if (ImGui::DragFloat3("Angle (deg)", &angleDeg.x, 1.0f, -360.0f, 360.0f))
        {
            angle.x = DirectX::XMConvertToRadians(angleDeg.x);
            angle.y = DirectX::XMConvertToRadians(angleDeg.y);
            angle.z = DirectX::XMConvertToRadians(angleDeg.z);
        }

        // 回転スピード（ラジアン）を度に変換して操作
        float rollSpeedDeg = DirectX::XMConvertToDegrees(rollSpeed);
        if (ImGui::DragFloat("Roll Speed (deg/sec)", &rollSpeedDeg, 1.0f, 0.0f, 1000.0f))
        {
            rollSpeed = DirectX::XMConvertToRadians(rollSpeedDeg);
        }

        // カメラ距離（range）
        ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 100.0f);

        // X軸回転制限（ラジアン）を度で表示して編集
        float maxAngleXDeg = DirectX::XMConvertToDegrees(maxAngleX);
        float minAngleXDeg = DirectX::XMConvertToDegrees(minAngleX);
        if (ImGui::DragFloat("Max Angle X (deg)", &maxAngleXDeg, 1.0f, 0.0f, 180.0f))
        {
            maxAngleX = DirectX::XMConvertToRadians(maxAngleXDeg);
        }
        if (ImGui::DragFloat("Min Angle X (deg)", &minAngleXDeg, 1.0f, -90.0f, 180.0f))
        {
            minAngleX = DirectX::XMConvertToRadians(minAngleXDeg);
        }
    }
    ImGui::End();
}

void EditCameraController::RangeOnryEvent(AnimationConfig* config, float animationSeconds, float secondsLength)
{
    const auto& keyframes = config->cameraKeyframes;
    size_t keyCount = keyframes.size();
    if (keyCount < 2) return;

    // 最初のフレーム範囲なら、現在rangeからkey0へ補間
    if (animationSeconds < keyframes[0].time)
    {
        float t = animationSeconds / keyframes[0].time;
        t = std::clamp(t, 0.0f, 1.0f);
        float interpRange = range + (keyframes[0].range - range) * t;

        // カメラ更新処理
        DirectX::XMMATRIX transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
        DirectX::XMVECTOR frontVec = DirectX::XMVector3Normalize(transform.r[2]);

        DirectX::XMFLOAT3 front;
        DirectX::XMStoreFloat3(&front, frontVec);

        DirectX::XMFLOAT3 actualTarget = GetTargetPosition();

        DirectX::XMFLOAT3 eye = {
            actualTarget.x - front.x * interpRange,
            actualTarget.y - front.y * interpRange,
            actualTarget.z - front.z * interpRange
        };

        Camera::Instance().SetLookAt(eye, actualTarget, DirectX::XMFLOAT3(0, 1, 0));
        range = interpRange;
        return;
    }

    // 通常のキーフレーム補間処理
    size_t keyIndex = 0;
    for (size_t i = 0; i < keyCount - 1; ++i)
    {
        if (animationSeconds >= keyframes[i].time && animationSeconds < keyframes[i + 1].time)
        {
            keyIndex = i;
            break;
        }
    }

    float t = 0.0f;
    float t0 = keyframes[keyIndex].time;
    float t1 = keyframes[(keyIndex + 1) % keyCount].time;
    float timeSpan = t1 - t0;

    if (timeSpan < 0.0f)
        timeSpan += secondsLength;

    float localTime = animationSeconds - t0;
    if (localTime < 0.0f)
        localTime += secondsLength;

    t = (timeSpan > 0.0f) ? std::clamp(localTime / timeSpan, 0.0f, 1.0f) : 0.0f;

    auto idx = [&](int i) -> size_t {
        int val = static_cast<int>(keyIndex) + i;
        while (val < 0) val += static_cast<int>(keyCount);
        return val % keyCount;
        };

    float r0 = keyframes[idx(0)].range;
    float r1 = keyframes[idx(1)].range;
    float interpRange = r0 + (r1 - r0) * t;

    DirectX::XMMATRIX transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
    DirectX::XMVECTOR frontVec = DirectX::XMVector3Normalize(transform.r[2]);

    DirectX::XMFLOAT3 front;
    DirectX::XMStoreFloat3(&front, frontVec);

    DirectX::XMFLOAT3 actualTarget = GetTargetPosition();

    DirectX::XMFLOAT3 eye = {
        actualTarget.x - front.x * interpRange,
        actualTarget.y - front.y * interpRange,
        actualTarget.z - front.z * interpRange
    };

    Camera::Instance().SetLookAt(eye, actualTarget, DirectX::XMFLOAT3(0, 1, 0));
    range = interpRange;
}

void EditCameraController::HandleCameraEvent(AnimationConfig* config, float animationSeconds, float secondsLength, float currentCharacterYaw)
{
    const auto& keyframes = config->cameraKeyframes;
    size_t keyCount = keyframes.size();
    if (keyCount < 2) return; // 最低2点ないと補間できない

    // 対象キーフレームインデックス検索
    size_t keyIndex = 0;
    for (size_t i = 0; i < keyCount - 1; ++i)
    {
        if (animationSeconds >= keyframes[i].time && animationSeconds < keyframes[i + 1].time)
        {
            keyIndex = i;
            break;
        }
    }

    // 補間率 t を算出
    float t = 0.0f;
    float t0 = keyframes[keyIndex].time;
    float t1 = keyframes[(keyIndex + 1) % keyCount].time;
    float timeSpan = t1 - t0;
    if (timeSpan <= 0.0f)
        t = 0.0f;
    else
    {
        if (timeSpan < 0.0f) timeSpan += secondsLength;
        float localTime = animationSeconds - t0;
        if (localTime < 0.0f) localTime += secondsLength;
        t = std::clamp(localTime / timeSpan, 0.0f, 1.0f);
    }

    auto idx = [&](int i) -> size_t {
        int val = static_cast<int>(keyIndex) + i;
        while (val < 0) val += static_cast<int>(keyCount);
        return val % keyCount;
        };

    auto LoadXMVECTOR = [](const DirectX::XMFLOAT3& f3) {
        return DirectX::XMLoadFloat3(&f3);
        };

    DirectX::XMVECTOR interpTargetVec, interpEyeVec;

    if (keyCount >= 4)
    {
        // --- Catmull-Rom補間 ---
        DirectX::XMVECTOR p0_target = LoadXMVECTOR(keyframes[idx(-1)].targetOffset);
        DirectX::XMVECTOR p1_target = LoadXMVECTOR(keyframes[idx(0)].targetOffset);
        DirectX::XMVECTOR p2_target = LoadXMVECTOR(keyframes[idx(1)].targetOffset);
        DirectX::XMVECTOR p3_target = LoadXMVECTOR(keyframes[idx(2)].targetOffset);
        interpTargetVec = DirectX::XMVectorCatmullRom(p0_target, p1_target, p2_target, p3_target, t);

        DirectX::XMVECTOR p0_eye = LoadXMVECTOR(keyframes[idx(-1)].eyeOffset);
        DirectX::XMVECTOR p1_eye = LoadXMVECTOR(keyframes[idx(0)].eyeOffset);
        DirectX::XMVECTOR p2_eye = LoadXMVECTOR(keyframes[idx(1)].eyeOffset);
        DirectX::XMVECTOR p3_eye = LoadXMVECTOR(keyframes[idx(2)].eyeOffset);
        interpEyeVec = DirectX::XMVectorCatmullRom(p0_eye, p1_eye, p2_eye, p3_eye, t);
    }
    else
    {
        // --- 線形補間 ---
        DirectX::XMVECTOR startTarget = LoadXMVECTOR(keyframes[keyIndex].targetOffset);
        DirectX::XMVECTOR endTarget = LoadXMVECTOR(keyframes[(keyIndex + 1) % keyCount].targetOffset);
        interpTargetVec = DirectX::XMVectorLerp(startTarget, endTarget, t);

        DirectX::XMVECTOR startEye = LoadXMVECTOR(keyframes[keyIndex].eyeOffset);
        DirectX::XMVECTOR endEye = LoadXMVECTOR(keyframes[(keyIndex + 1) % keyCount].eyeOffset);
        interpEyeVec = DirectX::XMVectorLerp(startEye, endEye, t);
    }

    // --- 向き補正（Yaw） ---
    float savedYaw = keyframes[(keyIndex + 1) % keyCount].savedYaw;
    float deltaYaw = currentCharacterYaw - savedYaw;
    DirectX::XMMATRIX yawRotation = DirectX::XMMatrixRotationY(deltaYaw);
    interpTargetVec = DirectX::XMVector3TransformCoord(interpTargetVec, yawRotation);
    interpEyeVec = DirectX::XMVector3TransformCoord(interpEyeVec, yawRotation);

    // --- 補間後データ取得 ---
    DirectX::XMFLOAT3 interpTarget, interpEyeDir;
    DirectX::XMStoreFloat3(&interpTarget, interpTargetVec);
    DirectX::XMStoreFloat3(&interpEyeDir, interpEyeVec);

    // --- Range補間 ---
    float r0 = keyframes[keyIndex].range;
    float r1 = keyframes[(keyIndex + 1) % keyCount].range;
    float interpRange = r0 + (r1 - r0) * t;

    // --- カメラ最終位置計算 ---
    DirectX::XMFLOAT3 baseTarget = target;
    DirectX::XMFLOAT3 finalTarget = {
        baseTarget.x + interpTarget.x,
        baseTarget.y + interpTarget.y,
        baseTarget.z + interpTarget.z
    };

    DirectX::XMVECTOR eyeDirVec = DirectX::XMLoadFloat3(&interpEyeDir);
    eyeDirVec = DirectX::XMVector3Normalize(eyeDirVec);
    DirectX::XMFLOAT3 eyeDir;
    DirectX::XMStoreFloat3(&eyeDir, eyeDirVec);

    DirectX::XMFLOAT3 eye = {
        finalTarget.x - eyeDir.x * interpRange,
        finalTarget.y - eyeDir.y * interpRange,
        finalTarget.z - eyeDir.z * interpRange
    };

    Camera::Instance().SetLookAt(eye, finalTarget, DirectX::XMFLOAT3(0, 1, 0));

    offsetTarget = interpTarget;
    range = interpRange;
    angle.x = std::asin(-eyeDir.y);
    angle.y = std::atan2(eyeDir.x, eyeDir.z);
}




