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
#include "Effect/EffectManager.h"
#include "UI/BattleUI.h"
#include "UI/Pause.h"
#include "Input/Input.h"
#include "System/HitStop.h"
#include <map>
//#include "BattleUI/DieUI.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

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
	EffectManager::Instance().StopAllEffects();
	// ステージ初期化
	StageManager& stageManager = StageManager::Instance();
	StageMain* stageMain = new StageMain();
	stageManager.Register(stageMain);

	ID3D11Device* device = Graphics::Instance().GetDevice();
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();

	// プレイヤー初期化
	player = std::make_unique<Player>(device, "Data/Model/unitychan/unitychan.gltf");
	boss = std::make_unique<EnemyBoss>(device, "Data/Model/Mannequin/SK_Mannequin.gltf", 1.0f);

	Camera& camera = Camera::Instance();
	CameraParam::Instance().SetIsLockOn(false);

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

	cameraController = std::make_unique<CameraController>();
	
	freeCameraController.SyncCameraToController(camera);

	LightManager& lightManager = LightManager::Instance();

	// ライト設定
	DirectionalLight directionalLight;
	directionalLight.direction = { direction };
	directionalLight.color = { Directioncolor };
	lightManager.SetDirectionalLight(directionalLight);

	skyBox = std::make_unique<SkyBox>(device);
	posteffect = std::make_unique<PostEffect>(device);
	//gauge = std::make_unique<Sprite>(device);

	//isEventCamera = false;

	///HPUI初期化
	BattleUI::Instance().Initialize();
	Pause::Instance().Initialize();
	//DieUI::Instance().Initialize();

	// --- 追加: 状態とタイマーの初期化 ---
	currentState = GameState::Intro;
	eventTimer = 0.0f;

	// ホワイトアウト用スプライトの生成 (1x1の白画像を引き伸ばす想定、または専用画像)
	// ※適当な白画像("Data/Sprite/White.png")を用意するか、シェーダーで白く描画してください
	whiteOutSprite = std::make_unique<Sprite>(Graphics::Instance().GetDevice(), "Data/Sprite/White.png");
	whiteOutAlpha = 0.0f;

	lagTimer = 0.0f;
}

void SceneGame::Finalize()
{
	EffectManager::Instance().StopAllEffects();
	ProjectileManager::Instance().Clear();

	StageManager::Instance().Clear();
	
	ShowCursor(true);

	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	// 画面クリア＆レンダーターゲット設定
	DirectX::XMFLOAT4 color = { 0.2f, 0.2f, 0.2f, 1.0f };	// RGBA(0.0～1.0);
	std::map<FrameBufferId, FrameBuffer*> buffers;
	for (int i = 0; i < static_cast<int>(FrameBufferId::EnumCount); i++)
	{
		buffers[static_cast<FrameBufferId>(i)] = Graphics::Instance().GetFrameBuffer(static_cast<FrameBufferId>(i));
		buffers[static_cast<FrameBufferId>(i)]->Clear(dc, color);
	}
}

void SceneGame::Update(float elapsedTime)
{
	// プレイヤーが死んでいるときはPauseに入れなくする
	if (!player->IsDeathFlag())
		Pause::Instance().Update(elapsedTime);

	// ポーズ中は他のUpdateを通さない。
	if (Pause::Instance().GetIsPause())
	{
		//ラグの時間
		lagTimer = 0.3f;
		return;
	}

	// ポーズを解除したときに一瞬ラグを持たせる
	if (lagTimer > 0)
	{
		lagTimer -= elapsedTime;
		return;
	}

	GamePad& gamePad = Input::Instance().GetGamePad();
	Mouse& mouse = Input::Instance().GetMouse();

	// --- 状態ごとの分岐 ---
	switch (currentState)
	{
	case GameState::Intro:
		// イントロ演出更新
		isWallTransparencyEnabled = false;
		UpdateIntroCamera(elapsedTime);
		player->EditUpdate(elapsedTime);
		boss->Update(elapsedTime);
		EffectManager::Instance().Update(elapsedTime);
		StageManager::Instance().Update(elapsedTime);
		break;

	case GameState::Battle:
		isWallTransparencyEnabled = true;
		// ヒットストップ更新
		HitStop::Instance().Update(elapsedTime);

		// カメラ更新 (通常)
		SelectedCamera(elapsedTime);
		if (Camera::Instance().GetFreeCameraFlag()) {
			freeCameraController.Update();
			freeCameraController.SyncControllerToCamera(Camera::Instance());
		}

		// ロックオン
		if (ImGui::IsAnyItemHovered() == false && ImGui::GetIO().WantCaptureMouse == false)
		{
			if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER || mouse.GetButtonDown() & Mouse::BTN_MIDDLE)
			{
				CameraParam::Instance().ReversLockOnSwitch();
			}
		}

		// 各更新処理
		StageManager::Instance().Update(elapsedTime);
		player->Update(elapsedTime * HitStop::Instance().GetPlayerTimeScale());
		boss->Update(elapsedTime);
		ProjectileManager::Instance().Update(elapsedTime);
		EffectManager::Instance().Update(elapsedTime);
		BattleUI::Instance().Update(elapsedTime);

		// --- 死亡判定チェック ---
		if (player->IsDeathFlag())
		{
			currentState = GameState::Ending;
			eventTimer = 0.0f;
			cameraSwitchTimer = 0.0f;
			// 死亡演出に入った瞬間の初期設定
		}
		else if (boss->IsDeathFlag()) // EnemyBossにIsDeathFlag()相当の機能があると仮定
		{
			currentState = GameState::Ending;
			eventTimer = 0.0f;
			cameraSwitchTimer = 0.0f;
		}
		break;

	case GameState::Ending:
		isWallTransparencyEnabled = false;
		// 死亡演出更新
		// どちらが死んだか判定して渡す
		Character* target = player->IsDeathFlag() ? (Character*)player.get() : (Character*)boss.get();
		UpdateEndingCamera(elapsedTime, target);
		break;
	}

	LightManager& lightManager = LightManager::Instance();

	// ポイントライト設定
	/*PointLight pointLight;
	for (int i = 0; i < POINT_MAX; i++)
	{
		pointLight.position.x = lightManager.GetPointLight(i).position.x + offsetPosition.x;
		pointLight.position.y = lightManager.GetPointLight(i).position.y + offsetPosition.y;
		pointLight.position.z = lightManager.GetPointLight(i).position.z + offsetPosition.z;
	}
	pointLight.position.w = attenuation;
	pointLight.color = pointColor;
	lightManager.SetPointLight(pointLight, 1);*/

	timer += elapsedTime;

	//player->onExitToTitle = []() {
	//	//SceneLoading* loadingScene = new SceneLoading(new SceneTitle());
	//	SceneManager::Instance().ChangeScene(new SceneTitle());
	//	};

	posteffect->SetUp(elapsedTime);
}

// 描画処理
void SceneGame::Render(float elapsedTime)
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();

	// 画面クリア＆レンダーターゲット設定
	DirectX::XMFLOAT4 color = { 0.2f, 0.2f, 0.2f, 1.0f };	// RGBA(0.0～1.0);
	std::map<FrameBufferId, FrameBuffer*> buffers;
	for (int i = 0; i < static_cast<int>(FrameBufferId::EnumCount); i++)
	{
		buffers[static_cast<FrameBufferId>(i)] = Graphics::Instance().GetFrameBuffer(static_cast<FrameBufferId>(i));
		buffers[static_cast<FrameBufferId>(i)]->Clear(dc, color);
	}

	buffers[FrameBufferId::Scene]->SetRenderTargets(dc);

	ShadowMap* shadowMap = Graphics::Instance().GetShadowMap();

	Camera& camera = Camera::Instance();

	// ワールド行列計算
	DirectX::XMFLOAT4X4 worldTransform;
	DirectX::XMStoreFloat4x4(&worldTransform, DirectX::XMMatrixIdentity());

	LightManager& lightManager = LightManager::Instance();

	// 描画コンテキスト設定
	RenderContext rc;
	rc.camera = &camera;
	rc.deviceContext = graphics.GetDeviceContext();
	rc.renderState = graphics.GetRenderState();
	rc.lightManager = &lightManager;
	rc.shadowMap = shadowMap;
	rc.timer = timer;
	DirectX::XMFLOAT3 pPos = player->GetPosition();
	rc.targetPosition = DirectX::XMFLOAT3(pPos.x, pPos.y + 1.0f, pPos.z);
	rc.enableWallTransparency = isWallTransparencyEnabled;

	// MEMO : VS変換にておかしな形に(LVP空間に変換する前は正しい形)
	//        LVPの値は一見おかしな値に見えなかった

	//シャドウマップ描画
	shadowMap->Begin(rc, DirectX::XMFLOAT3(0,0,0));
	{
		StageManager::Instance().ShadowRender(rc, shadowMap);
		player->ShadowRender(rc, shadowMap);
		boss->ShadowRender(rc, shadowMap);
	}
	shadowMap->End(rc);

	StageManager::Instance().Debug(rc);
	// 3D描画
	{
		player->Render(rc, ShaderId::PBR);
		boss->Render(rc, ShaderId::PBR);
		StageManager::Instance().Render(rc, ShaderId::PBR);
	}

	// トレイル描画
	{
		TrailRenderer* trailRenderer = graphics.GetTrailRenderer();
		trailRenderer->Render(dc, rc, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	}

	// スカイボックス描画
	skyBox->Begin(rc);
	skyBox->Render(rc);
	skyBox->End(rc);

	// 3Dエフェクト描画
	EffectManager::Instance().Render(camera.GetView(), camera.GetProjection());

	//ポストプロセス
	{
		posteffect->Begin(rc);

		buffers[FrameBufferId::Luminance]->SetRenderTargets(dc);
		posteffect->LuminanceExtraction(rc, buffers[FrameBufferId::Scene]->GetColorMap());

		posteffect->KawaseBloom(rc, buffers[FrameBufferId::Scene]->GetColorMap(), buffers[FrameBufferId::Luminance]->GetColorMap(), buffers[FrameBufferId::RadialBlur]);

		buffers[FrameBufferId::Chromatic]->SetRenderTargets(dc);
		posteffect->RadialBlur(rc, buffers[FrameBufferId::RadialBlur]->GetColorMap());

		buffers[FrameBufferId::Display]->SetRenderTargets(dc);
		posteffect->ChromaticAberration(rc, buffers[FrameBufferId::Chromatic]->GetColorMap());

		posteffect->End(rc);
	}

	// プレイヤーデバッグプリミティブ描画
	player->DrawDebugPrimitive();

	//エネミーデバッグプリミティブ描画
	boss->DrawDebugPrimitive();

	//ProjectileManager::Instance().DrawDebugPrimitive();

	//プレイヤー体力ゲージ描画処理
	if (currentState == GameState::Battle)
	{
		BattleUI::Instance().Render(elapsedTime);	

		Pause::Instance().Render(elapsedTime, dc);
	}

	if (whiteOutAlpha > 0.0f)
	{
		Graphics& graphics = Graphics::Instance();
		ID3D11DeviceContext* dc = graphics.GetDeviceContext();

		// 画面全体に引き伸ばして描画
		// 引数: x, y, z, w, h, angle, r, g, b, a
		whiteOutSprite->Render(dc, 0, 0, 0,
			graphics.GetScreenWidth(), graphics.GetScreenHeight(),
			0, 1.0f, 1.0f, 1.0f, whiteOutAlpha);
	}

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
	player->DrawDebugGUI();
	boss->DrawDebugGUI();
	BattleUI::Instance().DrawDebugGUI();
	Pause::Instance().DrawDebugGUI();

	if (ImGui::Begin("Graphics Menu", nullptr, ImGuiWindowFlags_None))
	{
		posteffect->DrawDebugGUI();

		Shader* PBR = Graphics::Instance().GetShader(ShaderId::PBR);
		PBR->ImGui();

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
			ImGui::SliderFloat("intensity", &Directioncolor.w, 0.0f, +1000.0f);

			directionalLight = { direction,Directioncolor };

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

		ImGui::DragFloat("WhiteOutAlpha", &whiteOutAlpha, 0.01f);

		TrailRenderer* trailRenderer = Graphics::Instance().GetTrailRenderer();
		trailRenderer->ImGui();

		{//シャドウマップ用ImGUI
			ShadowMap* shadowMap = Graphics::Instance().GetShadowMap();

			shadowMap->DrawDebugGUI();
		}

	}

	ImGui::End();

	if (ImGui::Begin("Camera Menu", nullptr, ImGuiWindowFlags_None))
	{
		cameraController->DrawDebugGUI();
		//deathCameraController->DrawDebugGUI();
	}

	ImGui::End();
}

void SceneGame::SelectedCamera(float elapsedTime)
{
	DirectX::XMFLOAT3 target = player->GetPosition();
	Model* model = player->GetModel();
	int currentIndex = model->GetCurrentAnimationIndex();
	cameraController->SetTarget(target);
	cameraController->Update(elapsedTime);
}

DirectX::XMFLOAT3 SceneGame::GetRandomPosition()
{
	float x = 0;
	// ステージの高さに合わせる
	float y = -2.7f; // Yの値はそのまま
	float z = 25.0f;
	return DirectX::XMFLOAT3(x, y, z);
}

void SceneGame::UpdateIntroCamera(float elapsedTime)
{
	eventTimer += elapsedTime;
	Camera& camera = Camera::Instance();

	// 0.0秒～2.0秒: プレイヤーの顔アップ
	if (eventTimer < 2.0f)
	{
		// 1. 基準となる「胸」の位置
		DirectX::XMFLOAT3 baseTarget = player->GetPosition();
		baseTarget.y += 1.3f; // 顔(1.5f)より少し下げて胸元にする

		// 2. カメラの位置決定 (左斜め下かつ至近距離)
		float playerAngle = player->GetAngle().y;
		float camAngle = playerAngle + DirectX::XMConvertToRadians(25.0f); // 角度は浅めに

		// ★ここを調整: 距離を1.0fまで詰める（以前は2.5f）
		float dist = 1.0f;

		DirectX::XMFLOAT3 eye = {
			baseTarget.x + sinf(camAngle) * dist,
			baseTarget.y - 0.3f, // あおり具合も距離に合わせて微調整
			baseTarget.z + cosf(camAngle) * dist
		};

		// 3. 注視点をずらす (画面左に配置するため、右を見る)
		float dx = baseTarget.x - eye.x;
		float dz = baseTarget.z - eye.z;
		DirectX::XMFLOAT3 rightVec = { -dz, 0.0f, dx }; // 右ベクトル

		// 正規化
		float len = sqrtf(rightVec.x * rightVec.x + rightVec.z * rightVec.z);
		if (len > 0.0f) {
			rightVec.x /= len;
			rightVec.z /= len;
		}

		// ★ここを調整: 距離が近いのでオフセット量は控えめにしないと画面外に出る
		float offsetAmount = -0.4f;

		DirectX::XMFLOAT3 finalFocus = {
			baseTarget.x + rightVec.x * offsetAmount,
			baseTarget.y, // 注視点の高さは胸のまま
			baseTarget.z + rightVec.z * offsetAmount
		};

		camera.SetLookAt(eye, finalFocus, { 0, 1, 0 });
	}
	// 2.0秒～4.0秒: ボス (画面右側・超近接)
	else if (eventTimer < 4.0f)
	{
		// 1. 基準となる「胸」の位置
		DirectX::XMFLOAT3 baseTarget = boss->GetPosition();
		baseTarget.y += 1.6f; // ボスの胸の高さ（モデルに合わせて調整）

		// 2. カメラの位置決定 (右斜め下)
		float bossAngle = boss->GetAngle().y;
		float camAngle = bossAngle - DirectX::XMConvertToRadians(25.0f);

		// ★ここを調整: ボスのサイズに合わせて詰める
		float dist = 1.0f;

		DirectX::XMFLOAT3 eye = {
			baseTarget.x + sinf(camAngle) * dist,
			baseTarget.y - 0.4f,
			baseTarget.z + cosf(camAngle) * dist
		};

		// 3. 注視点をずらす (画面右に配置するため、左を見る)
		float dx = baseTarget.x - eye.x;
		float dz = baseTarget.z - eye.z;
		DirectX::XMFLOAT3 rightVec = { -dz, 0.0f, dx };

		float len = sqrtf(rightVec.x * rightVec.x + rightVec.z * rightVec.z);
		if (len > 0.0f) {
			rightVec.x /= len;
			rightVec.z /= len;
		}

		// ★ここを調整: 左方向へオフセット
		float offsetAmount = 0.4f;

		DirectX::XMFLOAT3 finalFocus = {
			baseTarget.x + rightVec.x * offsetAmount,
			baseTarget.y,
			baseTarget.z + rightVec.z * offsetAmount
		};

		camera.SetLookAt(eye, finalFocus, { 0, 1, 0 });
	}
	// 戦闘開始直前の「通常カメラ」フェーズ (4.0s ~ 5.5s)
	// ここでカメラをプレイヤー背面に戻すが、まだ currentState は Intro のまま
	else if (eventTimer < 5.5f)
	{
		SelectedCamera(elapsedTime);
	}
	// 4.0秒: 演出終了 -> 戦闘開始
	else
	{
		currentState = GameState::Battle;
		boss->SetSearchRange(25.0f);
	}
}

void SceneGame::UpdateEndingCamera(float elapsedTime, Character* deadCharacter)
{
	// ゲーム進行速度を遅くする (スローモーション)
	float slowScale = 0.1f; // 通常の10%の速度
	float scaledTime = elapsedTime * slowScale;

	// キャラクター等の更新はスローで行う
	// ヒットストップの影響を除外したい場合は直接Updateを呼ぶ
	player->Update(scaledTime);
	boss->Update(scaledTime);
	EffectManager::Instance().Update(elapsedTime);
	// StageManagerなどは背景なので通常速度でも良いが、違和感なければスローに合わせる

	// カメラアングルの切り替え (カメラはリアルタイム elapsedTime で動かす)
	eventTimer += elapsedTime;      // 全体の経過時間
	cameraSwitchTimer += elapsedTime; // アングル切り替え用タイマー

	float switchInterval = 1.5f; // 1.5秒ごとにアングル変更

	if (cameraSwitchTimer > switchInterval && eventTimer < 5.0f) // 5秒間演出
	{
		cameraSwitchTimer = 0.0f;
		cameraAngleIndex++;
	}

	Camera& camera = Camera::Instance();
	DirectX::XMFLOAT3 targetPos = deadCharacter->GetPosition();
	targetPos.y += 1.0f; // 中心座標補正

	// ランダムあるいは定義されたアングル位置
	// ここでは簡易的に周りを回るような3視点
	DirectX::XMFLOAT3 eyePos = targetPos;
	float dist = 4.0f; // 距離

	switch (cameraAngleIndex % 3)
	{
	case 0: // 正面下からあおり
		eyePos.z -= dist;
		eyePos.y -= 0.5f;
		break;
	case 1: // 横から
		eyePos.x += dist;
		eyePos.y += 1.0f;
		break;
	case 2: // 真上から
		eyePos.y += dist * 1.5f;
		eyePos.z += 0.1f; // 真上すぎるとLookAt計算でおかしくなる対策
		break;
	}

	camera.SetLookAt(eyePos, targetPos, { 0, 1, 0 });

	// ホワイトアウト処理 (最後の1.5秒くらいでフェードイン)
	if (eventTimer > 4.0f)
	{
		// 4.0秒時点から1.0秒かけてアルファを0->1に
		whiteOutAlpha = (eventTimer - 4.0f) / 1.0f;
		if (whiteOutAlpha > 1.0f) whiteOutAlpha = 1.0f;
	}

	// シーン遷移
	if (eventTimer > 6.0f) // 演出終了
	{
		EffectManager::Instance().StopAllEffects();
		ProjectileManager::Instance().Clear();
		SceneManager::Instance().ChangeScene(new SceneResult());
	}
}
