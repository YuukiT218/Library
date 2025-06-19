#pragma once

#include <DirectXMath.h>
//#include "Character/Enemy/EnemyManager.h"

#define CameraLerp (1)

// カメラコントローラ
class CameraController
{
public:
	CameraController() {}
	~CameraController() {}

	// 更新処理
	void Update(float elapsedTime);

	// ターゲット位置設定
	void SetTarget(const DirectX::XMFLOAT3& target) { this->target = { target.x + offsetTarget.x,target.y + offsetTarget.y,target.z + offsetTarget.z }; }

	// ロックオン位置設定
	void SetRockonPoint();

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

	//コントローラー同士の引継ぎ用
	void InitCamera();

	DirectX::XMFLOAT3 GetAngle() const { return angle; }

private:
	// 外積を用いて横軸のズレ方向算出
	float CalcSide(DirectX::XMFLOAT3 p1, DirectX::XMFLOAT3 p2);

private:
	DirectX::XMFLOAT3 target = { 0, 0, 0 };//最終的な注視点
	DirectX::XMFLOAT3 eye = { 0,0,0 };//カメラ位置
	DirectX::XMFLOAT3 rockonpoint = { 0, 0, 0 };

	//Enemy* closestEnemy = nullptr;

	DirectX::XMFLOAT3 angle = { 0, 0, 0 };
	DirectX::XMFLOAT3 desiredAngle = { 0, 0, 0 };

	float rollSpeed = DirectX::XMConvertToRadians(200);
	float mouseRollSpeed = DirectX::XMConvertToRadians(10);
	float range = 12.0f;
	float maxAngleX = DirectX::XMConvertToRadians(80);
	float minAngleX = DirectX::XMConvertToRadians(3);

	DirectX::XMFLOAT3 offsetTarget = { 0,1.5,0 };

	//カメララープ前回の結果保存用
	DirectX::XMFLOAT3 newEye{ 0, 2, -20 };//現在のカメラ位置
	DirectX::XMFLOAT3 newTarget = { 0,0,0 };//現在の注視点

	DirectX::XMFLOAT3	targetWork[2] = { { 0, 0, 0 }, { 0, 0, 0 } };	// 0 : 座標, 1 : 注視点
	float				targetYoffset = 0;
	float				lengthLimit[2] = { 7, 14 };
	float				targetLimit[2] = { 0, 3 };
	float				sideValue = 1;

	float lerpSpeed = 3.8f;
	float AnglelerpSpeed = 5.f;
	float disAngleY;

	bool freeCameraFlag = false;
	bool isrockon = false;
	bool oldRockFlag = false;

	// ずらし具合を調整するための角度オフセット
	float angleOffset = DirectX::XMConvertToRadians(20);
	float adjustedYaw = 0;
	float turnSpeed = 5.f;
	float dotOffset = -0.5f;

	bool turnSwitch = false;

	//カメラシェイク用変数
	float cameraShakeRange = 0.03f;
	float shakeTime = 0.f;

	// マウス画面に固定するか
	bool isMouseLock = false;
};