#include "Camera.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

// クォータニオンによる回転設定
void Camera::SetRotation(const DirectX::XMFLOAT4& quaternion)
{
    rotation = quaternion;
}

// 位置設定
void Camera::SetPosition(const DirectX::XMFLOAT3& position)
{
    eye = position;
}

// クォータニオンと位置から行列を更新
void Camera::UpdateMatrices()
{
    // クォータニオンから回転行列を生成
    DirectX::XMVECTOR quat = DirectX::XMLoadFloat4(&rotation);
    DirectX::XMMATRIX rotMatrix = DirectX::XMMatrixRotationQuaternion(quat);

    // 方向ベクトルを計算
    DirectX::XMVECTOR rightVec = DirectX::XMVector3TransformNormal(
        DirectX::XMVectorSet(1, 0, 0, 0), rotMatrix);
    DirectX::XMVECTOR upVec = DirectX::XMVector3TransformNormal(
        DirectX::XMVectorSet(0, 1, 0, 0), rotMatrix);
    DirectX::XMVECTOR frontVec = DirectX::XMVector3TransformNormal(
        DirectX::XMVectorSet(0, 0, 1, 0), rotMatrix);

    DirectX::XMStoreFloat3(&right, rightVec);
    DirectX::XMStoreFloat3(&up, upVec);
    DirectX::XMStoreFloat3(&front, frontVec);

    // 注視点を計算（カメラ位置 + 前方向 * 距離）
    DirectX::XMVECTOR eyeVec = DirectX::XMLoadFloat3(&eye);
    DirectX::XMVECTOR focusVec = DirectX::XMVectorAdd(eyeVec,
        DirectX::XMVectorScale(frontVec, 10.0f));
    DirectX::XMStoreFloat3(&focus, focusVec);

    // ビュー行列を作成
    DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(
        eyeVec,
        focusVec,
        upVec
    );
    DirectX::XMStoreFloat4x4(&view, View);
}

// 従来の互換性のための関数
void Camera::SetLookAt(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up)
{
    // 視点、注視点、上方向からビュー行列を作成
    DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&eye);
    DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&focus);
    DirectX::XMVECTOR Up = DirectX::XMLoadFloat3(&up);
    DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, Focus, Up);
    DirectX::XMStoreFloat4x4(&view, View);

    // ビューを逆行列化し、ワールド行列に戻す
    DirectX::XMMATRIX World = DirectX::XMMatrixInverse(nullptr, View);
    DirectX::XMFLOAT4X4 world;
    DirectX::XMStoreFloat4x4(&world, World);

    // カメラの方向を取り出す
    this->right.x = world._11;
    this->right.y = world._12;
    this->right.z = world._13;
    this->up.x = world._21;
    this->up.y = world._22;
    this->up.z = world._23;
    this->front.x = world._31;
    this->front.y = world._32;
    this->front.z = world._33;

    // 視点、注視点を保存
    this->eye = eye;
    this->focus = focus;

    // クォータニオンに変換して保存
    DirectX::XMVECTOR quat = DirectX::XMQuaternionRotationMatrix(World);
    DirectX::XMStoreFloat4(&rotation, quat);
}

// パースペクティブ設定
void Camera::SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ)
{
    this->fov = fovY;
    this->aspect = aspect;
    this->nearZ = nearZ;
    this->farZ = farZ;

    DirectX::XMMATRIX Projection = DirectX::XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    DirectX::XMStoreFloat4x4(&projection, Projection);
}

// ワールド座標をスクリーン座標（NDC）に変換
DirectX::XMFLOAT3 Camera::WorldToNDC(const DirectX::XMFLOAT3& worldPos) const
{
    DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&worldPos);
    DirectX::XMMATRIX View = DirectX::XMLoadFloat4x4(&view);
    DirectX::XMMATRIX Proj = DirectX::XMLoadFloat4x4(&projection);
    DirectX::XMMATRIX ViewProj = DirectX::XMMatrixMultiply(View, Proj);

    DirectX::XMVECTOR screenPos = DirectX::XMVector3TransformCoord(pos, ViewProj);

    DirectX::XMFLOAT3 result;
    DirectX::XMStoreFloat3(&result, screenPos);
    return result;
}

// ワールド座標が画角内にあるかチェック
bool Camera::IsInViewport(const DirectX::XMFLOAT3& worldPos, float margin) const
{
    DirectX::XMFLOAT3 ndc = WorldToNDC(worldPos);

    // NDC座標でチェック（-1?1の範囲、マージン考慮）
    return (ndc.x >= -1.0f + margin && ndc.x <= 1.0f - margin &&
        ndc.y >= -1.0f + margin && ndc.y <= 1.0f - margin &&
        ndc.z >= 0.0f && ndc.z <= 1.0f);
}
