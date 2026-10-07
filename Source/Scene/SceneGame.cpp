#include "Graphics/Graphics.h"
#include "SceneManager.h"
#include "SceneTitle.h"
#include "SceneLoading.h"
#include "SceneGame.h"
#include "SceneClear.h"
#include "SceneGameOver.h"
#include "Camera/CameraParam.h"
#include "Stage/StageManager.h"
#include "Stage/StageMain.h"
#include "Graphics/GpuResourceUtils.h"
#include "Effect/EffectManager.h"
#include "UI/BattleUI.h"
#include "UI/Pause.h"
#include "Input/Input.h"
#include "System/HitStop.h"
#include "Debug/DebugToggles.h"
#include <map>
#include <stdlib.h>
#include "System/Audio/Audio.h"

namespace
{
	// 使用するアセット
	constexpr const char* PLAYER_MODEL_PATH = "Data/Model/unitychan/unitychan.gltf";
	constexpr const char* BOSS_MODEL_PATH = "Data/Model/Rogue/SK_ROGUE_F_02.gltf";
	constexpr const char* BGM_PATH = "Data/Sound/BGM/Fight to the Death.wav";
	constexpr const char* WHITE_OUT_SPRITE_PATH = "Data/Sprite/White.png";

	// BGMの音量
	constexpr float BGM_VOLUME = 0.4f;

	// カメラの初期設定
	const DirectX::XMFLOAT3 CAMERA_UP = { 0.0f, 1.0f, 0.0f };
	constexpr float CAMERA_NEAR_Z = 0.1f;
	constexpr float CAMERA_FAR_Z = 1000.0f;

	// 画面クリア色
	const DirectX::XMFLOAT4 CLEAR_COLOR = { 0.2f, 0.2f, 0.2f, 1.0f };

	// ポーズを解除してから操作を受け付けるまでの時間
	constexpr float PAUSE_RESUME_LAG_SECONDS = 0.3f;

	// 壁の透過判定に使う、プレイヤーの体の中心の高さ
	constexpr float PLAYER_CENTER_HEIGHT = 1.0f;

	// ボスの索敵範囲（戦闘開始時）
	constexpr float BATTLE_SEARCH_RANGE = 25.0f;

	// 開幕演出の各カットの終了時間
	constexpr float INTRO_PLAYER_CUT_END = 2.0f;
	constexpr float INTRO_BOSS_CUT_END = 4.0f;
	constexpr float INTRO_END = 5.5f;

	// 開幕演出で胸元に寄るカメラの設定
	constexpr float INTRO_PLAYER_CHEST_HEIGHT = 1.3f;
	constexpr float INTRO_PLAYER_EYE_DROP = 0.3f;
	constexpr float INTRO_PLAYER_FOCUS_SHIFT = -0.4f;
	constexpr float INTRO_BOSS_CHEST_HEIGHT = 1.6f;
	constexpr float INTRO_BOSS_EYE_DROP = 0.4f;
	constexpr float INTRO_BOSS_FOCUS_SHIFT = 0.4f;
	constexpr float INTRO_CAMERA_ANGLE_DEGREE = 25.0f;
	constexpr float INTRO_CAMERA_DISTANCE = 1.0f;

	// 決着演出のスローモーションの速さ
	constexpr float ENDING_SLOW_SCALE = 0.1f;

	// 決着演出でカメラアングルを切り替える間隔と、切り替えを続ける時間
	constexpr float ENDING_CAMERA_SWITCH_INTERVAL = 1.5f;
	constexpr float ENDING_CAMERA_SWITCH_DURATION = 5.0f;

	// 決着演出のカメラ（倒れたキャラクターの中心からの位置）
	constexpr int ENDING_CAMERA_ANGLE_COUNT = 3;
	constexpr float ENDING_TARGET_HEIGHT = 1.0f;
	constexpr float ENDING_CAMERA_DISTANCE = 4.0f;
	constexpr float ENDING_LOW_ANGLE_HEIGHT = -0.5f;
	constexpr float ENDING_SIDE_ANGLE_HEIGHT = 1.0f;
	constexpr float ENDING_TOP_ANGLE_HEIGHT_SCALE = 1.5f;
	constexpr float ENDING_TOP_ANGLE_Z_OFFSET = 0.1f;  // 真上すぎるとLookAt計算でおかしくなる対策

	// ホワイトアウトを始める時間とかける時間、シーンを切り替える時間
	constexpr float WHITE_OUT_START = 4.0f;
	constexpr float WHITE_OUT_DURATION = 1.0f;
	constexpr float ENDING_END = 6.0f;
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
 	player = std::make_unique<Player>(device, PLAYER_MODEL_PATH);
	boss = std::make_unique<EnemyBoss>(device, BOSS_MODEL_PATH, 1.0f);

	bgm = Audio::Instance().LoadAudioSource(BGM_PATH);

	Camera& camera = Camera::Instance();

	// ロックオン状態の初期化（前回のシーンの状態を持ち越さない）
	CameraParam::Instance().SetLockOn(false);
	CameraParam::Instance().SetLockOnEnemy(nullptr);

	camera.SetEye({ 0.0f,2.0f,-20.0f });
	camera.SetFocus({ 0.0f,0.0f,0.0f });

	// カメラ設定
	camera.SetPerspectiveFov(
		camera.GetFov(),	// 画角
		screenWidth / screenHeight,			// 画面アスペクト比
		CAMERA_NEAR_Z,						// ニアクリップ
		CAMERA_FAR_Z						// ファークリップ
	);
	camera.SetLookAt(
		{ 0.f, 10.0f, 10.0f },	// 視点
		{ 0.f, 0.0f, 0.f },	// 注視点
		CAMERA_UP				// 上ベクトル
	);

	cameraController = std::make_unique<CameraController>();
	
	freeCameraController.SyncCameraToController(camera);

	LightManager& lightManager = LightManager::Instance();

	// ライト設定
	DirectionalLight directionalLight;
	directionalLight.direction = { direction };
	directionalLight.color = { directionColor };
	lightManager.SetDirectionalLight(directionalLight);

	skyBox = std::make_unique<SkyBox>(device);
	postEffect = std::make_unique<PostEffect>(device);

	///HPUI初期化
	BattleUI::Instance().Initialize();
	Pause::Instance().Initialize();

	// 状態とタイマーの初期化
	currentState = GameState::Intro;
	eventTimer = 0.0f;

	whiteOutSprite = std::make_unique<Sprite>(Graphics::Instance().GetDevice(), WHITE_OUT_SPRITE_PATH);
	whiteOutAlpha = 0.0f;

	lagTimer = 0.0f;
}

void SceneGame::Finalize()
{
	Input::Instance().GetGamePad().Vibrate(0.0f, 0.0f);
	EffectManager::Instance().StopAllEffects();
	ProjectileManager::Instance().Clear();
	delete bgm;

	StageManager::Instance().Clear();

	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	// 画面クリア＆レンダーターゲット設定
	std::map<FrameBufferId, FrameBuffer*> buffers;
	for (int i = 0; i < static_cast<int>(FrameBufferId::EnumCount); i++)
	{
		buffers[static_cast<FrameBufferId>(i)] = Graphics::Instance().GetFrameBuffer(static_cast<FrameBufferId>(i));
		buffers[static_cast<FrameBufferId>(i)]->Clear(dc, CLEAR_COLOR);
	}
}

void SceneGame::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	Mouse& mouse = Input::Instance().GetMouse();
	bgm->Play(true, BGM_VOLUME);

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

		// プレイヤーが死んでいるときはPauseに入れなくする
		if (!player->IsDeathFlag())
			Pause::Instance().Update(elapsedTime);

		// ポーズ中は他のUpdateを通さない。
		if (Pause::Instance().IsPause())
		{
			//ラグの時間
			gamePad.Vibrate(0.0f, 0.0f);
			lagTimer = PAUSE_RESUME_LAG_SECONDS;
			return;
		}

		// ポーズを解除したときに一瞬ラグを持たせる
		if (lagTimer > 0.0f)
		{
			lagTimer -= elapsedTime;
			return;
		}

		// ヒットストップ更新
		HitStop::Instance().Update(elapsedTime);

		// カメラ更新 (通常)
		SelectedCamera(elapsedTime);
		if (Camera::Instance().IsFreeCamera()) {
			freeCameraController.Update();
			freeCameraController.SyncControllerToCamera(Camera::Instance());
		}

		// ロックオン
		if (ImGui::IsAnyItemHovered() == false && ImGui::GetIO().WantCaptureMouse == false)
		{
			if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER || mouse.GetButtonDown() & Mouse::BTN_MIDDLE)
			{
				CameraParam::Instance().ToggleLockOn();
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
		}
		else if (boss->IsDeathFlag())
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
		Character* target = player->IsDeathFlag() ? static_cast<Character*>(player.get()) : static_cast<Character*>(boss.get());
		UpdateEndingCamera(elapsedTime, target);
		break;
	}

	LightManager& lightManager = LightManager::Instance();

	timer += elapsedTime;

	postEffect->SetUp(elapsedTime);
}

// 描画処理
void SceneGame::Render(float elapsedTime)
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();

	// 画面クリア＆レンダーターゲット設定
	std::map<FrameBufferId, FrameBuffer*> buffers;
	for (int i = 0; i < static_cast<int>(FrameBufferId::EnumCount); i++)
	{
		buffers[static_cast<FrameBufferId>(i)] = Graphics::Instance().GetFrameBuffer(static_cast<FrameBufferId>(i));
		buffers[static_cast<FrameBufferId>(i)]->Clear(dc, CLEAR_COLOR);
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
	rc.targetPosition = DirectX::XMFLOAT3(pPos.x, pPos.y + PLAYER_CENTER_HEIGHT, pPos.z);
	rc.enableWallTransparency = isWallTransparencyEnabled;

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
	EffectManager::Instance().Render(rc);

	//ポストプロセス
	{
		postEffect->Begin(rc);

		buffers[FrameBufferId::Luminance]->SetRenderTargets(dc);
		postEffect->LuminanceExtraction(rc, buffers[FrameBufferId::Scene]->GetColorMap());

		postEffect->KawaseBloom(rc, buffers[FrameBufferId::Scene]->GetColorMap(), buffers[FrameBufferId::Luminance]->GetColorMap(), buffers[FrameBufferId::RadialBlur]);

		buffers[FrameBufferId::Chromatic]->SetRenderTargets(dc);
		postEffect->RadialBlur(rc, buffers[FrameBufferId::RadialBlur]->GetColorMap());

		buffers[FrameBufferId::Display]->SetRenderTargets(dc);
		postEffect->ChromaticAberration(rc, buffers[FrameBufferId::Chromatic]->GetColorMap());

		postEffect->End(rc);
	}

	// プレイヤーデバッグプリミティブ描画
	player->DrawDebugPrimitive();

	//エネミーデバッグプリミティブ描画
	boss->DrawDebugPrimitive();

	//ProjectileManager::Instance().DrawDebugPrimitive();

	//プレイヤー体力ゲージ描画処理
	if (currentState == GameState::Battle)
	{
		if (!Pause::Instance().IsPause())
			BattleUI::Instance().Render(elapsedTime);

		Pause::Instance().Render(elapsedTime, dc);
	}

	if (whiteOutAlpha > 0.0f)
	{
		RenderState* renderState = graphics.GetRenderState();

		// ステート設定
		FLOAT blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		UINT sampleMask = 0xFFFFFFFF;
		dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), blendFactor, sampleMask);
		dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
		dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));
		ID3D11SamplerState* samplers[] = { renderState->GetSamplerState(SamplerState::LinearClamp) };
		dc->PSSetSamplers(0, 1, samplers);

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

		// 比較動画の撮影用トグル
		DebugToggles::Instance().DrawDebugGUI();

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
		postEffect->DrawDebugGUI();

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
			ImGui::ColorEdit3("directionColor", &directionColor.x);
			ImGui::SliderFloat("intensity", &directionColor.w, 0.0f, +1000.0f);

			directionalLight = { direction,directionColor };

			LightManager& lightManager = LightManager::Instance();
			lightManager.SetDirectionalLight(directionalLight);
		}

		if (ImGui::CollapsingHeader("PointLight", ImGuiTreeNodeFlags_DefaultOpen))
		{
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

// キャラクターの胸元に寄ったカメラを設定する
void SceneGame::SetCloseUpCamera(const Character& character, float chestHeight, float angleOffsetDegree, float eyeDrop, float focusShift)
{
	// 基準となる「胸」の位置
	DirectX::XMFLOAT3 baseTarget = character.GetPosition();
	baseTarget.y += chestHeight;

	// カメラの位置決定
	float cameraAngle = character.GetAngle().y + DirectX::XMConvertToRadians(angleOffsetDegree);

	DirectX::XMFLOAT3 eye = {
		baseTarget.x + sinf(cameraAngle) * INTRO_CAMERA_DISTANCE,
		baseTarget.y - eyeDrop,
		baseTarget.z + cosf(cameraAngle) * INTRO_CAMERA_DISTANCE
	};

	// 注視点を横にずらして、キャラクターを画面の端に寄せる
	float dx = baseTarget.x - eye.x;
	float dz = baseTarget.z - eye.z;
	DirectX::XMFLOAT3 rightVec = { -dz, 0.0f, dx }; // 右ベクトル

	// 正規化
	float len = sqrtf(rightVec.x * rightVec.x + rightVec.z * rightVec.z);
	if (len > 0.0f) {
		rightVec.x /= len;
		rightVec.z /= len;
	}

	DirectX::XMFLOAT3 finalFocus = {
		baseTarget.x + rightVec.x * focusShift,
		baseTarget.y,
		baseTarget.z + rightVec.z * focusShift
	};

	Camera::Instance().SetLookAt(eye, finalFocus, CAMERA_UP);
}

void SceneGame::UpdateIntroCamera(float elapsedTime)
{
	eventTimer += elapsedTime;

	// プレイヤーの顔アップ（距離が近いのでずらす量は控えめにしないと画面外に出る）
	if (eventTimer < INTRO_PLAYER_CUT_END)
	{
		SetCloseUpCamera(*player, INTRO_PLAYER_CHEST_HEIGHT, INTRO_CAMERA_ANGLE_DEGREE, INTRO_PLAYER_EYE_DROP, INTRO_PLAYER_FOCUS_SHIFT);
	}
	// ボスの顔アップ（右斜め下から。画面右に配置するため左を見る）
	else if (eventTimer < INTRO_BOSS_CUT_END)
	{
		SetCloseUpCamera(*boss, INTRO_BOSS_CHEST_HEIGHT, -INTRO_CAMERA_ANGLE_DEGREE, INTRO_BOSS_EYE_DROP, INTRO_BOSS_FOCUS_SHIFT);
	}
	// 戦闘開始直前の通常カメラフェーズ
	// ここでカメラをプレイヤー背面に戻すが、まだ currentState は Intro のまま
	else if (eventTimer < INTRO_END)
	{
		SelectedCamera(elapsedTime);
	}
	// 演出終了 -> 戦闘開始
	else
	{
		currentState = GameState::Battle;
		CameraParam::Instance().SetLockOn(true);
		boss->SetSearchRange(BATTLE_SEARCH_RANGE);
	}
}

void SceneGame::UpdateEndingCamera(float elapsedTime, Character* deadCharacter)
{
	// ゲーム進行速度を遅くする (スローモーション)
	float scaledTime = elapsedTime * ENDING_SLOW_SCALE;

	// キャラクター等の更新はスローで行う
	player->Update(scaledTime);
	boss->Update(scaledTime);
	EffectManager::Instance().Update(elapsedTime);

	// カメラアングルの切り替え (カメラはリアルタイム elapsedTime で動かす)
	eventTimer += elapsedTime;      // 全体の経過時間
	cameraSwitchTimer += elapsedTime; // アングル切り替え用タイマー

	// 一定時間ごとにアングル変更
	if (cameraSwitchTimer > ENDING_CAMERA_SWITCH_INTERVAL && eventTimer < ENDING_CAMERA_SWITCH_DURATION)
	{
		cameraSwitchTimer = 0.0f;
		cameraAngleIndex++;
	}

	Camera& camera = Camera::Instance();
	DirectX::XMFLOAT3 targetPos = deadCharacter->GetPosition();
	targetPos.y += ENDING_TARGET_HEIGHT; // 中心座標補正

	// ランダムあるいは定義されたアングル位置
	// ここでは簡易的に周りを回るような3視点
	DirectX::XMFLOAT3 eyePos = targetPos;

	switch (cameraAngleIndex % ENDING_CAMERA_ANGLE_COUNT)
	{
	case 0: // 正面下からあおり
		eyePos.z -= ENDING_CAMERA_DISTANCE;
		eyePos.y += ENDING_LOW_ANGLE_HEIGHT;
		break;
	case 1: // 横から
		eyePos.x += ENDING_CAMERA_DISTANCE;
		eyePos.y += ENDING_SIDE_ANGLE_HEIGHT;
		break;
	case 2: // 真上から
		eyePos.y += ENDING_CAMERA_DISTANCE * ENDING_TOP_ANGLE_HEIGHT_SCALE;
		eyePos.z += ENDING_TOP_ANGLE_Z_OFFSET;
		break;
	}

	camera.SetLookAt(eyePos, targetPos, CAMERA_UP);

	// ホワイトアウト処理（アルファを0->1に）
	if (eventTimer > WHITE_OUT_START)
	{
		whiteOutAlpha = (eventTimer - WHITE_OUT_START) / WHITE_OUT_DURATION;
	}

	// シーン遷移
	if (eventTimer > ENDING_END) // 演出終了
	{
		EffectManager::Instance().StopAllEffects();
		ProjectileManager::Instance().Clear();
		if (player->IsDeathFlag())
		{
			// ゲームオーバー画面へ
			SceneManager::Instance().ChangeScene(new SceneGameOver());
		}
		else
		{
			// クリア画面へ
			SceneManager::Instance().ChangeScene(new SceneClear());
		}
	}
}
