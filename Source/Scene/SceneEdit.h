#pragma once

#include "Stage/Stage.h"
#include "Character/player.h"

#include "Character/Enemy/SilverDragonkin.h"
#include "Scene.h"
#include "Graphics/SkyBox.h"
#include "Graphics/PostEffect.h"
#include "Camera/EditCameraController.h"
#include "Camera/FreeCameraController.h"
//#include "BattleUI/CombatUIManager.h"

// ゲームシーン
class SceneEdit : public Scene
{
public:
	SceneEdit() {}
	~SceneEdit() override {}

	// 初期化
	void Initialize() override;

	// 終了化
	void Finalize() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render(float elapsedTime) override {};
	void Render(float elapsedTime, int width, int height);

	// デバッグ用GUI描画
	void DrawDebugGUI(float elapsedTime);

	void SelectedAnimationGui(bool& isPlay);

	void SaveAnimationConfigs(
		const std::vector<std::string>& characterNames,
		const std::vector<Character*>& characterList);

	void ImGuiSetStyle();

	void DrawAnimationControlUI(Model* model, bool& isPlaying, int animationIndex, bool animationLoop, float& animationSeconds, float secondsLength);

	void DrawAnimationSpeedGraphBackground(ImDrawList* draw_list, ImVec2 graphStart, ImVec2 graphEnd, float secondsLength, float graphWidth, float graphHeight, float labelMargin);

	void DrawSpeedCurveEditor(AnimationConfig* config, ImDrawList* draw_list, ImVec2 graphStart, ImVec2 graphEnd, float graphWidth, float graphHeight, int& selectedKeyIndex, float secondsLength, Model* model);

	void DrawSpeedCurvePoints(AnimationConfig* config, ImDrawList* draw_list, ImVec2 graphStart, ImVec2 graphEnd, float graphWidth, float graphHeight, int& selectedKeyIndex);

	void DrawSpeedCurveLines(AnimationConfig* config, ImDrawList* draw_list, ImVec2 graphStart, float graphWidth, float graphHeight);

	void DrawCurrentSpeedIndicator(AnimationConfig* config, ImDrawList* draw_list, ImVec2 graphStart, float graphWidth, float graphHeight, float secondsLength, Model* model);

	void DrawAttributeHandles(ImDrawList* draw_list, AnimationConfig* config, int animIndex, const ImVec2& graphStart, const ImVec2& graphEnd, float graphWidth, float secondsLength);

	void DrawAnimationEventsUI(AnimationConfig* config, Model* model, int animationIndex);

	void DrawAnimationEditorUI(int animationIndex, Model* model, const std::vector<std::string>& characterNames, int selectedIndex, float secondsLength, float animationSeconds);
	void DrawSpeedCurveUI(AnimationConfig* config, float secondsLength, float animationSeconds, Model* model);
	void HandleSpeedCurveRightClick(AnimationConfig* config, ImVec2 graphStart, ImVec2 graphEnd, float graphWidth, float graphHeight, int& selectedKeyIndex);

	// イベントシーケンサー関連の関数
	void DrawEventSequencerWindow(float elapsedTime);
	void DrawSequencerTimeline(AnimationConfig* config, float secondsLength, float& animationSeconds, Model* model, int& selectedEventIndex, int& currentFrame);
	void DrawSequencerEventList(AnimationConfig* config, int selectedEventIndex);
	void DrawEventEditPanel(AnimationConfig* config, int selectedEventIndex, float secondsLength);
	void DrawAttributeEditPanel(AnimationConfig* config, int selectedAttributeIndex, float secondsLength);
	void DrawAttributeListSection(AnimationConfig* config, int& selectedAttributeIndex);
	void HandleSequencerInput(AnimationConfig* config, float secondsLength, float& animationSeconds, int& selectedEventIndex, int& currentFrame, float timelineWidth);

	void DrawCameraKeyframePoints(AnimationConfig* config, ImDrawList* draw_list, ImVec2 graphStart, ImVec2 graphEnd, float graphWidth, float graphHeight, int& selectedKeyIndex);

	void DrawEventHandles(ImDrawList* draw_list, AnimationConfig* config, int animationIndex, const ImVec2& graphStart, const ImVec2& graphEnd, float graphWidth, float secondsLength);
private:
	std::unique_ptr<Player> player;
	std::unique_ptr<SilverDragonkin> dragonkin;

	std::unique_ptr<EditCameraController> cameraController;
	std::unique_ptr<SkyBox> skyBox;
	FreeCameraController freecameraController;
	Model::DissolveConstants enemysupport;

	std::unique_ptr<PostEffect> posteffect;

	// 点光源
	DirectX::XMFLOAT4 pointColor{ 0.0f / 255.0f, 50.0f / 255.0f, 255.0f / 255.0f, 0.0f / 255.0f };
	DirectX::XMFLOAT3 offsetPosition{ 0.0f, 3.0f, 0.0f };
	float attenuation = 4.5f;
	DirectX::XMFLOAT4 direction = { -1, -1, 0 ,1.0f };
	DirectX::XMFLOAT4 Directioncolor = { 1, 1, 1 ,1.0f };

	//タイマー
	float timer;

	Character* selectedCharacter = nullptr;
	bool playerRenderEnabled = true;

	int									animationIndex = -1;
	float								animationSeconds = 0.0f;
	std::unordered_map<int, float>      animationSpeed;
	bool								animationLoop = false;

	// イベントシーケンサー用の状態変数
	struct SequencerState
	{
		int currentFrame = 0;
		int selectedEventIndex = -1;
		int selectedAttributeIndex = -1; // 追加
		bool isDragging = false;
		bool isDraggingStart = false;
		bool isDraggingEnd = false;
		int draggedEventIndex = -1;
		int draggedAttributeIndex = -1; // 追加
		float timelineScrollX = 0.0f;
		float zoom = 1.0f;
	};
	SequencerState sequencerState;
};