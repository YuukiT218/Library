#pragma once
#include <DirectXMath.h>

class Camera
{
public:
    Camera() {}
    ~Camera() {}

public:
    static Camera& Instance()
    {
        static Camera camera;
        return camera;
    }

    // クォータニオンによる回転設定
    void SetRotation(const DirectX::XMFLOAT4& quaternion);

    // 位置設定
    void SetPosition(const DirectX::XMFLOAT3& position);

    // クォータニオンと位置から行列を更新
    void UpdateMatrices();

    // 従来の互換性のための関数
    void SetLookAt(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up);

    // パースペクティブ設定
    void SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ);

    // ビュー行列取得
    const DirectX::XMFLOAT4X4& GetView() const { return view; }

    // プロジェクション行列取得
    const DirectX::XMFLOAT4X4& GetProjection() const { return projection; }

    // 視点取得・設定
    void SetEye(const DirectX::XMFLOAT3 Eye) { eye = Eye; }
    const DirectX::XMFLOAT3& GetEye() const { return eye; }

    // 注視点取得・設定
    void SetFocus(const DirectX::XMFLOAT3 Focus) { focus = Focus; }
    const DirectX::XMFLOAT3& GetFocus() const { return focus; }

    // 方向ベクトル取得
    const DirectX::XMFLOAT3& GetUp() const { return up; }
    const DirectX::XMFLOAT3& GetFront() const { return front; }
    const DirectX::XMFLOAT3& GetRight() const { return right; }

    // クォータニオン取得
    const DirectX::XMFLOAT4& GetRotation() const { return rotation; }

    // 画角設定・取得
    void SetFov(float Fov) { fov = Fov; }
    const float GetFov() const { return fov; }

    // プロジェクション設定取得
    float GetAspect() const { return aspect; }
    float GetNearZ() const { return nearZ; }
    float GetFarZ() const { return farZ; }

    // ワールド座標をスクリーン座標（NDC）に変換
    DirectX::XMFLOAT3 WorldToNDC(const DirectX::XMFLOAT3& worldPos) const;

    // ワールド座標が画角内にあるかチェック
    bool IsInViewport(const DirectX::XMFLOAT3& worldPos, float margin = 0.0f) const;

    // フリーカメラフラグ
    void SetFreeCameraFlag(bool freeFlag) { freeCameraFlag = freeFlag; }
    const bool GetFreeCameraFlag() const { return freeCameraFlag; }

    // カメラシェイク
    void SetCameraShakeSwitch(bool shake, float timer = 0.5f, float power = 1.0f)
    {
        shakeflag = shake;
        shaketimer = timer;
        shakepower = power;
    }
    const bool GetCameraShakeSwitch() { return shakeflag; }
    float GetCameraShakeTimer() { return shaketimer; }
    float GetCameraShakePower() { return shakepower; }

private:
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
    DirectX::XMFLOAT3 eye;
    DirectX::XMFLOAT3 focus;
    DirectX::XMFLOAT3 up;
    DirectX::XMFLOAT3 front;
    DirectX::XMFLOAT3 right;

    // クォータニオン
    DirectX::XMFLOAT4 rotation = { 0, 0, 0, 1 };

    bool freeCameraFlag = false;
    bool shakeflag = false;

    float fov{ DirectX::XMConvertToRadians(45) };
    float shaketimer = 0;
    float shakepower = 0;
    float aspect = 1280.0f / 720.0f;
    float nearZ = 0.1f;
    float farZ = 1000.0f;
};