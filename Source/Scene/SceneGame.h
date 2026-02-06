#pragma once

#include "Stage/Stage.h"
#include "Camera/CameraController.h"
#include "Camera/FreeCameraController.h"
#include "Character/player.h"
#include "Character/Enemy/EnemyBoss.h"
#include "Character/Projectile/ProjectileManager.h"
#include "Scene.h"
#include "Graphics/SkyBox.h"
#include "Graphics/PostEffect.h"
//#include "BattleUI/CombatUIManager.h"

// ゲームの進行状態
enum class GameState
{
	Intro,      // 開幕演出
	Battle,     // 戦闘中
	Ending      // 決着（スローモーション演出）
};

// ゲームシーン
class SceneGame : public Scene
{
public:
	SceneGame() {}
	~SceneGame() override {}

	// 初期化
	void Initialize() override;

	// 終了化
	void Finalize() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI();

	void SelectedCamera(float elapsedTime);

private:
	//敵のスポーン位置のランダム生成
	DirectX::XMFLOAT3 GetRandomPosition();

private:
	std::unique_ptr<Player> player;
	std::unique_ptr<EnemyBoss> boss;

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
	DirectX::XMFLOAT4 direction = { -0.949f, 0.314f, 0.0f, 1.0f };
	DirectX::XMFLOAT4 Directioncolor = { 1.0f, 0.5f, 0.5f, 150.0f };

	//タイマー
	float timer;
	bool isWorldTime = false;
	float worldTime = 1.0f;

	bool isEventCamera = false;
	float lagTimer = 0.0f;

	// 演出制御用変数
	GameState currentState = GameState::Intro; // 現在の状態
	float eventTimer = 0.0f;        // 演出用タイマー
	float cameraSwitchTimer = 0.0f; // カメラ切り替え用タイマー（死亡時用）
	int cameraAngleIndex = 0;       // 現在のカメラアングル番号
	bool isWallTransparencyEnabled = true;

	// ホワイトアウト用
	std::unique_ptr<Sprite> whiteOutSprite;
	float whiteOutAlpha = 0.0f;

	// 演出用ヘルパー関数
	void UpdateIntroCamera(float elapsedTime);
	void UpdateEndingCamera(float elapsedTime, Character* deadCharacter);
};