#pragma once

#include "Stage/Stage.h"
#include "Camera/CameraController.h"
#include "Camera/FreeCameraController.h"
#include "Character/player.h"
#include "Character/Enemy/SilverDragonkin.h"
//#include "Character/Enemy/EnemyManager.h"
#include "Scene.h"
#include "Graphics/SkyBox.h"
#include "Graphics/PostEffect.h"
//#include "BattleUI/CombatUIManager.h"

// ゲームシーン
class SceneGame : public Scene
{
public:
	SceneGame() {}
	//~SceneGame() {}
	~SceneGame() override {}

	// 初期化
	//void Initialize();
	void Initialize() override;

	// 終了化
	//void Finalize();
	void Finalize() override;

	// 更新処理
	//void Update(float elapsedTime);
	void Update(float elapsedTime) override;

	// 描画処理
	//void Render();
	void Render(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI();

	void SelectedCamera(float elapsedTime);

private:
	//敵のスポーン位置のランダム生成
	DirectX::XMFLOAT3 GetRandomPosition();

private:
	std::unique_ptr<Player> player;
	std::unique_ptr<SilverDragonkin> dragonkin;

	std::unique_ptr<CameraController> cameraController;
	//std::unique_ptr<DeathCameraController> deathCameraController;
	std::unique_ptr<SkyBox> skyBox;
	FreeCameraController freeCameraController;

	std::unique_ptr<PostEffect> posteffect;
	//std::unique_ptr<Sprite> gauge;

	//std::unique_ptr<CombatUIManager> combatUI;

	// 点光源
	DirectX::XMFLOAT4 pointColor{ 0.0f / 255.0f, 50.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f };
	DirectX::XMFLOAT3 offsetPosition{ 0.0f, 3.0f, 0.0f };
	float attenuation = 4.5f;
	//ディレクショナルライト
	DirectX::XMFLOAT4 direction = { -0.775f, 0.125f, 0.625f, 1.0f };
	DirectX::XMFLOAT4 Directioncolor = { 1.0f, 1.0f, 1.0f, 100.0f };

	//タイマー
	float timer;
	bool isWorldTime = false;
	float worldTime = 1.0f;

	bool isEventCamera = false;
	float lagTimer = 0.0f;
};