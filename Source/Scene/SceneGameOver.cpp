#include "SceneGameOver.h"
#include "Graphics/Graphics.h"
#include "SceneTitle.h"
#include "SceneGame.h"
#include "SceneLoading.h"
#include "SceneManager.h"
#include "Input/Input.h"
#include "Graphics/GpuResourceUtils.h"
#include <map>

// メモリリーク検出用

#include <stdlib.h>



void SceneGameOver::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// スプライト初期化
	// 全て1920x1080のサイズと仮定
	Back = std::make_unique<Sprite>(device, "Data/Sprite/Title_Back.png"); // ファイル名は適宜調整してください
	Retry = std::make_unique<Sprite>(device, "Data/Sprite/Retry.png");
	Retry1 = std::make_unique<Sprite>(device, "Data/Sprite/Retry1.png");
	BackTitle = std::make_unique<Sprite>(device, "Data/Sprite/BackTitle.png");
	BackTitle1 = std::make_unique<Sprite>(device, "Data/Sprite/BackTitle1.png");

	// 初期状態はリトライ選択
	isRetrySelected = true;
	timer = 0.0f;
}

void SceneGameOver::Finalize()
{
	// スマートポインタを使用しているため明示的なdeleteは不要
}

void SceneGameOver::Update(float elapsedTime)
{
	timer += elapsedTime;

	// 遷移直後の誤入力を防ぐため、少し待機時間を設ける（任意）
	if (timer < 0.5f) return;

	GamePad& gamePad = Input::Instance().GetGamePad();
	Mouse& mouse = Input::Instance().GetMouse();

	// 上下左右キーで選択切り替え
	if (gamePad.GetButtonDown() & GamePad::BTN_UP ||
		gamePad.GetButtonDown() & GamePad::BTN_DOWN)
	{
		isRetrySelected = !isRetrySelected;
	}

	// 決定ボタン（Aボタン または A_EMU）
	if (gamePad.GetButtonDown() & GamePad::BTN_A || gamePad.GetButtonDown() & GamePad::BTN_A_EMU)
	{
		if (isRetrySelected)
		{
			// リトライ：SceneGameへ (Loadingを挟む)
			SceneManager::Instance().ChangeScene(new SceneLoading(new SceneGame()));
		}
		else
		{
			// タイトルへ：SceneTitleへ (Loadingを挟む)
			SceneManager::Instance().ChangeScene(new SceneLoading(new SceneTitle()));
		}
	}
}

void SceneGameOver::Render(float elapsedTime)
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();

	// 画面クリア＆レンダーターゲット設定
	DirectX::XMFLOAT4 color = { 0.0f, 0.0f, 0.0f, 1.0f }; // ゲームオーバーなので黒背景クリア等が一般的
	std::map<FrameBufferId, FrameBuffer*> buffers;
	for (int i = 0; i < static_cast<int>(FrameBufferId::EnumCount); i++)
	{
		buffers[static_cast<FrameBufferId>(i)] = Graphics::Instance().GetFrameBuffer(static_cast<FrameBufferId>(i));
		buffers[static_cast<FrameBufferId>(i)]->Clear(dc, color);
	}
	buffers[FrameBufferId::Display]->SetRenderTargets(dc);

	RenderState* renderState = graphics.GetRenderState();

	// サンプラ設定
	ID3D11SamplerState* samplers[] =
	{
		renderState->GetSamplerState(SamplerState::LinearClamp) // 拡大縮小が綺麗になるようLinear推奨
	};
	dc->PSSetSamplers(0, _countof(samplers), samplers);

	// ブレンドステート設定（透過処理有効）
	FLOAT blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	UINT sampleMask = 0xFFFFFFFF;
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), blendFactor, sampleMask);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// 画面サイズ取得
	const float screenW = graphics.GetScreenWidth();
	const float screenH = graphics.GetScreenHeight();

	// --- 描画実行 ---

	// 背景描画
	if (Back)
	{
		Back->Render(dc, 0, 0, 0, screenW, screenH, 0, 0, 1920, 1080, 0, 1, 1, 1, 1);
	}

	// 選択肢の描画
	if (isRetrySelected)
	{
		// リトライが選択されている場合
		// Retry1 (選択中:ハイライト版) を描画
		if (Retry1) Retry1->Render(dc, 0, 0, 0, screenW, screenH, 0, 0, 1920, 1080, 0, 1, 1, 1, 1);

		// BackTitle (未選択:通常版) を描画
		if (BackTitle) BackTitle->Render(dc, 0, 0, 0, screenW, screenH, 0, 0, 1920, 1080, 0, 1, 1, 1, 1);
	}
	else
	{
		// タイトルへ戻るが選択されている場合
		// Retry (未選択:通常版) を描画
		if (Retry) Retry->Render(dc, 0, 0, 0, screenW, screenH, 0, 0, 1920, 1080, 0, 1, 1, 1, 1);

		// BackTitle1 (選択中:ハイライト版) を描画
		if (BackTitle1) BackTitle1->Render(dc, 0, 0, 0, screenW, screenH, 0, 0, 1920, 1080, 0, 1, 1, 1, 1);
	}

#ifdef _DEBUG
	DrawDebugGUI();
#endif
}

void SceneGameOver::DrawDebugGUI()
{
	ImGui::Begin("Debug SceneGameOver");
	ImGui::Text("Retry Selected: %s", isRetrySelected ? "True" : "False");
	ImGui::End();
}