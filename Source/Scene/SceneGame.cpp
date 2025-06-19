#include "Graphics/Graphics.h"
#include "SceneManager.h"
#include "SceneLoading.h"
#include "SceneGame.h"
#include "Scene/SceneTitle.h"
#include "SceneResult.h"
#include "Camera/CameraParam.h"
#include "Stage/StageManager.h"
#include "Stage/StageMain.h"
#include "Graphics/GpuResourceUtils.h"
//#include "Effect/EffectManager.h"
//#include "BattleUI/HitPointUI.h"
#include "Input/Input.h"
#include <map>
//#include "BattleUI/Pause.h"
//#include "BattleUI/DieUI.h"

//0~255のカラーの値を0~1に変換する処理
static const DirectX::XMFLOAT4& Color255ToNormalized(const DirectX::XMFLOAT4& color)
{
	return DirectX::XMFLOAT4
	{
		color.x / 255.0f,
		color.y / 255.0f,
		color.z / 255.0f,
		color.w / 255.0f
	};
}

// 初期化
void SceneGame::Initialize()
{
	// ステージ初期化
	StageManager& stageManager = StageManager::Instance();
	//std::shared_ptr<StageMain> stageMain = std::make_shared<StageMain>();
	StageMain* stageMain = new StageMain();
	stageManager.Register(stageMain);

	ID3D11Device* device = Graphics::Instance().GetDevice();
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();

	// プレイヤー初期化
	player = std::make_unique<Player>(device, "Data/Model/unitychan/unitychan.glb", 0.015f);

	Camera& camera = Camera::Instance();

	camera.SetEye({ 0.0f,2.0f,-20.0f });
	camera.SetFocus({ 0.0f,0.0f,0.0f });

	// カメラ設定
	camera.SetPerspectiveFov(
		camera.GetFov(),	// 画角
		screenWidth / screenHeight,			// 画面アスペクト比
		0.1f,								// ニアクリップ
		1000.0f								// ファークリップ
	);
	camera.SetLookAt(
		{ 0.f, 10.0f, 10.0f },	// 視点
		{ 0.f, 0.0f, 0.f },	// 注視点
		{ 0.f, 1.f, 0.f }		// 上ベクトル
	);
	//// カメラ設定
	//camera.SetPerspectiveFov(
	//	DirectX::XMConvertToRadians(45),	// 画角
	//	screenWidth / screenHeight,			// 画面アスペクト比
	//	0.1f,								// ニアクリップ
	//	1000.0f								// ファークリップ
	//);
	//camera.SetLookAt(
	//	{ 0, 10, 10 },		// 視点
	//	{ 0, 0, 0 },		// 注視点
	//	{ 0, 1, 0 }			// 上ベクトル
	//);

	cameraController = std::make_unique<CameraController>();
	//movieCameraController = std::make_unique<MovieCameraController>();

	//deathCameraController = std::make_unique<DeathCameraController>();

	freeCameraController.SyncCameraToController(camera);

	LightManager& lightManager = LightManager::Instance();

	//// ライト設定
	//DirectionalLight directionalLight;
	//directionalLight.direction = { direction };
	//directionalLight.color = { Directioncolor };
	//lightManager.SetDirectionalLight(directionalLight);

	//skyBox = std::make_unique<SkyBox>(device);

	//posteffect = std::make_unique<PostEffect>(device);

	//gauge = std::make_unique<Sprite>(device);

	//isEventCamera = false;

	////HPUI初期化
	//HitPointUI::Instance().Initialize(player->GetMaxHealth(), 3);

	//combatUI = std::make_unique<CombatUIManager>();

	//Pause::Instance().Initialize();
	//DieUI::Instance().Initialize();

	lagTimer = 0.0f;

	////BGMの初期化
	//AngryBGM = Audio::Instance().LoadAudioSource("Data/Sound/BGM/AngryBGM.wav");
	//ClearBGM = Audio::Instance().LoadAudioSource("Data/Sound/BGM/ClearBGM.wav");
}

void SceneGame::Finalize()
{
	StageManager::Instance().Clear();
	//EnemyManager::Instance().Clear();
	

	ShowCursor(true);

	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	// 画面クリア＆レンダーターゲット設定
	DirectX::XMFLOAT4 color = { 0.2f, 0.2f, 0.2f, 1.0f };	// RGBA(0.0～1.0);
	/*std::map<FrameBufferId, FrameBuffer*> buffers;
	for (int i = 0; i < static_cast<int>(FrameBufferId::EnumCount); i++)
	{
		buffers[static_cast<FrameBufferId>(i)] = Graphics::Instance().GetFrameBuffer(static_cast<FrameBufferId>(i));
		buffers[static_cast<FrameBufferId>(i)]->Clear(dc, color);
	}*/
}

void SceneGame::Update(float elapsedTime)
{
	//プレイヤーが死んでいるときはPauseに入れなくする
	//if (!player->IsDeathFlag())
	//	Pause::Instance().Update(elapsedTime);

	////ポーズ中は他のUpdateを通さない。
	//if (Pause::Instance().GetIsPause())
	//{
	//	//ラグの時間
	//	lagTimer = 0.3f;
	//	return;
	//}

	//ポーズを解除したときに一瞬ラグを持たせる
	if (lagTimer > 0)
	{
		lagTimer -= elapsedTime;
		return;
	}

	// ヒットストップ更新処理
	//HitStop::Instance().Update(elapsedTime);
	//if (isWorldTime)
	//{
	//	hitStopTimeScale = worldTime;
	//}
	//else
	//{
	//	hitStopTimeScale = HitStop::Instance().GetTimeScale();
	//}

	SelectedCamera(elapsedTime);

	if (Camera::Instance().GetFreeCameraFlag())
	{
		// カメラを自由にマウスで動かしたいならこっちをつける
		// カメラコントローラー更新処理
		freeCameraController.Update();
		freeCameraController.SyncControllerToCamera(Camera::Instance());
	}

	// ステージ更新処理
	StageManager::Instance().Update(elapsedTime);


	//// プレイヤー更新処理
	//player->SetLockOnCamera(CameraParam::Instance().GetRockOnEnemy());
	//player->Update(elapsedTime * HitStop::Instance().GetPlayerTimeScale());
	player->Update(elapsedTime);

	////エネミーマネージャー更新
	//EnemyManager::Instance().Update(elapsedTime);

	//// エフェクト更新処理
	//EffectManager::Instance().Update(elapsedTime * HitStop::Instance().GetEnemyTimeScale());

	////HPUI更新
	////HitPointUI::Instance().SetRockOnEnemy(cameraController->GetRockOnEnemy());
	//HitPointUI::Instance().Update(elapsedTime, player->GetHealth(), 3);
	//DieUI::Instance().Update(elapsedTime);

	//combatUI->Update(elapsedTime);

	GamePad& gamePad = Input::Instance().GetGamePad();
	Mouse& mouse = Input::Instance().GetMouse();
	// ロックオン
	/*if (ImGui::IsAnyItemHovered() == false && ImGui::GetIO().WantCaptureMouse == false)
	{
		if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER || mouse.GetButtonDown() & Mouse::BTN_MIDDLE)
		{
			CameraParam::Instance().ReversLockOnSwitch();
		}
	}*/

	LightManager& lightManager = LightManager::Instance();

	//// ポイントライト設定
	//PointLight pointLight;
	//pointLight.position.x = supportEnemy->GetPosition().x + offsetPosition.x;
	//pointLight.position.y = supportEnemy->GetPosition().y + offsetPosition.y;
	//pointLight.position.z = supportEnemy->GetPosition().z + offsetPosition.z;
	//pointLight.position.w = attenuation;
	//pointLight.color = pointColor;
	//lightManager.SetPointLight(pointLight, 1);

	timer += elapsedTime;

	//player->onExitToTitle = []() {
	//	//SceneLoading* loadingScene = new SceneLoading(new SceneTitle());
	//	SceneManager::Instance().ChangeScene(new SceneTitle());
	//	};

	//posteffect->SetUp(elapsedTime);
}

// 描画処理
void SceneGame::Render(float elapsedTime)
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();

	// 画面クリア＆レンダーターゲット設定
	DirectX::XMFLOAT4 color = { 0.2f, 0.2f, 0.2f, 1.0f };	// RGBA(0.0～1.0);
	/*std::map<FrameBufferId, FrameBuffer*> buffers;
	for (int i = 0; i < static_cast<int>(FrameBufferId::EnumCount); i++)
	{
		buffers[static_cast<FrameBufferId>(i)] = Graphics::Instance().GetFrameBuffer(static_cast<FrameBufferId>(i));
		buffers[static_cast<FrameBufferId>(i)]->Clear(dc, color);
	}

	buffers[FrameBufferId::Scene]->SetRenderTargets(dc);*/

	//ShadowMap* shadowMap = Graphics::Instance().GetShadowMap();

	Camera& camera = Camera::Instance();

	// ワールド行列計算
	DirectX::XMFLOAT4X4 worldTransform;
	DirectX::XMStoreFloat4x4(&worldTransform, DirectX::XMMatrixIdentity());

	// アニメーション更新
	//character->UpdateAnimation(elapsedTime);

	// トランスフォーム更新
	//stage->UpdateTransform(worldTransform);
	//character->UpdateTransform(worldTransform);

	LightManager& lightManager = LightManager::Instance();

	// 描画コンテキスト設定
	RenderContext rc;
	rc.camera = &camera;
	rc.deviceContext = graphics.GetDeviceContext();
	rc.renderState = graphics.GetRenderState();
	rc.lightManager = &lightManager;
	//rc.shadowMap = shadowMap;
	//rc.timer = timer;

	// シャドウマップ描画
	/*{
		shadowMap->Begin(rc, camera.GetFocus());
		if (shadowMap->GetCascade())
		{
			StageManager::Instance().SetShadowModel(shadowMap);
			player->SetShadowMap(shadowMap);
			EnemyManager::Instance().SetShadowMap(shadowMap);

			shadowMap->CascadeDraw(rc);
		}
		else
		{
			StageManager::Instance().ShadowRender(rc, shadowMap);
			player->ShadowRender(rc, shadowMap);
			EnemyManager::Instance().ShadowRender(rc, shadowMap);
		}
		shadowMap->End(rc);
	}*/

	StageManager::Instance().Debug(rc);
	// 3D描画
	{
		//shader->Draw(rc, stage.get());
		StageManager::Instance().Render(rc, ShaderId::Lambert);
		player->Render(rc, ShaderId::Basic);
		//shader->Draw(rc, character.get());
		//EnemyManager::Instance().Render(rc, shader);
	}

	//{//トレイル描画

	//	TrailRenderer* trailRenderer = graphics.GetTrailRenderer();
	//	trailRenderer->Render(dc, rc, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//}

	////プレイヤー描画
	//shader = Graphics::Instance().GetShader(ShaderId::ActorPBR);
	//{
	//	shader->Begin(rc);
	//	player->Render(rc, shader);
	//	shader->End(rc);
	//}

	/*rc.dissove = enemysupport.dissolve;
	rc.alphaFactor = enemysupport.alphaFactor;
	rc.OverriteColor = enemysupport.OverwriteColor;*/

	// スカイボックス描画
	/*skyBox->Begin(rc);
	skyBox->Render(rc);
	skyBox->End(rc);*/

	//// 3Dエフェクト描画
	//EffectManager::Instance().Render(camera.GetView(), camera.GetProjection());

	////ポストプロセス
	//{
	//	posteffect->Begin(rc);

	//	buffers[FrameBufferId::Luminance]->SetRenderTargets(dc);
	//	posteffect->LuminanceExtraction(rc, buffers[FrameBufferId::Scene]->GetColorMap());

	//	//川瀬の場合下記をコメントアウト
	//	//buffers[FrameBufferId::Display]->SetRenderTargets(dc);
	//	posteffect->KawaseBloom(rc, buffers[FrameBufferId::Scene]->GetColorMap(), buffers[FrameBufferId::Luminance]->GetColorMap(), buffers[FrameBufferId::RadialBlur]);

	//	buffers[FrameBufferId::Chromatic]->SetRenderTargets(dc);
	//	posteffect->RadialBlur(rc, buffers[FrameBufferId::RadialBlur]->GetColorMap());

	//	buffers[FrameBufferId::Display]->SetRenderTargets(dc);
	//	posteffect->ChromaticAberration(rc, buffers[FrameBufferId::Chromatic]->GetColorMap());

	//	posteffect->End(rc);
	//}

	//// プレイヤーデバッグプリミティブ描画
	//player->DrawDebugPrimitive();

	//エネミーデバッグプリミティブ描画
	//EnemyManager::Instance().DrawDebugPrimitive();

	//エネミー体力ゲージ描画
	//RenderEnemyGauge(dc, camera.GetView(), camera.GetProjection());

	//プレイヤー体力ゲージ描画処理
	/*if (!EnemyManager::Instance().IsElderDragonDead()) {
		HitPointUI::Instance().Render(elapsedTime);
	}*/

	//ダメージ表記
	/*combatUI->Render(dc, rc);

	Pause::Instance().Render(elapsedTime, dc);

	DieUI::Instance().Render(elapsedTime, dc);*/

#if _DEBUG || DEBUG
	// デバッグレンダラ描画実行
	graphics.GetShapeRenderer()->Render(dc, camera.GetView(), camera.GetProjection());

	// デバッグGUI描画
	DrawDebugGUI();
#endif
}

void SceneGame::DrawDebugGUI()
{
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Debug Menu", nullptr, ImGuiWindowFlags_None))
	{
		ImGui::Checkbox(u8"時を操るか", &isWorldTime);
		ImGui::DragFloat(u8"時の進み具合", &worldTime, 0.01f, 0.0f, 10.0f);

		StageManager::Instance().DebugImGui();

		//HitPointUI::Instance().DrawDebugGUI();
	}

	ImGui::End();

	// プレイヤーデバッグ描画
	/*player->DrawDebugGUI();

	EnemyManager::Instance().DrawDebugGUI();

	combatUI->DrawDebugGui();

	Pause::Instance().DrawDebugGUI();
	DieUI::Instance().DrawDebugGUI();*/

	if (ImGui::Begin("Graphics Menu", nullptr, ImGuiWindowFlags_None))
	{
		/*posteffect->DrawDebugGUI();

		Shader* ActorPBR = Graphics::Instance().GetShader(ShaderId::ActorPBR);
		ActorPBR->DebugImGui();

		Shader* PBR = Graphics::Instance().GetShader(ShaderId::PBR);
		PBR->DebugImGui();*/

		if (ImGui::CollapsingHeader("DirectionalLight", ImGuiTreeNodeFlags_DefaultOpen))
		{
			DirectionalLight directionalLight;
			if (ImGui::SliderFloat3("direction", &direction.x, -1.0f, +1.0f))
			{
				float x = direction.x * direction.x
					+ direction.y * direction.y
					+ direction.z * direction.z;
				x = sqrtf(x);
				direction.x /= x;
				direction.y /= x;
				direction.z /= x;
			}
			ImGui::ColorEdit3("Directioncolor", &Directioncolor.x);
			ImGui::SliderFloat("intensity", &Directioncolor.w, 0.0f, +100.0f);

			//directionalLight = { direction,Directioncolor };

			LightManager& lightManager = LightManager::Instance();
			lightManager.SetDirectionalLight(directionalLight);
		}

		if (ImGui::CollapsingHeader("PointLight", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// ImGui に渡す
			float emissivedissolve = -0.1f;//エミッシブテクスチャ用ディゾルブ
			float dissolve = -0.1f;	//ディゾルブ
			float alphaFactor = 1.0f;//アルファ値調整
			DirectX::XMFLOAT4 OverwriteColor = { 1.0f,1.0f,1.0f,1.0f };//モデルの色を変化
			ImGui::ColorEdit4("pointColor", &pointColor.x);
			ImGui::DragFloat3("offsetPosition", &offsetPosition.x, 0.1f);
			ImGui::DragFloat("attenuation", &attenuation, 0.1f);
		}

		//TrailRenderer* trailRenderer = Graphics::Instance().GetTrailRenderer();
		//trailRenderer->ImGui();

		//{//シャドウマップ用ImGUI
		//	ShadowMap* shadowMap = Graphics::Instance().GetShadowMap();

		//	shadowMap->DrawDebugGUI();
		//}

	}

	ImGui::End();

	/*if (ImGui::Begin("Camera Menu", nullptr, ImGuiWindowFlags_None))
	{
		cameraController->DrawDebugGUI();
		deathCameraController->DrawDebugGUI();
	}

	ImGui::End();*/
}

void SceneGame::SelectedCamera(float elapsedTime)
{
	//if (player->IsDeathFlag())
	//{
	//	deathCameraController->PlayerDeathCamera(elapsedTime);
	//	return;
	//}

	//if (EnemyManager::Instance().IsElderDragonDead()) {
	//	// 死亡カメラ演出へ遷移
	//	deathCameraController->EnemyDeathCamera(elapsedTime);
	//	return;
	//}

	//static bool wasEventCamera = false; // 前フレームの状態を記録

	DirectX::XMFLOAT3 target = player->GetPosition();
	Model* model = player->GetModel();
	int currentIndex = model->GetCurrentAnimationIndex();

	//bool nowEventCamera = false;

	//if (currentIndex >= 0)
	//{
	//	const AnimationConfig* config = model->GetAnimationConfig("Player", currentIndex);
	//	float animationSeconds = model->GetCurrentAnimationSeconds();

	//	for (const auto& evt : config->events)
	//	{
	//		if (evt.eventType == EventType::Camera && evt.IsActive(animationSeconds))
	//		{
	//			nowEventCamera = true;

	//			// カメラ切り替わり検知（通常 → ムービー）
	//			if (!wasEventCamera)
	//			{
	//				//movieCameraController->SetAngle(cameraController->GetAngle());
	//				movieCameraController->SetAngleFromCurrentCamera();
	//			}

	//			target.y += 2.0f;
	//			movieCameraController->SetTarget(target);
	//			movieCameraController->Update(elapsedTime);

	//			isEventCamera = true;
	//			wasEventCamera = true;
	//			return;
	//		}
	//	}
	//}

	//// カメラ切り替わり検知（ムービー → 通常）
	//if (wasEventCamera && !nowEventCamera)
	//{
	//	cameraController->InitCamera(); // 戻ったときの初期化
	//}

	//isEventCamera = false;
	cameraController->SetTarget(target);
	cameraController->Update(elapsedTime);
	//wasEventCamera = false;
}

DirectX::XMFLOAT3 SceneGame::GetRandomPosition()
{
	float x = 0;
	// ステージの高さに合わせる
	float y = -2.7f; // Yの値はそのまま
	float z = 25.0f;
	return DirectX::XMFLOAT3(x, y, z);
}
