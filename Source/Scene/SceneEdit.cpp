#include "Graphics/Graphics.h"
#include "System/HitStop.h"
#include "SceneManager.h"
#include "SceneLoading.h"
#include "SceneEdit.h"
#include "Editor/BehaviorTreeEditor.h"
#include "SceneClear.h"
#include "Camera/CameraParam.h"
#include "Stage/StageManager.h"
#include "Stage/StageMain.h"
#include "Graphics/GpuResourceUtils.h"

#include "Input/Input.h"
#include "System/AnimationConfigLoader.h"

#include <fstream>
#include <WICTextureLoader.h>
#include <algorithm>

#include "ImSequencer.h"

using json = nlohmann::json;

DirectX::XMFLOAT3 DirectionToEuler(const DirectX::XMFLOAT3& dir)
{
	float pitch = std::asin(-dir.y);
	float yaw = std::atan2(dir.x, dir.z);
	return DirectX::XMFLOAT3(pitch, yaw, 0.0f);
}

// 初期化
void SceneEdit::Initialize()
{
	// ステージ初期化
	StageManager& stageManager = StageManager::Instance();
	StageMain* stageMain = new StageMain();
	stageManager.Register(stageMain);

	ID3D11Device* device = Graphics::Instance().GetDevice();
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();

	// プレイヤー初期化
	player = std::make_unique<Player>(device, "Data/Model/unitychan/unitychan.gltf");
	boss = std::make_unique<EnemyBoss>(device, "Data/Model/Rogue/SK_ROGUE_F_02.gltf", 1.0f);

	Camera& camera = Camera::Instance();

	// カメラ設定
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),	// 画角
		screenWidth / screenHeight,			// 画面アスペクト比
		0.1f,								// ニアクリップ
		1000.0f								// ファークリップ
	);
	camera.SetLookAt(
		{ 0, 10, 20 },	// 視点
		{ 0, 0, 0 },	// 注視点
		{ 0, 1, 0 }		// 上ベクトル
	);
	cameraController = std::make_unique<EditCameraController>();

	freecameraController.SyncCameraToController(camera);

	LightManager& lightManager = LightManager::Instance();

	// ライト設定
	DirectionalLight directionalLight;
	directionalLight.direction = { direction };
	directionalLight.color = { directionColor };
	lightManager.SetDirectionalLight(directionalLight);

	skyBox = std::make_unique<SkyBox>(device);

	posteffect = std::make_unique<PostEffect>(device);
	selectedCharacter = player.get();


}

void SceneEdit::Finalize()
{
	StageManager::Instance().Clear();
}

void SceneEdit::Update(float elapsedTime)
{
	//// ヒットストップ更新処理
	//HitStop::Instance().Update(elapsedTime);
	//float gameTimeScale = HitStop::Instance().GetTimeScale();

	// カメラ更新処理
	Character* targetCharacter = selectedCharacter; // ImGuiで選んだキャラ
	if (targetCharacter) {
		DirectX::XMFLOAT3 target = targetCharacter->GetPosition();
		target.y += 2;
		cameraController->SetTarget(target);
		cameraController->Update(elapsedTime);
	}

	if (Camera::Instance().GetFreeCameraFlag())
	{
		// カメラを自由にマウスで動かしたいならこっちをつける
		// カメラコントローラー更新処理
		freecameraController.Update();
		freecameraController.SyncControllerToCamera(Camera::Instance());
	}

	// ステージ更新処理
	StageManager::Instance().Update(elapsedTime);

	// プレイヤー更新処理
	//player->SetLockOnCamera(cameraController->GetRockOnEnemy());
	player->EditUpdate(elapsedTime);

	boss->EditUpdate(elapsedTime);

	// 行動エディタのホットリロード監視
	BehaviorTreeEditor::Instance().Update(elapsedTime, boss.get());

	// エフェクト更新処理
	//EffectManager::Instance().Update(elapsedTime);

	//combatUI->Update(elapsedTime);

	LightManager& lightManager = LightManager::Instance();

	timer += elapsedTime;
}

// 描画処理
void SceneEdit::Render(float elapsedTime, int width, int height)
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
	rc.deviceContext = Graphics::Instance().GetDeviceContext();
	rc.renderState = Graphics::Instance().GetRenderState();
	rc.lightManager = &lightManager;
	rc.shadowMap = shadowMap;
	rc.timer = timer;

	// シャドウマップ描画
	shadowMap->Begin(rc, DirectX::XMFLOAT3(0, 0, 0));
	{
		StageManager::Instance().ShadowRender(rc, shadowMap);
		player->ShadowRender(rc, shadowMap);
		boss->ShadowRender(rc, shadowMap);
	}
	shadowMap->End(rc);

	StageManager::Instance().Debug(rc);
	// 3D描画
	{
		StageManager::Instance().Render(rc, ShaderId::PBR);
		player->Render(rc, ShaderId::PBR);
		boss->Render(rc, ShaderId::PBR);
	}

	// スカイボックス描画
	skyBox->Begin(rc);
	skyBox->Render(rc);
	skyBox->End(rc);

	// 3Dエフェクト描画
	//EffectManager::Instance().Render(camera.GetView(), camera.GetProjection());

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

	buffers[FrameBufferId::Scene]->SetRenderTargets(dc);
	// プレイヤーデバッグプリミティブ描画
	player->DrawDebugPrimitive();

	//エネミーデバッグプリミティブ描画
	boss->DrawDebugPrimitive();

	// デバッグレンダラ描画実行
	graphics.GetShapeRenderer()->Render(dc, camera.GetView(), camera.GetProjection());

	buffers[FrameBufferId::Display]->SetRenderTargets(dc);
	// デバッグGUI描画
	DrawDebugGUI(elapsedTime);
}

void SceneEdit::DrawDebugGUI(float elapsedTime)
{
	//StyleをChangeするのでもとに戻すために保持しておく
	ImGuiStyle originalStyle = ImGui::GetStyle();

	//このEditだけUnity風にStyleChange
	ImGuiSetStyle();

	cameraController->DrawDebugGUI();

	// 敵の行動パターンエディタ
	BehaviorTreeEditor::Instance().DrawGui(boss.get());

	ImGui::Begin("Game View", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	ImVec2 availSize = ImGui::GetContentRegionAvail();

	// 安全チェック（ウィンドウが極端に小さい時は何もしない）
	if (availSize.x < 10 || availSize.y < 10)
	{
		ImGui::Text("Game view too small");
		ImGui::End();
		return;
	}

	int width = static_cast<int>(availSize.x);
	int height = static_cast<int>(availSize.y);

	// FrameBufferリサイズ
	static int prevWidth = 0, prevHeight = 0;
	if (width != prevWidth || height != prevHeight)
	{
		Graphics::Instance().ResizeFrameBuffer(FrameBufferId::Scene, width, height);
		prevWidth = width;
		prevHeight = height;
	}

	// 描画に使うフレームバッファ
	auto* fb = Graphics::Instance().GetFrameBuffer(FrameBufferId::Scene);
	void* texID = (void*)fb->GetColorMap();

	// アスペクト比維持計算 
	float fbAspect = static_cast<float>(width) / static_cast<float>(height);
	float winAspect = availSize.x / availSize.y;

	// 描画サイズ初期値
	ImVec2 drawSize = availSize;

	if (fabsf(fbAspect - winAspect) > 0.01f)
	{
		if (winAspect > fbAspect)
		{
			// ウィンドウのほうが横長 → 高さ基準
			drawSize.y = availSize.y;
			drawSize.x = drawSize.y * fbAspect;
		}
		else
		{
			// ウィンドウのほうが縦長 → 幅基準
			drawSize.x = availSize.x;
			drawSize.y = drawSize.x / fbAspect;
		}
	}

	// 中央配置
	ImVec2 cursorPos = ImGui::GetCursorScreenPos();
	ImVec2 offset = {
		(availSize.x - drawSize.x) * 0.5f,
		(availSize.y - drawSize.y) * 0.5f
	};
	ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + offset.x, cursorPos.y + offset.y));

	// Image描画
	ImGui::Image(texID, drawSize, ImVec2(0, 0), ImVec2(1, 1));

	ImGui::End();

	// Debug Menuウィンドウを開始
	if (ImGui::Begin("Animation", nullptr, ImGuiWindowFlags_None))
	{
		//プレイヤー,敵キャラクターをリストに追加
		std::vector<Character*> characterList;

		// プレイヤーをリストに追加
		characterList.push_back(player.get());  // unique_ptr から生ポインタを取得して追加

		// 敵もリストに追加
		characterList.push_back(boss.get());

		// キャラクターの名前リストを作成
		std::vector<std::string> characterNames;
		characterNames.push_back("Player");
		characterNames.push_back("EnemyBoss");

		// ImGuiのコンボボックスでキャラクター選択
		static int selectedIndex = 0;
		if (ImGui::Combo("Target Character", &selectedIndex,
			[](void* data, int idx, const char** out_text) {
				auto& characterNames = *reinterpret_cast<std::vector<std::string>*>(data);
				*out_text = characterNames[idx].c_str();
				return true;
			},
			&characterNames, static_cast<int>(characterNames.size()))) {
			// 選択されたキャラクターを設定
			selectedCharacter = characterList[selectedIndex];
		}

		// プレイヤーのレンダリング状態
		playerRenderEnabled = (selectedCharacter == player.get());

		// 敵のレンダリング状態
		

		if (selectedCharacter->GetModel() != nullptr)
		{
			Model* model = selectedCharacter->GetModel();

			static bool isPlaying = true;
			static bool wasPlaying = isPlaying;
			static bool wasLooping = animationLoop;

			GamePad& gamePad = Input::Instance().GetGamePad();
			float ax = gamePad.GetAxisLX();
			float ay = gamePad.GetAxisLY();
			bool hasInput = std::abs(ax) > 0.1f || std::abs(ay) > 0.1f || gamePad.GetButton() != 0;

			static bool wasInput = false;
			static float inputReleaseTimer = 0.0f;
			const float returnDelay = 1.0f;

			// 一度だけ状態を保存したかどうか
			static bool hasSavedState = false;

			if (hasInput) {
				// 一度だけ保存
				if (!hasSavedState) {
					wasPlaying = isPlaying;
					wasLooping = animationLoop;
					hasSavedState = true;
				}

				// 入力中は強制再生
				isPlaying = true;
				animationLoop = true;

				// タイマーリセット
				inputReleaseTimer = 0.0f;
			}
			else {
				// 無入力中はタイマー進行
				inputReleaseTimer += ImGui::GetIO().DeltaTime;

				// 一定時間経過したら元の状態に戻す
				if (inputReleaseTimer >= returnDelay && hasSavedState) {
					isPlaying = wasPlaying;
					animationLoop = wasLooping;

					// 戻したら保存フラグをリセット
					hasSavedState = false;
					inputReleaseTimer = 0.0f;
				}
			}

			wasInput = hasInput;

			// ImGuiで手動切り替えも可能
			ImGui::Checkbox(u8"ループ", &animationLoop);

			animationSeconds = model->GetCurrentAnimationSeconds();

			//アニメーションの現在時間、長さを変数に代入
			const std::vector<ModelResource::Animation>& animations = model->GetResource()->GetAnimations();
			float secondsLength = animationIndex >= 0 ? animations.at(animationIndex).secondsLength : 0;
			int currentFrame = static_cast<int>(animationSeconds * 60.0f);
			int frameLength = static_cast<int>(secondsLength * 60);


			//再生ボタン停止ボタンの読み込みと描画
			DrawAnimationControlUI(model, isPlaying, animationIndex, animationLoop, animationSeconds, secondsLength);

			if (ImGui::Button("Save", { 50,30 }))
			{
				SaveAnimationConfigs(characterNames, characterList);
			}

			if (ImGui::Begin("AnimationGraph", nullptr, ImGuiWindowFlags_None))
			{
				int index = 0;
				static bool loder = true;
				for (const ModelResource::Animation& animation : animations)
				{
					if (animationIndex == index && animationIndex >= 0)
					{
						DrawAnimationEditorUI(animationIndex, model, characterNames, selectedIndex, secondsLength, animationSeconds);
					}
					index++;
				}
			}

			// イベントシーケンサーウィンドウ
			DrawEventSequencerWindow(elapsedTime);

			ImGui::End();

			SelectedAnimationGui(isPlaying);
		}

		// Debug Menuウィンドウを終了
	}
	ImGui::End();

	ImGui::GetStyle() = originalStyle; // スタイルを復元
}

void SceneEdit::SelectedAnimationGui(bool& isPlay)
{
	Model* model = selectedCharacter->GetModel();
	const std::vector<ModelResource::Animation>& animations = model->GetResource()->GetAnimations();
	int index = 0;
	for (const ModelResource::Animation& animation : animations)
	{
		ImGui::BeginChild("AnimationList", ImVec2(0, 200), true); // 高さ200のスクロール領域
		ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Leaf;

		if (animationIndex == index)
		{
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		ImGui::TreeNodeEx(&animation, nodeFlags, animation.name.c_str());

		// クリックでアニメーション再生
		if (ImGui::IsItemClicked())
		{
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				isPlay = true;
				model->PlayAnimation(index, animationLoop);
				animationIndex = index;
			}
		}

		ImGui::TreePop();
		ImGui::EndChild();
		index++;
	}
}

void SceneEdit::SaveAnimationConfigs(const std::vector<std::string>& characterNames, const std::vector<Character*>& characterList)
{
	json root;

	if (!selectedCharacter) return;

	std::string selectedName;

	for (size_t i = 0; i < characterList.size(); ++i) {
		if (characterList[i] != selectedCharacter) continue;

		selectedName = characterNames[i];
		Model* model = selectedCharacter->GetModel();
		const auto& animations = model->GetResource()->GetAnimations();

		for (size_t j = 0; j < animations.size(); ++j) {
			AnimationConfig* config = model->GetAnimationConfig(selectedName, static_cast<int>(j));
			if (!config) continue;

			if (!root.contains(selectedName)) {
				root[selectedName] = json::array();
			}

			root[selectedName].push_back(json(*config));
		}

		break;
	}

	if (selectedName.empty()) return; // 名前が見つからなければ保存しない

	// ファイル名をキャラ名付きに変更
	std::string filePath = "Data/Json/AnimationConfigs_" + selectedName + ".json";
	std::ofstream file(filePath);
	if (file.is_open()) {
		file << root.dump(4);
		file.close();
	}
}

void SceneEdit::ImGuiSetStyle()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	// 基本の背景色・フレーム色
	colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);

	// タイトルバー
	colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);

	// 枠線とか
	colors[ImGuiCol_Border] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);

	// ボタン・チェックボックス
	colors[ImGuiCol_Button] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

	// ヘッダータイトル
	colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.55f, 1.0f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.6f, 1.0f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);

	// スライダー、入力欄
	colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);

	// ドッキング対応
	colors[ImGuiCol_DockingPreview] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);

	// タブ（ドッキングウィンドウタイトル）
	colors[ImGuiCol_Tab] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	colors[ImGuiCol_TabActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

	// スクロールバー
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);

	// デバッグGUIウィンドウの位置とサイズ
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
}

void SceneEdit::DrawAnimationControlUI(Model* model, bool& isPlaying, int animationIndex, bool animationLoop, float& animationSeconds, float secondsLength)
{
	static float stoptime = 0.0f;

	// テクスチャの読み込み（初回のみ）
	static ID3D11ShaderResourceView* playTexture = nullptr;
	static ID3D11ShaderResourceView* stopTexture = nullptr;
	static bool texturesLoaded = false;

	if (!texturesLoaded)
	{
		ID3D11Device* device = Graphics::Instance().GetDevice();
		ID3D11DeviceContext* context = Graphics::Instance().GetDeviceContext();
		Microsoft::WRL::ComPtr<ID3D11Resource> resource;

		HRESULT hr = DirectX::CreateWICTextureFromFile(device, context, L"Data/Sprite/Stop.png", resource.GetAddressOf(), &stopTexture);
		if (FAILED(hr)) {
			// エラー処理
			stopTexture = nullptr;
		}
		
		hr = DirectX::CreateWICTextureFromFile(device, context, L"Data/Sprite/Play.png", resource.GetAddressOf(), &playTexture);
		if (FAILED(hr)) {
			// エラー処理
			playTexture = nullptr;
		}

		texturesLoaded = true;
	}

	ImVec2 size(32, 32);
	ImVec2 pos = ImGui::GetCursorScreenPos();

	// InvisibleButtonを配置
	if (ImGui::InvisibleButton("image_button", size)) {
		if (isPlaying) {
			stoptime = animationSeconds;
		}
		else {
			if (!model->IsPlayAnimation() && animationIndex >= 0) {
				model->PlayAnimation(animationIndex, animationLoop);
			}
		}
		isPlaying = !isPlaying;
	}

	// ボタン画像を表示
	if (isPlaying && stopTexture) {
		ImGui::GetWindowDrawList()->AddImage(stopTexture, pos, ImVec2(pos.x + size.x, pos.y + size.y));
	}
	else if (!isPlaying && playTexture) {
		ImGui::GetWindowDrawList()->AddImage(playTexture, pos, ImVec2(pos.x + size.x, pos.y + size.y));
	}

	ImGui::SameLine();

	// スライダーを表示
	if (ImGui::SliderFloat("AnimationSeconds", &animationSeconds, 0, secondsLength, "%.3f")) {
		model->SetCurrentAnimationSeconds(animationSeconds);
		stoptime = animationSeconds;
	}
	else if (!isPlaying) {
		model->SetCurrentAnimationSeconds(stoptime);
	}

	// アニメーション終了時の停止処理
	if (isPlaying && !model->IsPlayAnimation()) {
		isPlaying = false;
		stoptime = animationSeconds;
	}
}

void SceneEdit::DrawAnimationSpeedGraphBackground(ImDrawList* draw_list, ImVec2 graphStart, ImVec2 graphEnd, float secondsLength, float graphWidth, float graphHeight, float labelMargin)
{
	// 背景
	draw_list->AddRectFilled(graphStart, graphEnd, IM_COL32(60, 60, 100, 100));
	draw_list->AddRect(graphStart, graphEnd, IM_COL32(200, 200, 255, 255));

	// 0.1秒ごとに縦線を描画（実時間ベース）
	const float tickInterval = 0.1f;
	const int tickCount = static_cast<int>(secondsLength / tickInterval);

	for (int i = 0; i <= tickCount; ++i)
	{
		float time = i * tickInterval;
		float normalizedTime = time / secondsLength;
		float x = graphStart.x + normalizedTime * graphWidth;

		// 色を変える（0.5秒ごとにやや濃いグレー）
		ImU32 lineColor = (i % 5 == 0) ? IM_COL32(180, 180, 180, 150) : IM_COL32(120, 120, 120, 100);

		draw_list->AddLine(ImVec2(x, graphStart.y), ImVec2(x, graphEnd.y), lineColor);
	}

	const float speedTickInterval = 0.5f;
	const int speedTicks = static_cast<int>(3.0f / speedTickInterval);

	for (int i = 0; i <= speedTicks; ++i)
	{
		float value = i * speedTickInterval;
		float y = graphStart.y + (1.0f - value / 3.0f) * graphHeight;

		// 横線描画（0.0、1.5、3.0で色を変えるなども可能）
		ImU32 lineColor = (i % 2 == 0) ? IM_COL32(180, 180, 180, 150) : IM_COL32(120, 120, 120, 100);
		draw_list->AddLine(ImVec2(graphStart.x, y), ImVec2(graphEnd.x, y), lineColor);

		// 数値ラベル（グラフの左に表示）
		char label[16];
		snprintf(label, sizeof(label), "%.1f", value);
		draw_list->AddText(ImVec2(graphStart.x - labelMargin + 4, y - 6), IM_COL32(255, 255, 255, 200), label);
	}

	// マウスオーバー時に秒数表示
	ImVec2 mousePos = ImGui::GetMousePos();
	float mouseX = mousePos.x - graphStart.x; // グラフの左上からのX位置
	if (mouseX >= 0 && mouseX <= graphWidth)
	{
		float mouseTime = (mouseX / graphWidth) * secondsLength;

		ImGui::BeginTooltip();
		ImGui::Text("Time: %.2f s", mouseTime);
		ImGui::EndTooltip();
	}
}

void SceneEdit::DrawSpeedCurveEditor(AnimationConfig* config, ImDrawList* draw_list, ImVec2 graphStart, ImVec2 graphEnd, float graphWidth, float graphHeight, int& selectedKeyIndex, float secondsLength, Model* model)
{
	DrawSpeedCurvePoints(config, draw_list, graphStart, graphEnd, graphWidth, graphHeight, selectedKeyIndex);

	if (config->speedCurve.size() > 1)
	{
		DrawSpeedCurveLines(config, draw_list, graphStart, graphWidth, graphHeight);
		DrawCurrentSpeedIndicator(config, draw_list, graphStart, graphWidth, graphHeight, secondsLength, model);
	}
}

void SceneEdit::DrawSpeedCurvePoints(AnimationConfig* config, ImDrawList* draw_list, ImVec2 graphStart, ImVec2 graphEnd, float graphWidth, float graphHeight, int& selectedKeyIndex)
{
	// 点の描画 & 操作
	for (size_t i = 0; i < config->speedCurve.size(); ++i)
	{
		auto& kf = config->speedCurve[i];
		ImVec2 pt = ImVec2(
			graphStart.x + kf.time * graphWidth,
			graphStart.y + (1.0f - kf.value / 3.0f) * graphHeight
		);

		std::string label = "##pt" + std::to_string(i);
		ImGui::SetCursorScreenPos(ImVec2(pt.x - 6, pt.y - 6));
		ImGui::InvisibleButton(label.c_str(), ImVec2(12, 12));

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			selectedKeyIndex = static_cast<int>(i);
		}

		draw_list->AddCircleFilled(pt, 4.0f, IM_COL32(255, 255, 100, 255));

		if (selectedKeyIndex == static_cast<int>(i)) // 選択されたキーのみ処理
		{
			// ドラッグによる点の移動
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				ImVec2 delta = ImVec2(ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y);
				float dx = delta.x / graphWidth;
				float dy = -delta.y / graphHeight * 3.0f;
				kf.time += dx;
				kf.value += dy;

				// 必要なら制限
				kf.time = std::clamp(kf.time, 0.0f, 1.0f);
				kf.value = std::clamp(kf.value, 0.0f, 3.0f);
			}

			// ドラッグ中はハンドルを描画しない
			if (!ImGui::IsItemActive() || !ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				// ハンドル設定
				const float radius = 30.0f;
				ImVec2 leftHandle, rightHandle;

				if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
					// ドラッグ方向から仮ハンドル設定
					ImVec2 delta = ImVec2(ImGui::GetIO().MousePos.x - pt.x, ImGui::GetIO().MousePos.y - pt.y);
					float len = sqrtf(delta.x * delta.x + delta.y * delta.y);
					if (len > 1e-5f) {
						ImVec2 dir = ImVec2(delta.x / len, delta.y / len);
						leftHandle = ImVec2(pt.x - dir.x * radius, pt.y - dir.y * radius);
						rightHandle = ImVec2(pt.x + dir.x * radius, pt.y + dir.y * radius);
					}
				}
				else {
					// tangent に従ったハンドル位置
					auto tangentToHandle = [&](float tangent, bool isOut) -> ImVec2 {
						float dx = 1.0f;
						float dy = tangent * dx;
						ImVec2 dir = ImVec2(dx * graphWidth, -dy * graphHeight / 3.0f);
						float len = sqrtf(dir.x * dir.x + dir.y * dir.y);
						if (len > 1e-5f) {
							dir.x /= len;
							dir.y /= len;
						}
						else {
							dir = ImVec2(1.0f, 0.0f);
						}
						return ImVec2(pt.x + dir.x * radius * (isOut ? 1.0f : -1.0f),
							pt.y + dir.y * radius * (isOut ? 1.0f : -1.0f));
						};

					leftHandle = tangentToHandle(kf.inTangent, false);
					rightHandle = tangentToHandle(kf.outTangent, true);
				}

				auto DrawHandle = [&](ImVec2& handlePos, float& tangent, bool isOut)
					{
						draw_list->AddLine(pt, handlePos, IM_COL32(200, 100, 255, 255), 2.0f);
						draw_list->AddCircleFilled(handlePos, 3.0f, IM_COL32(255, 150, 255, 255));

						ImGui::SetCursorScreenPos(ImVec2(handlePos.x - 6, handlePos.y - 6));
						std::string handleLabel = isOut ? "##rightHandle" + std::to_string(i) : "##leftHandle" + std::to_string(i);
						ImGui::InvisibleButton(handleLabel.c_str(), ImVec2(12, 12));

						if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
							ImVec2 delta = ImVec2(ImGui::GetIO().MousePos.x - pt.x, ImGui::GetIO().MousePos.y - pt.y);
							float len = sqrtf(delta.x * delta.x + delta.y * delta.y);
							if (len > 1e-5f) {
								ImVec2 dir = ImVec2(delta.x / len, delta.y / len);
								handlePos = ImVec2(pt.x + dir.x * radius, pt.y + dir.y * radius);

								// tangent 更新
								float dx = dir.x / graphWidth;
								float dy = -dir.y / graphHeight * 3.0f;
								float newTangent = (fabsf(dx) > 1e-5f) ? (dy / dx) : 0.0f;

								kf.outTangent = newTangent;
								kf.inTangent = newTangent;
							}
						}
					};

				DrawHandle(leftHandle, kf.inTangent, false);
				DrawHandle(rightHandle, kf.outTangent, true);
			}
		}
	}
}

void SceneEdit::DrawSpeedCurveLines(AnimationConfig* config, ImDrawList* draw_list, ImVec2 graphStart, float graphWidth, float graphHeight)
{
	for (size_t i = 0; i < config->speedCurve.size() - 1; ++i)
	{
		const auto& k0 = config->speedCurve[i];
		const auto& k1 = config->speedCurve[i + 1];

		float t0 = k0.time;
		float v0 = k0.value;
		float t1 = k1.time;
		float v1 = k1.value;

		float dt = t1 - t0;

		// スケーリングされたin/outタンジェント
		float m0 = k0.outTangent * dt;
		float m1 = k1.inTangent * dt;

		const int segments = 32; // 曲線の分割数（多めにして滑らかに）
		ImVec2 prev = ImVec2(
			graphStart.x + t0 * graphWidth,
			graphStart.y + (1.0f - v0 / 3.0f) * graphHeight
		);

		for (int j = 1; j <= segments; ++j)
		{
			float u = (float)j / (float)segments;
			float u2 = u * u;
			float u3 = u2 * u;

			float h00 = 2 * u3 - 3 * u2 + 1;
			float h10 = u3 - 2 * u2 + u;
			float h01 = -2 * u3 + 3 * u2;
			float h11 = u3 - u2;

			float interpV = h00 * v0 + h10 * m0 + h01 * v1 + h11 * m1;
			float interpT = t0 + u * dt;

			ImVec2 current = ImVec2(
				graphStart.x + interpT * graphWidth,
				graphStart.y + (1.0f - interpV / 3.0f) * graphHeight
			);

			draw_list->AddLine(prev, current, IM_COL32(255, 255, 100, 255), 2.0f);
			prev = current;
		}
	}
}

void SceneEdit::DrawCurrentSpeedIndicator(AnimationConfig* config, ImDrawList* draw_list, ImVec2 graphStart, float graphWidth, float graphHeight, float secondsLength, Model* model)
{
	// 現在のアニメーション時間に基づくスピードを計算
	float t = animationSeconds / secondsLength;  // アニメーション時間を[0, 1]にクランプ
	t = std::clamp(t, 0.0f, 1.0f);  // 念のため 0.0～1.0 にクランプ

	// SpeedをEvaluateSpeedを使って取得
	float speed = model->EvaluateSpeed(config->speedCurve, t);

	// Y座標は補完されたスピードに基づいて計算
	float speedNormalized = (1.0f - speed / 3.0f);  // スピードを0?1の範囲に正規化

	// X座標はアニメーションの現在の時間に基づく
	ImVec2 current = ImVec2(
		graphStart.x + animationSeconds / secondsLength * graphWidth,  // X座標（現在のアニメーション時間に基づく）
		graphStart.y + speedNormalized * graphHeight  // Y座標（スピードに基づいて）
	);

	// 現在の点を描画
	draw_list->AddCircleFilled(current, 6.0f, IM_COL32(255, 0, 0, 255));  // 赤い点
}

void SceneEdit::DrawAttributeHandles(ImDrawList* draw_list, AnimationConfig* config, int animIndex, const ImVec2& graphStart, const ImVec2& graphEnd, float graphWidth, float secondsLength)
{
	for (size_t attrIndex = 0; attrIndex < config->attributes.size(); ++attrIndex)
	{
		auto& attr = config->attributes[attrIndex];

		if (attr.flag == AnimationFlag::None)
			continue;

		float startX = graphStart.x + (attr.startTime / secondsLength) * graphWidth;
		float endX = graphStart.x + (attr.endTime / secondsLength) * graphWidth;

		ImVec2 startTop = ImVec2(startX, graphStart.y);
		ImVec2 startBottom = ImVec2(startX, graphEnd.y + 10);
		ImVec2 endTop = ImVec2(endX, graphStart.y);
		ImVec2 endBottom = ImVec2(endX, graphEnd.y + 10);

		ImU32 fillColor, lineColor, circleColor;

		switch (attr.flag) {
		case AnimationFlag::Attack:
			fillColor = IM_COL32(255, 100, 100, 60);
			lineColor = IM_COL32(255, 100, 100, 200);
			circleColor = IM_COL32(255, 150, 150, 255);
			break;
		case AnimationFlag::Invincible:
			fillColor = IM_COL32(100, 255, 100, 60);
			lineColor = IM_COL32(100, 255, 100, 200);
			circleColor = IM_COL32(150, 255, 150, 255);
			break;
		case AnimationFlag::Guard:
			fillColor = IM_COL32(100, 100, 255, 60);
			lineColor = IM_COL32(100, 100, 255, 200);
			circleColor = IM_COL32(150, 150, 255, 255);
			break;
		case AnimationFlag::SuperArmor:
			fillColor = IM_COL32(255, 255, 100, 60);
			lineColor = IM_COL32(255, 255, 100, 200);
			circleColor = IM_COL32(255, 255, 150, 255);
			break;
		default:
			fillColor = IM_COL32(200, 200, 200, 40);
			lineColor = IM_COL32(200, 200, 200, 100);
			circleColor = IM_COL32(180, 180, 180, 180);
			break;
		}

		draw_list->AddRectFilled(ImVec2(startX, graphStart.y), ImVec2(endX, graphEnd.y), fillColor);
		draw_list->AddLine(startTop, startBottom, lineColor, 2.0f);
		draw_list->AddLine(endTop, endBottom, lineColor, 2.0f);
		draw_list->AddCircleFilled(startBottom, 6.0f, circleColor);
		draw_list->AddCircleFilled(endBottom, 6.0f, circleColor);

		std::string startId = "startAttr##" + std::to_string(animIndex) + "_" + std::to_string(attrIndex);
		ImGui::SetCursorScreenPos(ImVec2(startBottom.x - 6, startBottom.y - 6));
		ImGui::InvisibleButton(startId.c_str(), ImVec2(12, 12));
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			float delta = ImGui::GetIO().MouseDelta.x / graphWidth * secondsLength;
			attr.startTime += delta;
			attr.startTime = std::clamp(attr.startTime, 0.0f, attr.endTime);
		}

		std::string endId = "endAttr##" + std::to_string(animIndex) + "_" + std::to_string(attrIndex);
		ImGui::SetCursorScreenPos(ImVec2(endBottom.x - 6, endBottom.y - 6));
		ImGui::InvisibleButton(endId.c_str(), ImVec2(12, 12));
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			float delta = ImGui::GetIO().MouseDelta.x / graphWidth * secondsLength;
			attr.endTime += delta;
			attr.endTime = std::clamp(attr.endTime, attr.startTime, secondsLength);
		}

		// 攻撃属性の場合、AttackParamのハンドルも表示（この属性自身のattackParamを使用）
		if (attr.flag == AnimationFlag::Attack)
		{
			const float handleOffset = 12.0f;
			AttackAnimParam& param = attr.attackParam; // config->attackParam から attr.attackParam に変更

			ImU32 attackFill = IM_COL32(180, 100, 255, 60);
			ImU32 attackLine = IM_COL32(180, 100, 255, 200);
			ImU32 attackCircle = IM_COL32(200, 150, 255, 255);

			float atkStartX = graphStart.x + (config->advanceInputStartFrame / secondsLength) * graphWidth;
			float atkEndX = graphStart.x + (config->advanceInputEndFrame / secondsLength) * graphWidth;

			ImVec2 atkStartTop = ImVec2(atkStartX, graphStart.y - handleOffset);
			ImVec2 atkStartBottom = ImVec2(atkStartX, graphEnd.y + handleOffset);
			ImVec2 atkEndTop = ImVec2(atkEndX, graphStart.y - handleOffset);
			ImVec2 atkEndBottom = ImVec2(atkEndX, graphEnd.y + handleOffset);

			draw_list->AddRectFilled(ImVec2(atkStartX, graphStart.y), ImVec2(atkEndX, graphEnd.y), attackFill);

			draw_list->AddLine(ImVec2(atkStartBottom.x, graphEnd.y), atkStartBottom, attackLine, 2.0f);
			draw_list->AddLine(ImVec2(atkEndBottom.x, graphEnd.y), atkEndBottom, attackLine, 2.0f);
			draw_list->AddCircleFilled(atkStartBottom, 6.0f, attackCircle);
			draw_list->AddCircleFilled(atkEndBottom, 6.0f, attackCircle);

			std::string atkStartIdBtm = "atkStartBtm##" + std::to_string(animIndex) + "_" + std::to_string(attrIndex);
			ImGui::SetCursorScreenPos(ImVec2(atkStartBottom.x - 6, atkStartBottom.y - 6));
			ImGui::InvisibleButton(atkStartIdBtm.c_str(), ImVec2(12, 12));
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				float delta = ImGui::GetIO().MouseDelta.x / graphWidth * secondsLength;
				config->advanceInputStartFrame += delta;
				config->advanceInputStartFrame = std::clamp(config->advanceInputStartFrame, 0.0f, config->advanceInputEndFrame);
			}

			std::string atkEndIdBtm = "atkEndBtm##" + std::to_string(animIndex) + "_" + std::to_string(attrIndex);
			ImGui::SetCursorScreenPos(ImVec2(atkEndBottom.x - 6, atkEndBottom.y - 6));
			ImGui::InvisibleButton(atkEndIdBtm.c_str(), ImVec2(12, 12));
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				float delta = ImGui::GetIO().MouseDelta.x / graphWidth * secondsLength;
				config->advanceInputEndFrame += delta;
				config->advanceInputEndFrame = std::clamp(config->advanceInputEndFrame, config->advanceInputStartFrame, secondsLength);
			}
		}
	}
}

void SceneEdit::DrawAnimationEventsUI(AnimationConfig* config, Model* model, int animationIndex)
{
	for (size_t i = 0; i < config->events.size(); ++i)
	{
		AnimationEvent& evt = config->events[i];
		ImGui::PushID(static_cast<int>(i));

		int type = static_cast<int>(evt.eventType);
		if (ImGui::Combo("Type", &type, "Camera\0Effect\0\0")) {
			evt.eventType = static_cast<EventType>(type);
		}

		// string → char[64] で安全に編集
		char buf[64] = {};
		strncpy_s(buf, sizeof(buf), evt.eventName.c_str(), _TRUNCATE);

		buf[sizeof(buf) - 1] = '\0'; // 念のため終端

		if (ImGui::InputText("Event Name", buf, sizeof(buf))) {
			evt.eventName = std::string(buf); // ゴミのない文字列で代入
		}

		if (ImGui::Button("Delete Event")) {
			config->events.erase(config->events.begin() + i);
			ImGui::PopID();
			break;
		}

		ImGui::Separator();
		ImGui::PopID();
	}
}

void SceneEdit::DrawEventHandles(ImDrawList* draw_list, AnimationConfig* config, int animationIndex, const ImVec2& graphStart, const ImVec2& graphEnd, float graphWidth, float secondsLength)
{
	for (size_t i = 0; i < config->events.size(); ++i)
	{
		AnimationEvent& evt = config->events[i];

		float startX = graphStart.x + (evt.timeInSeconds / secondsLength) * graphWidth;
		float endX = graphStart.x + (evt.timeOutSeconds / secondsLength) * graphWidth;

		ImVec2 startTop = ImVec2(startX, graphStart.y);
		ImVec2 startBottom = ImVec2(startX, graphEnd.y + 10);
		ImVec2 endTop = ImVec2(endX, graphStart.y);
		ImVec2 endBottom = ImVec2(endX, graphEnd.y + 10);

		ImU32 fillColor, lineColor, circleColor;

		switch (evt.eventType) {
		case EventType::Camera:
			fillColor = IM_COL32(255, 255, 100, 60);
			lineColor = IM_COL32(255, 255, 100, 200);
			circleColor = IM_COL32(255, 255, 150, 255);
			break;
		case EventType::Effect:
			fillColor = IM_COL32(100, 255, 255, 60);
			lineColor = IM_COL32(100, 255, 255, 200);
			circleColor = IM_COL32(150, 255, 255, 255);
			break;
		default:
			fillColor = IM_COL32(200, 200, 200, 40);
			lineColor = IM_COL32(200, 200, 200, 100);
			circleColor = IM_COL32(180, 180, 180, 180);
			break;
		}

		// 塗りつぶし矩形（イベント期間）
		draw_list->AddRectFilled(ImVec2(startX, graphStart.y), ImVec2(endX, graphEnd.y), fillColor);

		// 両端ラインとハンドル
		draw_list->AddLine(startTop, startBottom, lineColor, 2.0f);
		draw_list->AddLine(endTop, endBottom, lineColor, 2.0f);
		draw_list->AddCircleFilled(startBottom, 6.0f, circleColor);
		draw_list->AddCircleFilled(endBottom, 6.0f, circleColor);

		// 開始ハンドル
		std::string startId = "eventStart##" + std::to_string(animationIndex) + "_" + std::to_string(i);
		ImGui::SetCursorScreenPos(ImVec2(startBottom.x - 6, startBottom.y - 6));
		ImGui::InvisibleButton(startId.c_str(), ImVec2(12, 12));
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			float delta = ImGui::GetIO().MouseDelta.x / graphWidth * secondsLength;
			evt.timeInSeconds += delta;
			evt.timeInSeconds = std::clamp(evt.timeInSeconds, 0.0f, evt.timeOutSeconds);
		}

		// 終了ハンドル
		std::string endId = "eventEnd##" + std::to_string(animationIndex) + "_" + std::to_string(i);
		ImGui::SetCursorScreenPos(ImVec2(endBottom.x - 6, endBottom.y - 6));
		ImGui::InvisibleButton(endId.c_str(), ImVec2(12, 12));
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			float delta = ImGui::GetIO().MouseDelta.x / graphWidth * secondsLength;
			evt.timeOutSeconds += delta;
			evt.timeOutSeconds = std::clamp(evt.timeOutSeconds, evt.timeInSeconds, secondsLength);
		}
	}
}

void SceneEdit::DrawAnimationEditorUI(int animationIndex, Model* model, const std::vector<std::string>& characterNames, int selectedIndex, float secondsLength, float animationSeconds)
{
	std::string characterName = characterNames[selectedIndex];

	static bool loder = true;
	AnimationConfig* config = nullptr;
	if (loder)
	{
		config = AnimationConfigLoader::GetConfig(characterName, animationIndex);
		if (!config)
			config = model->GetAnimationConfig(characterName, animationIndex);
	}
	else
	{
		config = model->GetAnimationConfig(characterName, animationIndex);
	}

	if (!config) return;

	DrawSpeedCurveUI(config, secondsLength, animationSeconds, model);
	DrawAnimationEventsUI(config, model, animationIndex);

	model->SetAnimationConfig(*config);
}

void SceneEdit::DrawSpeedCurveUI(AnimationConfig* config, float secondsLength, float animationSeconds, Model* model)
{
	static float graphWidth = 800.0f;
	static float graphHeight = 200.0f;
	const float labelMargin = 40.0f;
	static int selectedKeyIndex = -1;
	static int selectedCameraKeyIndex = -1;

	ImVec2 graphStart = ImGui::GetCursorScreenPos();
	graphStart.x += labelMargin;
	ImVec2 graphEnd = ImVec2(graphStart.x + graphWidth, graphStart.y + graphHeight);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	// グラフ背景描画
	DrawAnimationSpeedGraphBackground(draw_list, graphStart, graphEnd, secondsLength, graphWidth, graphHeight, labelMargin);

	// グラフサイズ変更用ドラッグハンドル
	ImVec2 resizeHandleSize = ImVec2(10, 10);
	ImVec2 resizeHandlePos = ImVec2(
		graphEnd.x - resizeHandleSize.x,
		graphEnd.y - resizeHandleSize.y
	);

	ImGui::SetCursorScreenPos(resizeHandlePos);
	ImGui::InvisibleButton("ResizeHandle", resizeHandleSize, ImGuiButtonFlags_None);

	if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		ImVec2 delta = ImGui::GetIO().MouseDelta;
		graphWidth = max(100.0f, graphWidth + delta.x);
		graphHeight = max(50.0f, graphHeight + delta.y);
	}

	// ドラッグハンドルの可視化（小さな三角形）
	draw_list->AddTriangleFilled(
		{ resizeHandlePos.x, resizeHandlePos.y + resizeHandleSize.y },
		{ resizeHandlePos.x + resizeHandleSize.x, resizeHandlePos.y + resizeHandleSize.y },
		{ resizeHandlePos.x + resizeHandleSize.x, resizeHandlePos.y },
		IM_COL32(200, 200, 200, 255)
	);

	// グラフの描画後
	ImVec2 currentCursorPos = ImGui::GetCursorScreenPos();

	// グラフの終わった位置より少し右にカーソルを移動
	float spacing = 20.0f; // グラフとの隙間
	ImVec2 rightUIPos = ImVec2(graphEnd.x + spacing, graphStart.y);  // グラフと同じ高さから始める
	ImGui::SetCursorScreenPos(rightUIPos);

	DrawSpeedCurveEditor(config, draw_list, graphStart, graphEnd, graphWidth, graphHeight, selectedKeyIndex, secondsLength, model);
	DrawCameraKeyframePoints(config, draw_list, graphStart, graphEnd, graphWidth, graphHeight, selectedCameraKeyIndex);

	// DrawAttributeHandles を animationIndex 付きで呼び出す
	DrawAttributeHandles(draw_list, config, animationIndex, graphStart, graphEnd, graphWidth, secondsLength);
	DrawEventHandles(draw_list, config, animationIndex, graphStart, graphEnd, graphWidth, secondsLength);

	// イベントシーケンサー追加
	ImGui::Dummy(ImVec2(0, 10)); // 少し余白

	// 右クリック処理
	HandleSpeedCurveRightClick(config, graphStart, graphEnd, graphWidth, graphHeight, selectedKeyIndex);

	// 現在のスピードを反映
	float t = animationSeconds / secondsLength;
	t = std::clamp(t, 0.0f, 1.0f);
	float speed = model->EvaluateSpeed(config->speedCurve, t);
	model->SetAnimationSpeed(speed);
}

void SceneEdit::HandleSpeedCurveRightClick(AnimationConfig* config, ImVec2 graphStart, ImVec2 graphEnd, float graphWidth, float graphHeight, int& selectedKeyIndex)
{
	static ImVec2 rightClickPos; // 右クリック位置を保持

	if (ImGui::IsMouseHoveringRect(graphStart, graphEnd) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
		rightClickPos = ImGui::GetMousePos(); // 右クリック時点の位置を記録
		ImGui::OpenPopup("RightClickMenu");
	}

	if (ImGui::BeginPopup("RightClickMenu"))
	{
		float normTime = (rightClickPos.x - graphStart.x) / graphWidth;
		float normValue = 1.0f - (rightClickPos.y - graphStart.y) / graphHeight;
		normTime = std::clamp(normTime, 0.0f, 1.0f);
		normValue = std::clamp(normValue * 3.0f, 0.0f, 3.0f);

		if (ImGui::MenuItem("Add Speed Curve Keyframe"))
		{
			Keyframe newKey;
			newKey.time = normTime;
			newKey.value = normValue;
			newKey.handleOffsetX = 0.05f;
			newKey.handleOffsetY = 0.0f;
			newKey.inTangent = 0.0f;
			newKey.outTangent = 0.0f;

			config->speedCurve.push_back(newKey);
			std::sort(config->speedCurve.begin(), config->speedCurve.end(),
				[](const Keyframe& a, const Keyframe& b) {
					return a.time < b.time;
				});

			for (size_t i = 0; i < config->speedCurve.size(); ++i) {
				const Keyframe& k = config->speedCurve[i];
				if (std::abs(k.time - newKey.time) < 1e-5f && std::abs(k.value - newKey.value) < 1e-5f) {
					selectedKeyIndex = static_cast<int>(i);
					break;
				}
			}
		}

		if (ImGui::MenuItem("Add Camera Keyframe"))
		{
			CameraKeyframe camKey;
			camKey.time = normTime;
			camKey.range = cameraController->GetRange();
			camKey.eyeOffset = cameraController->GetDirection();
			camKey.targetOffset = cameraController->GetOffsetTarget();
			camKey.savedYaw = selectedCharacter->GetAngle().y;
			config->cameraKeyframes.push_back(camKey);
			std::sort(config->cameraKeyframes.begin(), config->cameraKeyframes.end(),
				[](const CameraKeyframe& a, const CameraKeyframe& b) {
					return a.time < b.time;
				});
		}

		ImGui::EndPopup();
	}
}

// ========================================
// イベントシーケンサーのメインウィンドウ
// ========================================
void SceneEdit::DrawEventSequencerWindow(float elapsedTime)
{
	if (!selectedCharacter || !selectedCharacter->GetModel()) return;

	Model* model = selectedCharacter->GetModel();
	const std::vector<ModelResource::Animation>& animations = model->GetResource()->GetAnimations();

	if (animationIndex < 0 || animationIndex >= animations.size()) return;

	float secondsLength = animations[animationIndex].secondsLength;

	// キャラクター名取得
	std::vector<std::string> characterNames = { "Player", "EnemyBoss" };
	std::vector<Character*> characterList = { player.get(), boss.get() };
	int selectedIndex = 0;
	for (size_t i = 0; i < characterList.size(); ++i) {
		if (characterList[i] == selectedCharacter) {
			selectedIndex = static_cast<int>(i);
			break;
		}
	}
	std::string characterName = characterNames[selectedIndex];

	AnimationConfig* config = model->GetAnimationConfig(characterName, animationIndex);
	if (!config) return;

	// ウィンドウ表示
	ImGui::SetNextWindowSize(ImVec2(1200, 600), ImGuiCond_FirstUseEver);
	if (ImGui::Begin(u8"イベントシーケンサー", nullptr, ImGuiWindowFlags_None))
	{
		ImGui::TextColored(ImVec4(1, 0.9f, 0.3f, 1), u8"アニメーション: %s", animations[animationIndex].name.c_str());
		ImGui::Text(u8"長さ: %.2f秒 (%dフレーム)", secondsLength, static_cast<int>(secondsLength * 60.0f));
		ImGui::Separator();

		// タイムライン描画
		DrawSequencerTimeline(config, secondsLength, animationSeconds, model, sequencerState.selectedEventIndex, sequencerState.currentFrame);

		ImGui::Spacing();
		ImGui::Separator();

		// 2列レイアウト
		ImGui::BeginChild("LeftPanel", ImVec2(350, 0), true);
		{
			// イベントリスト
			DrawSequencerEventList(config, sequencerState.selectedEventIndex);

			ImGui::Separator();

			// Attributeリスト
			DrawAttributeListSection(config, sequencerState.selectedAttributeIndex);
		}
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
		{
			// 選択中のイベント編集パネル
			if (sequencerState.selectedEventIndex >= 0 &&
				sequencerState.selectedEventIndex < static_cast<int>(config->events.size()))
			{
				DrawEventEditPanel(config, sequencerState.selectedEventIndex, secondsLength);
			}
			else if (sequencerState.selectedAttributeIndex >= 0 &&
				sequencerState.selectedAttributeIndex < static_cast<int>(config->attributes.size()))
			{
				// Attribute編集
				DrawAttributeEditPanel(config, sequencerState.selectedAttributeIndex, secondsLength);
			}
			else
			{
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1), u8"イベントまたは属性を選択してください");
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}


// ========================================
// タイムライン描画
// ========================================
void SceneEdit::DrawSequencerTimeline(AnimationConfig* config, float secondsLength, float& animationSeconds, Model* model, int& selectedEventIndex, int& currentFrame)
{
	const float timelineHeight = 300.0f; // 高さを増やす
	const float trackHeight = 30.0f;
	const float timelineMargin = 50.0f;
	const float handleSize = 8.0f; // ドラッグハンドルのサイズ

	ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImVec2(ImGui::GetContentRegionAvail().x, timelineHeight);
	float timelineWidth = canvasSize.x - timelineMargin;

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// 背景
	drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
		IM_COL32(40, 40, 50, 255));

	// 時間軸の目盛り描画
	const float pixelsPerSecond = timelineWidth / secondsLength;
	const float tickInterval = 0.5f;
	const int tickCount = static_cast<int>(secondsLength / tickInterval) + 1;

	for (int i = 0; i < tickCount; ++i)
	{
		float time = i * tickInterval;
		float x = canvasPos.x + timelineMargin + time * pixelsPerSecond;

		ImU32 lineColor = (i % 2 == 0) ? IM_COL32(100, 100, 120, 255) : IM_COL32(70, 70, 80, 255);
		drawList->AddLine(ImVec2(x, canvasPos.y), ImVec2(x, canvasPos.y + canvasSize.y), lineColor);

		if (i % 2 == 0)
		{
			char label[16];
			snprintf(label, sizeof(label), "%.1fs", time);
			drawList->AddText(ImVec2(x - 15, canvasPos.y + 5), IM_COL32(200, 200, 200, 255), label);
		}
	}

	float trackY = canvasPos.y + 30;

	// ======================================
	// イベントトラック描画
	// ======================================
	ImGui::PushID("Events");
	for (size_t i = 0; i < config->events.size(); ++i)
	{
		auto& evt = config->events[i];

		float startX = canvasPos.x + timelineMargin + evt.timeInSeconds * pixelsPerSecond;
		float endX = canvasPos.x + timelineMargin + evt.timeOutSeconds * pixelsPerSecond;
		float width = endX - startX;

		ImVec2 rectMin = ImVec2(startX, trackY);
		ImVec2 rectMax = ImVec2(endX, trackY + trackHeight);

		// イベントタイプによって色を変える
		ImU32 fillColor, borderColor;
		if (evt.eventType == EventType::Camera)
		{
			fillColor = IM_COL32(80, 150, 220, 200);
			borderColor = IM_COL32(100, 180, 255, 255);
		}
		else
		{
			fillColor = IM_COL32(220, 150, 80, 200);
			borderColor = IM_COL32(255, 180, 100, 255);
		}

		// 選択中は強調
		if (selectedEventIndex == static_cast<int>(i))
		{
			fillColor = IM_COL32(255, 255, 100, 220);
			borderColor = IM_COL32(255, 255, 150, 255);
		}

		// 矩形描画
		drawList->AddRectFilled(rectMin, rectMax, fillColor, 3.0f);
		drawList->AddRect(rectMin, rectMax, borderColor, 3.0f, 0, 2.0f);

		// イベント名表示
		const char* label = evt.eventName.c_str();
		ImVec2 textSize = ImGui::CalcTextSize(label);
		if (textSize.x < width - 10)
		{
			drawList->AddText(ImVec2(startX + 5, trackY + 7), IM_COL32(255, 255, 255, 255), label);
		}

		// 左端ハンドル（開始時間）
		ImVec2 leftHandleMin = ImVec2(startX - handleSize / 2, trackY);
		ImVec2 leftHandleMax = ImVec2(startX + handleSize / 2, trackY + trackHeight);
		drawList->AddRectFilled(leftHandleMin, leftHandleMax, IM_COL32(255, 255, 255, 200));

		ImGui::SetCursorScreenPos(leftHandleMin);
		ImGui::InvisibleButton(("evtStartHandle##" + std::to_string(i)).c_str(), ImVec2(handleSize, trackHeight));
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float deltaX = ImGui::GetIO().MouseDelta.x;
			float deltaTime = deltaX / pixelsPerSecond;
			evt.timeInSeconds += deltaTime;
			evt.timeInSeconds = std::clamp(evt.timeInSeconds, 0.0f, evt.timeOutSeconds - 0.05f);
		}

		// 右端ハンドル（終了時間）
		ImVec2 rightHandleMin = ImVec2(endX - handleSize / 2, trackY);
		ImVec2 rightHandleMax = ImVec2(endX + handleSize / 2, trackY + trackHeight);
		drawList->AddRectFilled(rightHandleMin, rightHandleMax, IM_COL32(255, 255, 255, 200));

		ImGui::SetCursorScreenPos(rightHandleMin);
		ImGui::InvisibleButton(("evtEndHandle##" + std::to_string(i)).c_str(), ImVec2(handleSize, trackHeight));
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float deltaX = ImGui::GetIO().MouseDelta.x;
			float deltaTime = deltaX / pixelsPerSecond;
			evt.timeOutSeconds += deltaTime;
			evt.timeOutSeconds = std::clamp(evt.timeOutSeconds, evt.timeInSeconds + 0.05f, secondsLength);
		}

		// 中央部分のクリック・ドラッグ判定
		ImVec2 centerMin = ImVec2(startX + handleSize, trackY);
		ImVec2 centerMax = ImVec2(endX - handleSize, trackY + trackHeight);

		ImGui::SetCursorScreenPos(centerMin);
		ImGui::InvisibleButton(("evt##" + std::to_string(i)).c_str(), ImVec2(width - handleSize * 2, trackHeight));

		if (ImGui::IsItemClicked())
		{
			selectedEventIndex = static_cast<int>(i);
			sequencerState.selectedAttributeIndex = -1; // Attribute選択をクリア
		}

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float deltaX = ImGui::GetIO().MouseDelta.x;
			float deltaTime = deltaX / pixelsPerSecond;

			float newStart = evt.timeInSeconds + deltaTime;
			float newEnd = evt.timeOutSeconds + deltaTime;

			if (newStart >= 0.0f && newEnd <= secondsLength)
			{
				evt.timeInSeconds = newStart;
				evt.timeOutSeconds = newEnd;
			}
		}

		trackY += trackHeight + 5;
	}
	ImGui::PopID();

	// ======================================
	// Attributeトラック描画
	// ======================================
	ImGui::PushID("Attributes");
	for (size_t i = 0; i < config->attributes.size(); ++i)
	{
		auto& attr = config->attributes[i];

		float startX = canvasPos.x + timelineMargin + attr.startTime * pixelsPerSecond;
		float endX = canvasPos.x + timelineMargin + attr.endTime * pixelsPerSecond;
		float width = endX - startX;

		ImVec2 rectMin = ImVec2(startX, trackY);
		ImVec2 rectMax = ImVec2(endX, trackY + trackHeight);

		// Attributeタイプによって色を変える
		ImU32 fillColor, borderColor;
		switch (attr.flag)
		{
		case AnimationFlag::Attack:
			fillColor = IM_COL32(220, 80, 80, 200);
			borderColor = IM_COL32(255, 100, 100, 255);
			break;
		case AnimationFlag::Invincible:
			fillColor = IM_COL32(80, 220, 80, 200);
			borderColor = IM_COL32(100, 255, 100, 255);
			break;
		case AnimationFlag::Guard:
			fillColor = IM_COL32(80, 80, 220, 200);
			borderColor = IM_COL32(100, 100, 255, 255);
			break;
		case AnimationFlag::SuperArmor:
			fillColor = IM_COL32(220, 220, 80, 200);
			borderColor = IM_COL32(255, 255, 100, 255);
			break;
		default:
			fillColor = IM_COL32(150, 150, 150, 200);
			borderColor = IM_COL32(180, 180, 180, 255);
			break;
		}

		// 選択中は強調
		if (sequencerState.selectedAttributeIndex == static_cast<int>(i))
		{
			fillColor = IM_COL32(255, 255, 100, 220);
			borderColor = IM_COL32(255, 255, 150, 255);
		}

		drawList->AddRectFilled(rectMin, rectMax, fillColor, 3.0f);
		drawList->AddRect(rectMin, rectMax, borderColor, 3.0f, 0, 2.0f);

		// ラベル
		const char* attrLabel = "";
		switch (attr.flag)
		{
		case AnimationFlag::Attack: attrLabel = u8"攻撃"; break;
		case AnimationFlag::Invincible: attrLabel = u8"無敵"; break;
		case AnimationFlag::Guard: attrLabel = u8"ガード"; break;
		case AnimationFlag::SuperArmor: attrLabel = u8"スーパーアーマー"; break;
		default: attrLabel = u8"無し"; break;
		}
		drawList->AddText(ImVec2(startX + 5, trackY + 7), IM_COL32(255, 255, 255, 255), attrLabel);

		// 左端ハンドル（開始時間）
		ImVec2 leftHandleMin = ImVec2(startX - handleSize / 2, trackY);
		ImVec2 leftHandleMax = ImVec2(startX + handleSize / 2, trackY + trackHeight);
		drawList->AddRectFilled(leftHandleMin, leftHandleMax, IM_COL32(255, 255, 255, 200));

		ImGui::SetCursorScreenPos(leftHandleMin);
		ImGui::InvisibleButton(("attrStartHandle##" + std::to_string(i)).c_str(), ImVec2(handleSize, trackHeight));
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float deltaX = ImGui::GetIO().MouseDelta.x;
			float deltaTime = deltaX / pixelsPerSecond;
			attr.startTime += deltaTime;
			attr.startTime = std::clamp(attr.startTime, 0.0f, attr.endTime - 0.05f);
		}

		// 右端ハンドル（終了時間）
		ImVec2 rightHandleMin = ImVec2(endX - handleSize / 2, trackY);
		ImVec2 rightHandleMax = ImVec2(endX + handleSize / 2, trackY + trackHeight);
		drawList->AddRectFilled(rightHandleMin, rightHandleMax, IM_COL32(255, 255, 255, 200));

		ImGui::SetCursorScreenPos(rightHandleMin);
		ImGui::InvisibleButton(("attrEndHandle##" + std::to_string(i)).c_str(), ImVec2(handleSize, trackHeight));
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float deltaX = ImGui::GetIO().MouseDelta.x;
			float deltaTime = deltaX / pixelsPerSecond;
			attr.endTime += deltaTime;
			attr.endTime = std::clamp(attr.endTime, attr.startTime + 0.05f, secondsLength);
		}

		// 中央部分のクリック・ドラッグ判定
		ImVec2 centerMin = ImVec2(startX + handleSize, trackY);
		ImVec2 centerMax = ImVec2(endX - handleSize, trackY + trackHeight);

		ImGui::SetCursorScreenPos(centerMin);
		ImGui::InvisibleButton(("attr##" + std::to_string(i)).c_str(), ImVec2(width - handleSize * 2, trackHeight));

		if (ImGui::IsItemClicked())
		{
			sequencerState.selectedAttributeIndex = static_cast<int>(i);
			selectedEventIndex = -1; // イベント選択をクリア
		}

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			float deltaX = ImGui::GetIO().MouseDelta.x;
			float deltaTime = deltaX / pixelsPerSecond;

			float newStart = attr.startTime + deltaTime;
			float newEnd = attr.endTime + deltaTime;

			if (newStart >= 0.0f && newEnd <= secondsLength)
			{
				attr.startTime = newStart;
				attr.endTime = newEnd;
			}
		}

		trackY += trackHeight + 5;
	}
	ImGui::PopID();

	// 現在時間のインジケータ
	float currentX = canvasPos.x + timelineMargin + animationSeconds * pixelsPerSecond;
	drawList->AddLine(ImVec2(currentX, canvasPos.y),
		ImVec2(currentX, canvasPos.y + canvasSize.y),
		IM_COL32(255, 0, 0, 255), 2.0f);
	drawList->AddCircleFilled(ImVec2(currentX, canvasPos.y + 10), 5.0f, IM_COL32(255, 0, 0, 255));

	// タイムラインクリックで時間移動
	ImGui::SetCursorScreenPos(canvasPos);
	ImGui::InvisibleButton("timeline", canvasSize);
	if (ImGui::IsItemClicked())
	{
		float clickX = ImGui::GetMousePos().x - canvasPos.x - timelineMargin;
		float clickTime = clickX / pixelsPerSecond;
		clickTime = std::clamp(clickTime, 0.0f, secondsLength);
		animationSeconds = clickTime;
		model->SetCurrentAnimationSeconds(clickTime);
	}

	ImGui::SetCursorScreenPos(ImVec2(canvasPos.x, canvasPos.y + canvasSize.y));
	ImGui::Dummy(ImVec2(canvasSize.x, 0));
}

// ========================================
// イベントリスト描画
// ========================================
void SceneEdit::DrawSequencerEventList(AnimationConfig* config, int selectedEventIndex)
{
	ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1), u8"イベントリスト");
	ImGui::Separator();

	// イベント追加ボタン
	if (ImGui::Button(u8"+ カメライベント追加", ImVec2(-1, 0)))
	{
		AnimationEvent newEvent;
		newEvent.eventType = EventType::Camera;
		newEvent.eventName = "NewCamera";
		newEvent.timeInSeconds = 0.0f;
		newEvent.timeOutSeconds = 0.5f;
		config->events.push_back(newEvent);
	}

	if (ImGui::Button(u8"+ エフェクトイベント追加", ImVec2(-1, 0)))
	{
		AnimationEvent newEvent;
		newEvent.eventType = EventType::Effect;
		newEvent.eventName = "NewEffect";
		newEvent.timeInSeconds = 0.0f;
		newEvent.timeOutSeconds = 0.5f;
		config->events.push_back(newEvent);
	}

	ImGui::Separator();

	// イベント一覧
	for (size_t i = 0; i < config->events.size(); ++i)
	{
		auto& evt = config->events[i];

		bool isSelected = (selectedEventIndex == static_cast<int>(i));

		ImGui::PushID(static_cast<int>(i));

		// アイコンと名前
		const char* icon = (evt.eventType == EventType::Camera) ? u8"📷" : u8"✨";
		char label[128];
		snprintf(label, sizeof(label), "%s %s", icon, evt.eventName.c_str());

		if (ImGui::Selectable(label, isSelected))
		{
			sequencerState.selectedEventIndex = static_cast<int>(i);
		}

		// 時間情報
		ImGui::SameLine(200);
		ImGui::TextDisabled("%.2f-%.2f", evt.timeInSeconds, evt.timeOutSeconds);

		ImGui::PopID();
	}
}

// ========================================
// イベント編集パネル
// ========================================
void SceneEdit::DrawEventEditPanel(AnimationConfig* config, int selectedEventIndex, float secondsLength)
{
	if (selectedEventIndex < 0 || selectedEventIndex >= config->events.size()) return;

	auto& evt = config->events[selectedEventIndex];

	ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), u8"イベント編集");
	ImGui::Separator();

	// イベント名
	char nameBuf[128];
	strncpy_s(nameBuf, sizeof(nameBuf), evt.eventName.c_str(), _TRUNCATE);
	if (ImGui::InputText(u8"イベント名", nameBuf, sizeof(nameBuf)))
	{
		evt.eventName = nameBuf;
	}

	// イベントタイプ
	const char* typeNames[] = { u8"カメラ", u8"エフェクト" };
	int typeIndex = (evt.eventType == EventType::Camera) ? 0 : 1;
	if (ImGui::Combo(u8"タイプ", &typeIndex, typeNames, 2))
	{
		evt.eventType = (typeIndex == 0) ? EventType::Camera : EventType::Effect;
	}

	ImGui::Spacing();

	// 時間設定
	ImGui::Text(u8"時間設定");
	ImGui::DragFloat(u8"開始時間(秒)", &evt.timeInSeconds, 0.01f, 0.0f, evt.timeOutSeconds);
	ImGui::DragFloat(u8"終了時間(秒)", &evt.timeOutSeconds, 0.01f, evt.timeInSeconds, secondsLength);

	int startFrame = static_cast<int>(evt.timeInSeconds * 60.0f);
	int endFrame = static_cast<int>(evt.timeOutSeconds * 60.0f);
	ImGui::Text(u8"フレーム: %d - %d", startFrame, endFrame);

	ImGui::Spacing();
	ImGui::Separator();

	// 削除ボタン
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
	if (ImGui::Button(u8"このイベントを削除", ImVec2(-1, 0)))
	{
		config->events.erase(config->events.begin() + selectedEventIndex);
		sequencerState.selectedEventIndex = -1;
	}
	ImGui::PopStyleColor();
}

// ========================================
// Attributeリストセクション
// ========================================
void SceneEdit::DrawAttributeListSection(AnimationConfig* config, int& selectedAttributeIndex)
{
	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1), u8"属性リスト");
	ImGui::Separator();

	// Attribute追加ボタン
	if (ImGui::Button(u8"+ 攻撃属性追加", ImVec2(-1, 0)))
	{
		AnimationAttribute newAttr;
		newAttr.flag = AnimationFlag::Attack;
		newAttr.startTime = 0.0f;
		newAttr.endTime = 0.5f;
		config->attributes.push_back(newAttr);
	}

	if (ImGui::Button(u8"+ 無敵属性追加", ImVec2(-1, 0)))
	{
		AnimationAttribute newAttr;
		newAttr.flag = AnimationFlag::Invincible;
		newAttr.startTime = 0.0f;
		newAttr.endTime = 0.5f;
		config->attributes.push_back(newAttr);
	}

	if (ImGui::Button(u8"+ ガード属性追加", ImVec2(-1, 0)))
	{
		AnimationAttribute newAttr;
		newAttr.flag = AnimationFlag::Guard;
		newAttr.startTime = 0.0f;
		newAttr.endTime = 0.5f;
		config->attributes.push_back(newAttr);
	}

	if (ImGui::Button(u8"+ スーパーアーマー追加", ImVec2(-1, 0)))
	{
		AnimationAttribute newAttr;
		newAttr.flag = AnimationFlag::SuperArmor;
		newAttr.startTime = 0.0f;
		newAttr.endTime = 0.5f;
		config->attributes.push_back(newAttr);
	}

	ImGui::Separator();

	// Attribute一覧
	for (size_t i = 0; i < config->attributes.size(); ++i)
	{
		auto& attr = config->attributes[i];

		bool isSelected = (selectedAttributeIndex == static_cast<int>(i));

		ImGui::PushID(static_cast<int>(i));

		// アイコンと名前
		const char* icon = "";
		const char* name = "";
		switch (attr.flag)
		{
		case AnimationFlag::Attack:
			icon = u8"⚔️";
			name = u8"攻撃";
			break;
		case AnimationFlag::Invincible:
			icon = u8"🛡️";
			name = u8"無敵";
			break;
		case AnimationFlag::Guard:
			icon = u8"⚡";
			name = u8"ガード";
			break;
		case AnimationFlag::SuperArmor:
			icon = u8"💪";
			name = u8"スーパーアーマー";
			break;
		default:
			icon = u8"❓";
			name = u8"無し";
			break;
		}

		char label[128];
		snprintf(label, sizeof(label), "%s %s", icon, name);

		if (ImGui::Selectable(label, isSelected))
		{
			selectedAttributeIndex = static_cast<int>(i);
			sequencerState.selectedEventIndex = -1; // イベント選択をクリア
		}

		// 時間情報
		ImGui::SameLine(200);
		ImGui::TextDisabled("%.2f-%.2f", attr.startTime, attr.endTime);

		ImGui::PopID();
	}
}

// ========================================
// Attribute編集パネル（修正版）
// ========================================
void SceneEdit::DrawAttributeEditPanel(AnimationConfig* config, int selectedAttributeIndex, float secondsLength)
{
	if (selectedAttributeIndex < 0 || selectedAttributeIndex >= config->attributes.size()) return;

	auto& attr = config->attributes[selectedAttributeIndex];

	ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), u8"属性編集 #%d", selectedAttributeIndex);
	ImGui::Separator();

	// 属性タイプ
	const char* flagNames[] = {
		u8"無し",
		u8"攻撃",
		u8"無敵",
		u8"ガード",
		u8"スーパーアーマー"
	};
	int flagIndex = static_cast<int>(attr.flag);
	if (ImGui::Combo(u8"属性タイプ", &flagIndex, flagNames, 5))
	{
		attr.flag = static_cast<AnimationFlag>(flagIndex);
	}

	ImGui::Spacing();

	// 時間設定
	ImGui::Text(u8"時間設定");
	ImGui::DragFloat(u8"開始時間(秒)", &attr.startTime, 0.01f, 0.0f, attr.endTime);
	ImGui::DragFloat(u8"終了時間(秒)", &attr.endTime, 0.01f, attr.startTime, secondsLength);
	ImGui::DragFloat(u8"先行入力受付開始", &config->advanceInputStartFrame, 0.01f, attr.startTime, secondsLength);
	ImGui::DragFloat(u8"先行入力受付終了", &config->advanceInputEndFrame, 0.01f, attr.startTime, secondsLength);

	int startFrame = static_cast<int>(attr.startTime * 60.0f);
	int endFrame = static_cast<int>(attr.endTime * 60.0f);
	ImGui::Text(u8"フレーム: %d - %d", startFrame, endFrame);

	// 攻撃属性の場合、詳細パラメータ（この属性自身のattackParamを使用）
	if (attr.flag == AnimationFlag::Attack)
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), u8"攻撃パラメータ");

		auto& ap = attr.attackParam; // config->attackParam から attr.attackParam に変更

		const char* flagNames[] = {
		u8"なし",
		u8"弱",
		u8"強",
		u8"打ち上げ",
		u8"撃ち落とし",
		};
		int knockbackType = static_cast<int>(ap.knockbackType);
		ImGui::Spacing();
		ImGui::Separator();
		if (ImGui::Combo(u8"ノックバックタイプ", &knockbackType, flagNames, 5))
		{
			ap.knockbackType = static_cast<KnockbackType>(knockbackType);
		}

		ImGui::DragFloat(u8"移動倍率", &ap.moveRate, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat(u8"回転倍率", &ap.turnRate, 0.01f, 0.0f, 5.0f);

		ImGui::Spacing();

		ImGui::DragInt(u8"攻撃ダメージ", &ap.attackDamage, 1, 0, 9999);
		ImGui::DragFloat(u8"無敵時間", &ap.invisibleTime, 0.01f, 0.0f, 10.0f);
		ImGui::DragInt(u8"リベンジ値蓄積量", &ap.revengeValue, 1, 0, 10);

		ImGui::Spacing();

		ImGui::Checkbox(u8"前方移動あり", &ap.forwarded);
		if (ap.forwarded)
		{
			ImGui::Indent();
			ImGui::DragFloat(u8"前方移動力", &ap.forwardPower, 0.1f);
			ImGui::DragFloat(u8"前方移動フレーム", &ap.forwardFrame, 0.5f);
			ImGui::Unindent();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text(u8"コントローラー振動");
		ImGui::DragFloat(u8"左振動", &ap.attackLeftVibrate, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat(u8"右振動", &ap.attackRightVibrate, 0.01f, 0.0f, 1.0f);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Text(u8"ヒットストップ");
		ImGui::DragFloat(u8"ヒットストップ時間", &ap.attackHitStopTime, 0.01f, 0.0f, 5.0f);
		ImGui::DragFloat(u8"ヒットストップ速度", &ap.attackHitStopSpeed, 0.01f, 0.0f, 1.0f);
	}

	ImGui::Spacing();
	ImGui::Separator();

	// 削除ボタン
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
	if (ImGui::Button(u8"この属性を削除", ImVec2(-1, 0)))
	{
		config->attributes.erase(config->attributes.begin() + selectedAttributeIndex);
		sequencerState.selectedAttributeIndex = -1;
	}
	ImGui::PopStyleColor();
}

void SceneEdit::DrawCameraKeyframePoints(AnimationConfig* config, ImDrawList* draw_list,
	ImVec2 graphStart, ImVec2 graphEnd,
	float graphWidth, float graphHeight,
	int& selectedKeyIndex)
{
	for (size_t i = 0; i < config->cameraKeyframes.size(); ++i)
	{
		auto& kf = config->cameraKeyframes[i];

		ImVec2 pt = ImVec2(
			graphStart.x + kf.time * graphWidth,
			graphStart.y + graphHeight * 0.5f
		);

		std::string label = "##campt" + std::to_string(i);
		ImGui::SetCursorScreenPos(ImVec2(pt.x - 6, pt.y - 6));
		ImGui::InvisibleButton(label.c_str(), ImVec2(12, 12));

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
			selectedKeyIndex = static_cast<int>(i);

			// キーをカメラに反映
			cameraController->SetRange(kf.range);
			cameraController->SetOffsetTarget(kf.targetOffset.value);

			// 方向ベクトル → 角度（ピッチ・ヨー）変換して設定
			DirectX::XMFLOAT3 angle = DirectionToEuler(kf.eyeOffset.value);
			cameraController->SetAngle(angle);
		}

		// 色分け（選択中なら強調）
		ImU32 color = (selectedKeyIndex == static_cast<int>(i))
			? IM_COL32(150, 255, 255, 255)  // 明るいシアン
			: IM_COL32(100, 255, 255, 255); // 通常のシアン

		draw_list->AddCircleFilled(pt, 4.0f, color);

		float lineLength = graphHeight * 0.5f;
		draw_list->AddLine(ImVec2(pt.x, pt.y - lineLength),
			ImVec2(pt.x, pt.y + lineLength),
			color, 2.0f);
		
		// 選択中なら設定UIとドラッグ操作
		if (selectedKeyIndex == static_cast<int>(i))
		{
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
				float dx = ImGui::GetIO().MouseDelta.x / graphWidth;
				kf.time += dx;
				kf.time = std::clamp(kf.time, 0.0f, 1.0f);
			}

			if (ImGui::Begin("Camera Keyframe Settings", nullptr, ImGuiWindowFlags_None))
			{
				ImGui::Text("Keyframe %d", selectedKeyIndex);
				ImGui::DragFloat("Range", &kf.range, 0.1f, 0.0f, 50.0f);
				ImGui::DragFloat3("Target Offset", &kf.targetOffset.value.x, 0.1f);
				ImGui::DragFloat3("Eye Offset", &kf.eyeOffset.value.x, 0.1f);
			}
			ImGui::End();
		}
	}
}



