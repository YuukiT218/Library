#pragma once
#include <DirectXMath.h>
#include "Character/Enemy/EnemyBoss.h"
#include <deque>

#define CameraLerp (1)

class CameraController
{
public:
    CameraController() {}
    ~CameraController() {}

    // 更新処理
    void Update(float elapsedTime);

    // ターゲット位置設定
    void SetTarget(const DirectX::XMFLOAT3& target)
    {
        this->target = {
            target.x + offsetTarget.x,
            target.y + offsetTarget.y,
            target.z + offsetTarget.z
        };
    }

    // ロックオン位置設定
    void SetLockonPoint();

    // エネミーの生死判定
    void EnemyAlived();

    // カメラシェイク
    void CameraShake(float elapsedTime);

    // マウスカメラ操作
    void MouseCameraController(float elapsedTime);

    // デバッグ用GUI描画
    void DrawDebugGUI();

    // デバッグプリミティブ描画
    void DrawDebugPrimitive();

    // コントローラー同士の引継ぎ用
    void InitCamera();

    DirectX::XMFLOAT3 GetAngle() const { return angle; }

    // 敵の動き履歴を記録する構造体
    struct MovementHistory
    {
        DirectX::XMFLOAT3 position;
        float timestamp;
    };

private:
    // 外積を用いて横軸のズレ方向算出
    float CalcSide(DirectX::XMFLOAT3 p1, DirectX::XMFLOAT3 p2);

private:
    DirectX::XMFLOAT3 target = { 0, 0, 0 };
    DirectX::XMFLOAT3 eye = { 0, 0, 0 };
    DirectX::XMFLOAT3 lockonpoint = { 0, 0, 0 };

    Enemy* closestEnemy = nullptr;

    DirectX::XMFLOAT3 angle = { 0, 0, 0 };
    DirectX::XMFLOAT3 desiredAngle = { 0, 0, 0 };

    // クォータニオンベースのカメラ制御
    DirectX::XMFLOAT4 currentRotation = { 0, 0, 0, 1 };
    DirectX::XMFLOAT4 targetRotation = { 0, 0, 0, 1 };
    DirectX::XMFLOAT3 currentCameraPosition = { 0, 0, 0 };
    DirectX::XMFLOAT3 targetCameraPosition = { 0, 0, 0 };

    float defaultRotationLerpSpeed = 15.0f;
    float rotationLerpSpeed = 15.0f;
    float defaultPositionLerpSpeed = 20.0f;
    float positionLerpSpeed = 20.0f;

    // 画角チェック設定
    float viewportMargin = 0.1f;
    bool bothInView = false;

    float rollSpeed = DirectX::XMConvertToRadians(200);
    float mouseRollSpeed = DirectX::XMConvertToRadians(10);
    float range = 8.0f;
    float maxAngleX = DirectX::XMConvertToRadians(80);
    float minAngleX = DirectX::XMConvertToRadians(3);

    DirectX::XMFLOAT3 offsetTarget = { 0, 1.5, 0 };

    DirectX::XMFLOAT3 newEye{ 0, 2, -20 };
    DirectX::XMFLOAT3 newTarget = { 0, 0, 0 };

    DirectX::XMFLOAT3 targetWork[2] = { { 0, 0, 0 }, { 0, 0, 0 } };
    float targetYoffset = 0;
    float lengthLimit[2] = { 5.0, 22.5f };
    float targetLimit[2] = { 0, 3.0f };
    float sideValue = 1;

    float lerpSpeed = 5.5f;
    float AnglelerpSpeed = 5.5f;
    float disAngleY;

    bool freeCameraFlag = false;
    bool islockon = false;
    bool oldLockFlag = false;

    float angleOffset = DirectX::XMConvertToRadians(20);
    float adjustedYaw = 0;
    float turnSpeed = 5.f;
    float dotOffset = -0.5f;

    bool turnSwitch = false;

    float cameraShakeRange = 0.03f;
    float shakeTime = 0.f;

    bool isMouseLock = false;

    float minTargetDistance = 7.0f;
    float maxTargetDistance = 22.5f;

    float distanceEasingPower = 1.0f;
    float lockOnHeightAngle = 10.0f;
    float lockOnSideAngle = 0.0f;
    float lockOnFovCorrection = 2.0f;
    DirectX::XMFLOAT3 lastCameraRight = { 1, 0, 0 };

    // 距離に応じた自動調整パラメータ
    float distanceParamMin = 5.0f;
    float distanceParamMax = 15.0f;

    float minOffsetTargetY = -1.0f;
    float maxOffsetTargetY = -2.0f;

    float minHeightAngle = 20.0f;
    float maxHeightAngle = 0.0f;

    float minFovCorrection = 1.0f;
    float maxFovCorrection = 1.5f;

    // 注視点切り替え距離
    float focusSwitchDistance = 10.0f;
    float minCameraHeight = 0.5f;

    // ===== テレポート対策用の変数 =====
    DirectX::XMFLOAT3 previousEnemyPosition = { 0, 0, 0 };
    bool isFirstLockOn = true;
    float teleportDetectionThreshold = 8.0f;
    float teleportRecoveryTime = 0.0f;
    float teleportRecoveryDuration = 0.5f;
    float teleportSlowdownFactor = 0.15f;
    float maxRotationSpeedPerFrame = DirectX::XMConvertToRadians(720.0f);

    DirectX::XMFLOAT3 smoothedLockonPoint = { 0, 0, 0 };
    float lockonSmoothSpeed = 8.0f;

    // ===== テレポート時のカメラ調整用変数 =====
    bool isTeleportAdjusting = false;           // テレポート調整中フラグ
    float teleportAdjustTimer = 0.0f;           // 調整タイマー
    float teleportAdjustDuration = 1.0f;        // 調整時間(秒)
    float teleportDistanceIncrease = 0.5f;      // カメラ距離増加率(0.5 = 50%増)
    float teleportFovIncrease = 0.3f;           // FOV増加率

    // ===== プレイヤー位置スムーズ化（攻撃時の横揺れ対策）=====
    DirectX::XMFLOAT3 smoothedPlayerPosition = { 0, 0, 0 };
    DirectX::XMFLOAT3 smoothedFocusTarget = { 0, 0, 0 };
    float playerPositionSmoothSpeed = 15.0f;    // プレイヤー位置の補間速度
    float focusSmoothSpeed = 12.0f;             // 注視点の補間速度
};