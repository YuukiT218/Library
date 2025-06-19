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

    oldRockFlag = isrockon;
    isrockon = CameraParam::Instance().GetIsLockOn();

    if (!isrockon)
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
        MouseCameraController(elapsedTime);
#endif
    }

#if _DEBUG
    if (GetAsyncKeyState(VK_F1) & 0x01)
    {
        isMouseLock = !isMouseLock;
    }
    if (isMouseLock)
    {
        //// マウスを画面中央に固定
        //Mouse& mouse = Input::Instance().GetMouse();
        //float bx = mouse.GetPositionX();
        //float by = mouse.GetPositionY();
        //POINT P = { width / 2, height / 2 };
        //if (fabsf(bx - P.x) > 50 || fabsf(by - P.y) > 50)
        //{
        //    mouse.SetMouseCursorPos(P.x, P.y);
        //}

        Mouse& mouse = Input::Instance().GetMouse();
        HWND hwnd = Input::Instance().GetMouse().GetHwnd();
        // CameraController::Update などの中で使用
        static POINT lastMousePos = { 0 };
        POINT currentPos;
        GetCursorPos(&currentPos);

        // マウスの差分を計算（中心からの移動量）
        RECT rect;
        GetClientRect(hwnd, &rect);
        POINT center = {
            (rect.right - rect.left) / 2,
            (rect.bottom - rect.top) / 2
        };
        ClientToScreen(hwnd, &center); // クライアント座標→スクリーン座標

        int deltaX = currentPos.x - center.x;
        int deltaY = currentPos.y - center.y;

        // カメラ角度に反映（スケーリング係数は適宜調整）
        angle.y += deltaX * 0.01f;
        angle.x += deltaY * 0.01f;

        // 中心にマウスを戻す（次フレームで差分がゼロになる）
        SetCursorPos(center.x, center.y);
    }
#else

    Mouse& mouse = Input::Instance().GetMouse();
    HWND hwnd = Input::Instance().GetMouse().GetHwnd();
    static POINT lastMousePos = { 0 };
    POINT currentPos;
    GetCursorPos(&currentPos);

    // マウスの差分を計算（中心からの移動量）
    RECT rect;
    GetClientRect(hwnd, &rect);
    POINT center = {
        (rect.right - rect.left) / 2,
        (rect.bottom - rect.top) / 2
    };
    ClientToScreen(hwnd, &center); // クライアント座標→スクリーン座標

    int deltaX = currentPos.x - center.x;
    int deltaY = currentPos.y - center.y;

    // カメラ角度に反映（スケーリング係数は適宜調整）
    angle.y += deltaX * 0.001f;
    angle.x += deltaY * 0.001f;

    // 中心にマウスを戻す（次フレームで差分がゼロになる）
    SetCursorPos(center.x, center.y);

    ShowCursor(false);
#endif

    angle.x = std::clamp(angle.x, minAngleX, maxAngleX);

    DirectX::XMMATRIX Transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);
    DirectX::XMVECTOR Front = Transform.r[2];
    DirectX::XMFLOAT3 front;
    DirectX::XMStoreFloat3(&front, Front);
    if (isrockon)
    {
        EnemyAlived();
        /*if (closestEnemy)
        {
            rockonpoint = closestEnemy->GetPosition();
        }
        else SetRockonPoint();*/

        if (oldRockFlag != isrockon)
        {
            sideValue = CalcSide(target, rockonpoint);
        }

        targetWork[0] = target;
        targetWork[1] = rockonpoint;
        targetWork[0].y += 0.01f;
        targetWork[1].y += 0.01f;

        // 後方斜に移動させる
        DirectX::XMVECTOR t0 = DirectX::XMVectorSet(targetWork[0].x, 0.5f, targetWork[0].z, 0);
        DirectX::XMVECTOR t1 = DirectX::XMVectorSet(targetWork[1].x, 0.5f + targetYoffset, targetWork[1].z, 0);
        DirectX::XMVECTOR crv = DirectX::XMLoadFloat3(&Camera::Instance().GetRight());
        DirectX::XMVECTOR cuv = DirectX::XMVectorSet(0, 1, 0, 0);

        // v: t1 - t0
        DirectX::XMVECTOR v = DirectX::XMVectorSubtract(t1, t0);
        DirectX::XMVECTOR vNorm = DirectX::XMVector3Normalize(v);

        // 生の距離を取得
        DirectX::XMVECTOR lRawVec = DirectX::XMVector3Length(v);
        float rawDistance = 0.0f;
        DirectX::XMStoreFloat(&rawDistance, lRawVec);

        // 距離を反転
        float minDist = lengthLimit[0];
        float maxDist = lengthLimit[1];
        float t = (rawDistance - minDist) / (maxDist - minDist);
        t = std::clamp(t, 0.0f, 1.0f);
        t = 1.0f - t;      // 反転
        t = t * t;         // イージング（任意）

        float reversedDistance = minDist + (maxDist - minDist) * t;
        DirectX::XMVECTOR l = DirectX::XMVectorReplicate(reversedDistance);

        // 新しい注視点（中間点）
        t0 = DirectX::XMLoadFloat3(&targetWork[0]);
        t1 = DirectX::XMLoadFloat3(&targetWork[1]);
        DirectX::XMStoreFloat3(&rockonpoint, DirectX::XMVectorMultiplyAdd(v, DirectX::XMVectorReplicate(0.5f), t0));

        // カメラ位置を算出
        t0 = DirectX::XMVectorMultiplyAdd(l, DirectX::XMVectorNegate(vNorm), t0);
        t0 = DirectX::XMVectorMultiplyAdd(crv, DirectX::XMVectorReplicate(sideValue * 3.0f), t0);
        t0 = DirectX::XMVectorMultiplyAdd(cuv, DirectX::XMVectorReplicate(3.0f), t0);
        DirectX::XMStoreFloat3(&eye, t0);

        // eye.y を距離に応じて変化させる
        float minY = targetLimit[0];
        float maxY = targetLimit[1];
        float yT = (reversedDistance - minDist) / (maxDist - minDist);
        yT = std::clamp(yT, 0.0f, 1.0f);
        yT = yT * yT;  // optional
        eye.y = minY + (maxY - minY) * yT;
    }
    else
    {
        SetRockonPoint();
        angle.y = normalizeAngle(angle.y);

        // ロックオンを解除した直後であれば、eyeの更新をスキップ
        if (!(oldRockFlag && !isrockon)) {
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

    /*if (isrockon)
    {
        if (closestEnemy)
        {
            newTarget = {
                Mathf::Lerp(newTarget.x, rockonpoint.x, lerpSpeed * elapsedTime),
                Mathf::Lerp(newTarget.y, rockonpoint.y + 0.9f, lerpSpeed * elapsedTime),
                Mathf::Lerp(newTarget.z, rockonpoint.z, lerpSpeed * elapsedTime)
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
    }*/

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

void CameraController::SetRockonPoint()
{
    //EnemyManager& enemyManager = EnemyManager::Instance();
    //int enemyCount = enemyManager.GetEnemyCount();

    //if (enemyCount == 0)
    //{
    //    CameraParam::Instance().SetIsLockOn(false);
    //    return;
    //}

    //float bestScore = -FLT_MAX; // 最良スコア
    //closestEnemy = nullptr;

    //// スコアの重み
    //const float weightDistance = 0.98f; // 距離重視
    //const float weightCenter = 0.02f;   // 中心重視

    //for (int i = 0; i < enemyCount; ++i)
    //{
    //    Enemy* enemy = enemyManager.GetEnemy(i);
    //    DirectX::XMFLOAT3 enemyPosition = enemy->GetPosition();

    //    //// 距離を計算する
    //    //DirectX::XMVECTOR currentTargetVec = DirectX::XMLoadFloat3(&newTarget);
    //    //DirectX::XMVECTOR enemyPositionVec = DirectX::XMLoadFloat3(&enemyPosition);

    //    //DirectX::XMVECTOR distanceVec = DirectX::XMVectorSubtract(currentTargetVec, enemyPositionVec);
    //    //float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(distanceVec));

    //    //// 距離が30より大きい場合はスキップ
    //    ////if (distance > 30.0f) continue;

    //    //// ビューとプロジェクション行列を取得
    //    //const DirectX::XMFLOAT4X4& viewMatrix = Camera::Instance().GetView();
    //    //const DirectX::XMFLOAT4X4& projectionMatrix = Camera::Instance().GetProjection();

    //    //DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&viewMatrix);
    //    //DirectX::XMMATRIX projection = DirectX::XMLoadFloat4x4(&projectionMatrix);
    //    //DirectX::XMMATRIX viewProjection = DirectX::XMMatrixMultiply(view, projection);

    //    //// NDC座標に変換
    //    //DirectX::XMVECTOR targetNDCPositionVec = DirectX::XMVector3TransformCoord(enemyPositionVec, viewProjection);
    //    //DirectX::XMFLOAT3 targetNdcPosition;
    //    //DirectX::XMStoreFloat3(&targetNdcPosition, targetNDCPositionVec);

    //    //// カメラの方向ベクトル
    //    //DirectX::XMFLOAT3 cameraForward = Camera::Instance().GetFront();
    //    //DirectX::XMVECTOR cameraPositionVec = DirectX::XMLoadFloat3(&Camera::Instance().GetEye());
    //    //DirectX::XMVECTOR cameraForwardVec = DirectX::XMLoadFloat3(&cameraForward);

    //    //DirectX::XMVECTOR directionToEnemyVec = DirectX::XMVectorSubtract(enemyPositionVec, cameraPositionVec);
    //    //DirectX::XMVECTOR directionToEnemyNorm = DirectX::XMVector3Normalize(directionToEnemyVec);
    //    //float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(cameraForwardVec, directionToEnemyNorm));

    //    //// 視野角内にあるか確認
    //    //float fieldOfViewCos = cosf(DirectX::XMConvertToRadians(60 / 2.0f));
    //    //if (dot < fieldOfViewCos) continue;

    //    //// 距離スコアを計算
    //    //float distanceScore = 1.0f / (distance + 1.0f);

    //    //// 中心スコアを計算
    //    //float centerScore = 1.0f / (fabs(targetNdcPosition.x) + 0.1f); // 0.1を加えてゼロ除算回避

    //    //// 合成スコアを計算
    //    //float score = weightDistance * distanceScore + weightCenter * centerScore;

    //    //// 最良スコアを更新
    //    //if (score > bestScore)
    //    //{
    //    //    bestScore = score;
    //    closestEnemy = enemy;
    //    CameraParam::Instance().SetRockOnEnemy(closestEnemy);
    //    //}
    //}

    //if (!closestEnemy)
    //{
    //    CameraParam::Instance().SetIsLockOn(false);
    //}
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

        ImGui::DragFloat("TargetY Offset", &targetYoffset, 0.1f);

        ImGui::DragFloat("lengthLimit 1", &lengthLimit[0], 0.1f, -10.0f, lengthLimit[1]);
        ImGui::DragFloat("lengthLimit 2", &lengthLimit[1], 0.1f, lengthLimit[0], 500.0f);

        ImGui::DragFloat("TargetLimit 1", &targetLimit[0], 0.1f);
        ImGui::DragFloat("TargetLimit 2", &targetLimit[1], 0.1f);

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
    v.x = target.x - rockonpoint.x;
    v.y = target.z - rockonpoint.z;
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