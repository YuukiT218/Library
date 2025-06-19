#pragma once

#include <DirectXMath.h>

// カメラ
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

	// 指定方向を向く
	void SetLookAt(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up);

	// パースペクティブ設定
	void SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ);

	// ビュー行列取得
	const DirectX::XMFLOAT4X4& GetView() const { return view; }

	// プロジェクション行列取得
	const DirectX::XMFLOAT4X4& GetProjection() const { return projection; }

	// 視点取得
	void SetEye(const DirectX::XMFLOAT3 Eye) { eye = Eye; }
	const DirectX::XMFLOAT3& GetEye() const { return eye; }

	// 注視点取得
	void SetFocus(const DirectX::XMFLOAT3 Focus) { focus = Focus; }
	const DirectX::XMFLOAT3& GetFocus() const { return focus; }

	// 上方向取得
	const DirectX::XMFLOAT3& GetUp() const { return up; }

	// 前方向取得
	const DirectX::XMFLOAT3& GetFront() const { return front; }

	// 右方向取得
	const DirectX::XMFLOAT3& GetRight() const { return right; }

	//画角設定
	void SetFov(float Fov) { fov = Fov; }

	//画角取得
	const float GetFov() const { return fov; }

	// フリーカメラフラグ設定
	void SetFreeCameraFlag(bool freeFlag) { freeCameraFlag = freeFlag; }

	// フリーカメラフラグ取得
	const bool GetFreeCameraFlag() const { return freeCameraFlag; }

private:
	DirectX::XMFLOAT4X4		view;
	DirectX::XMFLOAT4X4		projection;

	DirectX::XMFLOAT3		eye;
	DirectX::XMFLOAT3		focus;

	DirectX::XMFLOAT3		up;
	DirectX::XMFLOAT3		front;
	DirectX::XMFLOAT3		right;

	bool freeCameraFlag = false;

	//画角
	float fov{ DirectX::XMConvertToRadians(45) };
};
