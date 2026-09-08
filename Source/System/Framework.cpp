#include <memory>
#include <sstream>
#include <imgui.h>

#include "Framework.h"

#include "Audio/Audio.h"
#include "Graphics/Graphics.h"
#include "Debug/ImGuiRenderer.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneTitle.h"
#include "Scene/SceneGame.h"
#include "Scene/SceneClear.h"
#include "Scene/SceneEdit.h"
#include "Scene/SceneLoading.h"
#include "System/AnimationConfigLoader.h"
#include "Effect/EffectManager.h"

// 垂直同期間隔設定
static const int syncInterval = 1;

// コンストラクタ
Framework::Framework(HWND hWnd)
	: hWnd(hWnd)
	, input(hWnd)
{
#if !_DEBUG
	ShowCursor(false);
#endif

	// グラフィックス初期化
	Graphics::Instance().Initialize(hWnd);

	Audio::Instance().Initialize();

	// IMGUI初期化
	ImGuiRenderer::Initialize(hWnd, Graphics::Instance().GetDevice(), Graphics::Instance().GetDeviceContext());

	EffectManager::Instance().Initialize();
	AnimationConfigLoader::LoadAllConfigs();

	// シーン初期化
	SceneManager::Instance().ChangeScene(new SceneTitle);
}

// デストラクタ
Framework::~Framework()
{
	ShowCursor(true);

	SceneManager::Instance().Clear();

	Audio::Instance().Finalize();

	// IMGUI終了化
	ImGuiRenderer::Finalize();

	EffectManager::Instance().Finalize();
}

// 更新処理
void Framework::Update(float elapsedTime)
{
	input.Update();

#ifndef _DEBUG
	if (GetForegroundWindow() == hWnd) // 自分のウィンドウがアクティブな時のみ
	{
		RECT rect;
		GetClientRect(hWnd, &rect);
		// ウィンドウの中央座標を計算
		POINT center = { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
		// スクリーン座標に変換
		ClientToScreen(hWnd, &center);
		// カーソル位置を強制的に中央へセット
		SetCursorPos(center.x, center.y);
	}
#endif

	// シーン更新処理
	SceneManager::Instance().Update(elapsedTime);
	input.OnKeyUp();
}

// 描画処理
void Framework::Render(float elapsedTime)
{
	std::lock_guard<std::mutex>lock(Graphics::Instance().GetMutex());
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// IMGUIフレーム開始処理	
	ImGuiRenderer::NewFrame();

	// 画面クリア
	Graphics::Instance().GetFrameBuffer(FrameBufferId::Display)->Clear(dc, DirectX::XMFLOAT4(0, 0, 1, 1));

	// レンダーターゲット設定
	Graphics::Instance().GetFrameBuffer(FrameBufferId::Display)->SetRenderTargets(dc);

	// シーン描画処理
	if (currentSceneType == SceneType::Edit)
	{
		// エディットシーンは画面解像度のまま描き、Game View パネル側で縮小表示する。
		// パネルに合わせて解像度を変えると、スプライトや UI の配置が崩れるため。
		const int width = static_cast<int>(Graphics::Instance().GetScreenWidth());
		const int height = static_cast<int>(Graphics::Instance().GetScreenHeight());

		// ローディング中などは SceneEdit ではないので、通常の描画に任せる
		auto* editScene = dynamic_cast<SceneEdit*>(SceneManager::Instance().GetCurrentScene());
		if (editScene != nullptr)
		{
			editScene->Render(elapsedTime, width, height);
		}
		else
		{
			SceneManager::Instance().Render(elapsedTime);
		}
	}
	else
	{
		// 通常通り描画
		SceneManager::Instance().Render(elapsedTime);
	}

	// グローバルまたは静的変数として管理（必要に応じて）
	static bool showSceneSelector = false;

	ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f - 20, 0), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.3f); // ちょっと透明にする

	// Scene Selector ウィンドウ（表示中のみ）
#if _DEBUG || DEBUG
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Scene Selector"))
		{
			if (ImGui::MenuItem("Title Scene")) {
				currentSceneType = SceneType::Title;
				SceneManager::Instance().ChangeScene(new SceneLoading(new SceneTitle()));
				ResizeSceneFramebufferToWindow();
			}

			if (ImGui::MenuItem("Game Scene")) {
				currentSceneType = SceneType::Game;
				SceneManager::Instance().ChangeScene(new SceneLoading(new SceneGame()));
				ResizeSceneFramebufferToWindow();
			}

			if (ImGui::MenuItem("Result Scene")) {
				currentSceneType = SceneType::Result;
				SceneManager::Instance().ChangeScene(new SceneLoading(new SceneClear()));
				ResizeSceneFramebufferToWindow();
			}

			if (ImGui::MenuItem("Edit Scene")) {
				currentSceneType = SceneType::Edit;
				SceneManager::Instance().ChangeScene(new SceneLoading(new SceneEdit()));
				ResizeSceneFramebufferToWindow();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
#endif
	
#if 0
	// IMGUIデモウインドウ描画（IMGUI機能テスト用）
	ImGui::ShowDemoWindow();
#endif
	// IMGUI描画
	ImGuiRenderer::Render(dc);

	// 画面表示
	Graphics::Instance().Present(syncInterval);
}

template<class T>
void Framework::ChangeSceneButtonGUI(const char* name)
{
	if (ImGui::Button(name))
	{
		scene = std::make_unique<T>();
	}
}

// シーン切り替えGUI
void Framework::SceneSelectGUI()
{
	ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	ImVec2 pos = ImGui::GetMainViewport()->GetWorkPos();
	float width = 210;
	float height = 490;
	ImGui::SetNextWindowPos(ImVec2(pos.x + displaySize.x - width - 10, pos.y + 10), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Once);
}

// フレームレート計算
void Framework::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
	static int frames = 0;
	static float time_tlapsed = 0.0f;

	frames++;

	// Compute averages over one second period.
	if ((timer.TimeStamp() - time_tlapsed) >= 1.0f)
	{
		float fps = static_cast<float>(frames); // fps = frameCnt / 1
		float mspf = 1000.0f / fps;
		std::ostringstream outs;
		outs.precision(6);
		outs << "FPS : " << fps << " / " << "Frame Time : " << mspf << " (ms)";
		SetWindowTextA(hWnd, outs.str().c_str());

		// Reset for next average.
		frames = 0;
		time_tlapsed += 1.0f;
	}
}

void Framework::ResizeSceneFramebufferToWindow()
{
	ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
	int fbWidth = static_cast<int>(viewportSize.x);
	int fbHeight = static_cast<int>(viewportSize.y);
	Graphics::Instance().ResizeFrameBuffer(FrameBufferId::Scene, fbWidth, fbHeight);
}

// アプリケーションループ
int Framework::Run()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timer.Tick();
			CalculateFrameStats();

			float elapsedTime = timer.TimeInterval();
			Update(elapsedTime);
			Render(elapsedTime);
		}
	}
	return static_cast<int>(msg.wParam);
}

// メッセージハンドラ
LRESULT CALLBACK Framework::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGuiRenderer::HandleMessage(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		break;
	case WM_KEYDOWN:
		//if (wParam == VK_ESCAPE) PostMessage(hWnd, WM_CLOSE, 0, 0);
		input.OnKeyDown();
		input.SetIsLastGamePad(false);
		break;
	case WM_ENTERSIZEMOVE:
		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
		timer.Stop();
		break;
	case WM_EXITSIZEMOVE:
		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
		timer.Start();
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
