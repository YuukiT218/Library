#pragma once
#include <DirectXMath.h>
#include "Character/Enemy/EnemyBoss.h"

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

private:
    DirectX::XMFLOAT3 target = { 0, 0, 0 };
    DirectX::XMFLOAT3 eye = { 0, 0, 0 };
    DirectX::XMFLOAT3 lockonpoint = { 0, 0, 0 };

    Enemy* closestEnemy = nullptr;

    DirectX::XMFLOAT3 angle = { 0, 0, 0 };
    DirectX::XMFLOAT3 desiredAngle = { 0, 0, 0 };

    // カメラベース位置
    DirectX::XMFLOAT3 currentCameraPosition = { 0, 0, 0 };

    // 基本操作パラメータ
    float rollSpeed = DirectX::XMConvertToRadians(200);
    float mouseRollSpeed = DirectX::XMConvertToRadians(10);
    float range = 8.0f;
    float maxAngleX = DirectX::XMConvertToRadians(80);
    float minAngleX = DirectX::XMConvertToRadians(3);

    DirectX::XMFLOAT3 offsetTarget = { 0, 1.5, 0 };

    DirectX::XMFLOAT3 newEye{ 0, 2, -20 };
    DirectX::XMFLOAT3 newTarget = { 0, 0, 0 };

    float lerpSpeed = 5.5f;
    float AnglelerpSpeed = 5.5f;

    bool freeCameraFlag = false;
    bool islockon = false;
    bool oldLockFlag = false;

    // カメラシェイク
    float cameraShakeRange = 0.03f;
    float shakeTime = 0.f;

    // 距離に応じた自動調整パラメータ
    float lengthLimit[2] = { 5.0, 3.0f };
    float distanceParamMin = 5.0f;
    float distanceParamMax = 15.0f;
    float minOffsetTargetY = -1.0f;
    float maxOffsetTargetY = -2.0f;

    float minCameraHeight = 0.5f;

    // 注視点スムーズ化
    DirectX::XMFLOAT3 smoothedFocusTarget = { 0, 0, 0 };
    float focusSmoothSpeed = 12.0f;
    float positionLerpSpeed = 20.0f;

    // ソフトロックオン・カメラ追従用の調整パラメータ
    float autoAimYawSpeed = 1.5f;       // 左右の追従速度
    float autoAimPitchSpeed = 2.0f;     // 上下の追従速度

    // 距離に応じた見下ろし角度(度数法)
    float nearPitchDeg = -8.0f;         // 近距離時
    float farPitchDeg = 5.0f;           // 遠距離時

    float heightZoomMultiplier = 1.5f;  // 高低差によるズームアウト倍率
    float eyeBaseYMultiplier = 0.5f;    // 高低差によるカメラ位置の高さ補正倍率
    float focusYOffset = 1.5f;          // 注視点の高さオフセット

    float playerFocusWeight = 0.6f;     // プレイヤー手前時の注視点Lerp割合
    float enemyFocusWeight = 0.2f;      // 敵手前時の注視点Lerp割合

    // 空中状態ごとのカメラパラメータ
    float playerAirPitchDeg = 20.0f;       // プレイヤーのみ空中の見下ろし角
    float playerAirOffsetY = -2.0f;         // プレイヤーのみ空中のYオフセット
    float playerAirLengthOffset = 2.0f;

    float enemyAirPitchDeg = -10.0f;       // 敵のみ空中の見上げ角
    float enemyAirOffsetY = -0.75f;          // 敵のみ空中のYオフセット
    float enemyAirLengthOffset = 2.0f;

    float bothAirPitchDeg = 0.0f;         // 両方空中の見下ろし角
    float bothAirOffsetY = -1.0f;          // 両方空中のYオフセット
    float bothAirLengthOffset = 1.0f;

    float groundMaxLengthMultiplier = 1.5f; // 地上時の限界距離の倍率
    float airMaxLengthMultiplier = 2.5f;    // 空中時の限界距離の倍率

    float currentLerpedPitchDeg = 15.0f;
    float currentLerpedOffsetY = 0.0f;
    float currentLerpedLengthOffset = 0.0f;
    float currentLerpedMaxLengthMultiplier = 1.5f;
    float stateTransitionSpeed = 5.0f;  // 状態が切り替わる際の滑らかさ（数値を下げるとゆっくりになる）
};