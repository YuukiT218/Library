#pragma once

#include <memory>
#include "Scene.h"
#include "Camera/Camera.h"
#include "Graphics/Light.h"
#include "Model/Model.h"
#include "Camera/FreeCameraController.h"

// ルックアット処理シーン
class LookAtScene : public Scene
{
public:
	LookAtScene();
	~LookAtScene() override = default;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render(float elapsedTime) override;

	// GUI描画処理
	void DrawGUI() override;

private:
	Camera								camera;
	FreeCameraController				cameraController;
	std::shared_ptr<Model>				character;
	std::vector<Model::NodePose>		nodePoses;
	DirectX::XMFLOAT3					headLocalForward = { 0, 0, 1 };
	DirectX::XMFLOAT3					targetPosition = { 0, 0, 0 };
	float								animationSeconds = 0;
};
