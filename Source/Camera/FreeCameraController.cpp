#include <imgui.h>
#include "FreeCameraController.h"
#include <stdlib.h>

namespace
{
	// マウスの移動量をカメラ操作量に変換する係数
	constexpr float MOUSE_MOVE_SCALE = 0.02f;

	// 回転速度の係数
	constexpr float ROTATE_SPEED_SCALE = 0.5f;

	// 平行移動の速さ（注視点までの距離に対する割合）
	constexpr float PAN_SPEED_SCALE = 0.035f;

	// ズームの速さ（注視点までの距離に対する割合）
	constexpr float ZOOM_SPEED_SCALE = 0.1f;
}

// カメラからコントローラーへパラメータを同期する
void FreeCameraController::SyncCameraToController(const Camera& camera)
{
	eye = camera.GetEye();
	focus = camera.GetFocus();
	up = camera.GetUp();
	right = camera.GetRight();

	// 視点から注視点までの距離を算出
	DirectX::XMVECTOR Eye = DirectX::XMLoadFloat3(&eye);
	DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&focus);
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Focus, Eye);
	DirectX::XMVECTOR Distance = DirectX::XMVector3Length(Vec);
	DirectX::XMStoreFloat(&distance, Distance);

	// 回転角度を算出
	const DirectX::XMFLOAT3& front = camera.GetFront();
	angleX = ::asinf(-front.y);
	if (up.y < 0)
	{
		if (front.y > 0)
		{
			angleX = -DirectX::XM_PI - angleX;
		}
		else
		{
			angleX = DirectX::XM_PI - angleX;
		}
		angleY = ::atan2f(front.x, front.z);
	}
	else
	{
		angleY = ::atan2f(-front.x, -front.z);
	}

}

// コントローラーからカメラへパラメータを同期する
void FreeCameraController::SyncControllerToCamera(Camera& camera)
{
	camera.SetLookAt(eye, focus, up);
}

// 更新処理
void FreeCameraController::Update()
{
	// デバッグウインドウ操作中は処理しない
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow))
	{
		return;
	}

	// IMGUIのマウス入力値を使ってカメラ操作する
	ImGuiIO io = ImGui::GetIO();

	// マウスカーソルの移動量を求める
	float moveX = io.MouseDelta.x * MOUSE_MOVE_SCALE;
	float moveY = io.MouseDelta.y * MOUSE_MOVE_SCALE;

	// マウス左ボタン押下中
	if (io.MouseDown[ImGuiMouseButton_Right])
	{
		// Y軸回転
		angleY += moveX * ROTATE_SPEED_SCALE;
		if (angleY > DirectX::XM_PI)
		{
			angleY -= DirectX::XM_2PI;
		}
		else if (angleY < -DirectX::XM_PI)
		{
			angleY += DirectX::XM_2PI;
		}
		// X軸回転
		angleX += moveY * ROTATE_SPEED_SCALE;
		if (angleX > DirectX::XM_PI)
		{
			angleX -= DirectX::XM_2PI;
		}
		else if (angleX < -DirectX::XM_PI)
		{
			angleX += DirectX::XM_2PI;
		}
	}
	// マウス中ボタン押下中
	else if (io.MouseDown[ImGuiMouseButton_Middle])
	{
		// 平行移動
		float s = distance * PAN_SPEED_SCALE;
		float x = moveX * s;
		float y = moveY * s;

		focus.x -= right.x * x;
		focus.y -= right.y * x;
		focus.z -= right.z * x;

		focus.x += up.x * y;
		focus.y += up.y * y;
		focus.z += up.z * y;
	}
	// マウス右ボタン押下中
	else if (io.MouseDown[ImGuiMouseButton_Left] && io.MouseDown[ImGuiMouseButton_Right])
	{
		// ズーム
		distance += (-moveY - moveX) * distance * ZOOM_SPEED_SCALE;
	}
	// マウスホイール
	else if (io.MouseWheel != 0.0f)
	{
		// ズーム
		distance -= io.MouseWheel * distance * ZOOM_SPEED_SCALE;
	}

	float sx = ::sinf(angleX);
	float cx = ::cosf(angleX);
	float sy = ::sinf(angleY);
	float cy = ::cosf(angleY);

	// カメラの方向を算出
	DirectX::XMVECTOR Front = DirectX::XMVectorSet(-cx * sy, -sx, -cx * cy, 0.0f);
	DirectX::XMVECTOR Right = DirectX::XMVectorSet(cy, 0.0f, -sy, 0.0f);
	DirectX::XMVECTOR Up = DirectX::XMVector3Cross(Right, Front);
	// カメラの視点＆注視点を算出
	DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&focus);
	DirectX::XMVECTOR Distance = DirectX::XMVectorSet(distance, distance, distance, 0.0f);
	DirectX::XMVECTOR Eye = DirectX::XMVectorSubtract(Focus, DirectX::XMVectorMultiply(Front, Distance));
	// ビュー行列からワールド行列を算出
	DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, Focus, Up);
	DirectX::XMMATRIX World = DirectX::XMMatrixTranspose(View);
	// ワールド行列から方向を算出
	Right = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(1, 0, 0, 0), World);
	Up = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0, 1, 0, 0), World);
	// 結果を格納
	DirectX::XMStoreFloat3(&eye, Eye);
	DirectX::XMStoreFloat3(&up, Up);
	DirectX::XMStoreFloat3(&right, Right);
}
