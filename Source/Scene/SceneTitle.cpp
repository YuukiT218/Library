#include "Graphics/Graphics.h"
#include "SceneTitle.h"
#include "SceneGame.h"
#include "SceneManager.h"
#include "Input/Input.h"
#include "SceneLoading.h"
#include "Math/Easing.h"
#include "Graphics/GpuResourceUtils.h"
#include "Math/Mathf.h"
#include <stdlib.h>
#include "System/Audio/Audio.h"

namespace
{
	// BGM
	constexpr const char* BGM_PATH = "Data/Sound/BGM/Dearly in Dreams.wav";
	constexpr float BGM_VOLUME = 0.5f;

	// 画面クリア色
	const DirectX::XMFLOAT4 CLEAR_COLOR = { 0.2f, 0.2f, 0.2f, 1.0f };

	// タイトル画面の1枚絵を描く大きさ（横は画面より少し広く描いている）
	constexpr float TITLE_DRAW_WIDTH = 1980.0f;
	constexpr float TITLE_DRAW_HEIGHT = 1080.0f;

	// 「何かボタンを押してください」画像の大きさ
	constexpr float ANY_BUTTON_TEXTURE_WIDTH = 1920.0f;
	constexpr float ANY_BUTTON_TEXTURE_HEIGHT = 1080.0f;

	// タイトルロゴの描画サイズと画像サイズ
	constexpr float LOGO_DRAW_WIDTH = 1228.0f;
	constexpr float LOGO_DRAW_HEIGHT = 819.0f;
	constexpr float LOGO_TEXTURE_WIDTH = 1536.0f;
	constexpr float LOGO_TEXTURE_HEIGHT = 1024.0f;

	// 終了確認ダイアログの画像サイズ
	constexpr float END_DIALOG_WIDTH = 1280.0f;
	constexpr float END_DIALOG_HEIGHT = 720.0f;

	// 「はい」を選んでいるときの選択カーソルの位置
	constexpr float YES_CURSOR_OFFSET_X = -285.0f;

	// ロゴがこの不透明度になるまではボタン入力を受け付けない
	constexpr float START_INPUT_ALPHA_THRESHOLD = 0.3f;

	// ロゴと「何かボタンを押してください」を出し始める時間
	constexpr float LOGO_FADE_START_SECONDS = 1.0f;
	constexpr float ANY_BUTTON_BLINK_START_SECONDS = 3.0f;

	// ロゴがフェードインする速さ
	constexpr float LOGO_FADE_SPEED = 0.3f;

	// イージングの時間を進める速さ（1秒あたりのフレーム数）
	constexpr float EASING_FRAMES_PER_SECOND = 60.0f;

	// 入力表示やダイアログの不透明度を切り替える速さ
	constexpr float ALPHA_LERP_SPEED = 5.0f;

	// これ以下の不透明度なら描画しない
	constexpr float MIN_VISIBLE_ALPHA = 0.01f;
}


// 初期化
void SceneTitle::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// スプライト初期化
	titleBack = std::make_unique<Sprite>(device, "Data/Sprite/Title_Back.png");
	titleName = std::make_unique<Sprite>(device, "Data/Sprite/TitleLogo.png");
	anyButton = std::make_unique<Sprite>(device, "Data/Sprite/PressAnyKey.png");

	endPause = std::make_unique<Sprite>(device, "Data/Sprite/EndPause.png");
	endKey = std::make_unique<Sprite>(device, "Data/Sprite/EndKey.png");
	endCon = std::make_unique<Sprite>(device, "Data/Sprite/EndCon.png");
	endYes = std::make_unique<Sprite>(device, "Data/Sprite/EndYes.png");
	endNo = std::make_unique<Sprite>(device, "Data/Sprite/EndNo.png");
	endSelect = std::make_unique<Sprite>(device, "Data/Sprite/EndSele.png");

	bgm = Audio::Instance().LoadAudioSource(BGM_PATH);

	GpuResourceUtils::LoadTexture(device, "Data/Mask/dissolve_animation.png", maskTexture.GetAddressOf(), &maskTexture2dDesc);

	// sprite用デフォルト描画シェーダー
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	GpuResourceUtils::LoadVertexShader(device, "Data/Shader/SpriteDissolveVS.cso", inputElementDesc, _countof(inputElementDesc), spriteInputLayout.GetAddressOf(), spriteVertexShader.GetAddressOf());
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/SpriteDissolvePS.cso", spritePixelShader.GetAddressOf());
}

// 終了化
void SceneTitle::Finalize()
{
	delete bgm;
}

// 更新処理
void SceneTitle::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	Mouse& mouse = Input::Instance().GetMouse();

	bgm->Play(true, BGM_VOLUME);

	// なにかボタンを押したらローディングシーンを挟んでゲームシーンへ切り替え
	const GamePadButton anyButtonMask =
		GamePad::BTN_A
		| GamePad::BTN_B
		| GamePad::BTN_X
		| GamePad::BTN_Y
		;
	bool isEscPressed = gamePad.GetButtonDown() & GamePad::BTN_START;

	if (isEscPressed)
	{
		isPause = !isPause;
	}

	if (!isPause)
	{
		// Escが押されてたらスキップ
		if (!isEscPressed)
		{
			if (nameAlpha >= START_INPUT_ALPHA_THRESHOLD)
			{
				if (Input::Instance().IsAnyButtonPressed() || (gamePad.GetButtonDown() & anyButtonMask))
				{
					SceneLoading* loadingScene = new SceneLoading(new SceneGame());
					SceneManager::Instance().ChangeScene(loadingScene);
				}
			}
		}
	}
	else
	{
		if (gamePad.GetButtonDown() & GamePad::BTN_LEFT || gamePad.GetButtonDown() & GamePad::BTN_RIGHT)
		{
			isYesSelected = !isYesSelected;
		}
		if (isYesSelected)
		{
			sePos.x = YES_CURSOR_OFFSET_X;
			if (gamePad.GetButtonDown() & GamePad::BTN_A_EMU || gamePad.GetButtonDown() & GamePad::BTN_A)
			{
				//ゲームを落とす
				PostQuitMessage(0);  // メインループでWM_QUITを受け取って終了する
			}
		}
		else
		{
			sePos.x = 0.0f;
			if (gamePad.GetButtonDown() & GamePad::BTN_A_EMU)
			{
				isPause = !isPause;
			}
		}

		if (gamePad.GetButtonDown() & GamePad::BTN_B)
		{
			isPause = !isPause;
		}
	}



	if (easingData.startFlag)
	{
		easingData.time += EASING_FRAMES_PER_SECOND * elapsedTime;
	}

	if (easingData.time >= easingData.totalTime)
	{
		easingData.time = easingData.totalTime;
	}

	timer += elapsedTime;
	if (timer >= LOGO_FADE_START_SECONDS)
	{
		nameAlpha = Mathf::Lerp(nameAlpha, 1.0f, LOGO_FADE_SPEED * elapsedTime);

		if (timer >= ANY_BUTTON_BLINK_START_SECONDS)
		{
			alphaTime += elapsedTime * alphaSpeed;

			// サイン波で透明度を変化させる
			anyAlpha = (std::sin(alphaTime - DirectX::XM_PIDIV2) * 0.5f) + 0.5f;
		}
	}

	bool isController = Input::Instance().IsLastGamePad();

	const float lerpSpeed = ALPHA_LERP_SPEED;

	if (isController)
	{
		// ゲームパッド使用中なので、conAlphaを1に近づけ、keyAlphaを0に近づける
		keyAlpha = Mathf::Lerp(keyAlpha, 0.0f, elapsedTime * lerpSpeed);
		conAlpha = Mathf::Lerp(conAlpha, 1.0f, elapsedTime * lerpSpeed);
	}
	else
	{
		// キーボード使用中なので、keyAlphaを1に近づけ、conAlphaを0に近づける
		keyAlpha = Mathf::Lerp(keyAlpha, 1.0f, elapsedTime * lerpSpeed);
		conAlpha = Mathf::Lerp(conAlpha, 0.0f, elapsedTime * lerpSpeed);
	}

	if (isPause)
	{
		pauseAlpha = Mathf::Lerp(pauseAlpha, 1.0f, elapsedTime * lerpSpeed);
	}
	else
	{
		pauseAlpha = Mathf::Lerp(pauseAlpha, 0.0f, elapsedTime * lerpSpeed);
	}
}

// 描画処理
void SceneTitle::Render(float elapsedTime)
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
	buffers[FrameBufferId::Display]->SetRenderTargets(dc);

	RenderState* renderState = graphics.GetRenderState();

	ID3D11SamplerState* samplers[] =
	{
		renderState->GetSamplerState(SamplerState::PointClamp)
	};
	dc->PSSetSamplers(0, _countof(samplers), samplers);

	FLOAT blendFactor[4] = { 1.0f,1.0f,1.0f,1.0f };
	UINT sampleMask = 0xFFFFFFFF;

	// ブレンドステート
	dc->OMSetBlendState(
		renderState->GetBlendState(BlendState::Transparency),
		nullptr,
		0xFFFFFFFF
	);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	dc->PSSetShaderResources(1, 1, maskTexture.GetAddressOf());

	const float screenW = graphics.GetScreenWidth();
	const float screenH = graphics.GetScreenHeight();

	titleBack->Render(dc, 0, 0, 0, TITLE_DRAW_WIDTH, TITLE_DRAW_HEIGHT, 0, 0, TITLE_DRAW_WIDTH, TITLE_DRAW_HEIGHT, 0, 1, 1, 1, 1);
	titleName->Render(dc, 0, 0, 0, LOGO_DRAW_WIDTH, LOGO_DRAW_HEIGHT, 0, 0, LOGO_TEXTURE_WIDTH, LOGO_TEXTURE_HEIGHT, 0, 1, 1, 1, nameAlpha);
	anyButton->Render(dc, 0, 0, 0, TITLE_DRAW_WIDTH, TITLE_DRAW_HEIGHT, 0, 0, ANY_BUTTON_TEXTURE_WIDTH, ANY_BUTTON_TEXTURE_HEIGHT, 0, 1, 1, 1, anyAlpha);

	endKey->Render(dc, 0, 0, 0, TITLE_DRAW_WIDTH, TITLE_DRAW_HEIGHT, 0, 0, TITLE_DRAW_WIDTH, TITLE_DRAW_HEIGHT, 0, 1, 1, 1, keyAlpha);
	endCon->Render(dc, 0, 0, 0, TITLE_DRAW_WIDTH, TITLE_DRAW_HEIGHT, 0, 0, TITLE_DRAW_WIDTH, TITLE_DRAW_HEIGHT, 0, 1, 1, 1, conAlpha);

	if (pauseAlpha > MIN_VISIBLE_ALPHA)
	{
		endPause->Render(dc, 0, 0, 0, END_DIALOG_WIDTH, END_DIALOG_HEIGHT, 0, 0, END_DIALOG_WIDTH, END_DIALOG_HEIGHT, 0, 1, 1, 1, pauseAlpha);
		endSelect->Render(dc, sePos.x, sePos.y, sePos.z, END_DIALOG_WIDTH, END_DIALOG_HEIGHT, 0, 0, END_DIALOG_WIDTH, END_DIALOG_HEIGHT, 0, 1, 1, 1, pauseAlpha);

		// 選択中は白、非選択は黒で描画
		const float yesBrightness = isYesSelected ? 1.0f : 0.0f;
		const float noBrightness = isYesSelected ? 0.0f : 1.0f;
		endYes->Render(dc, 0, 0, 0, END_DIALOG_WIDTH, END_DIALOG_HEIGHT, 0, 0, END_DIALOG_WIDTH, END_DIALOG_HEIGHT, 0, yesBrightness, yesBrightness, yesBrightness, pauseAlpha);
		endNo->Render(dc, 0, 0, 0, END_DIALOG_WIDTH, END_DIALOG_HEIGHT, 0, 0, END_DIALOG_WIDTH, END_DIALOG_HEIGHT, 0, noBrightness, noBrightness, noBrightness, pauseAlpha);
	}

#ifdef _DEBUG
	DrawDebugGUI();
#endif
}

// デバッグ用GUI描画
void SceneTitle::DrawDebugGUI()
{
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);

	if (ImGui::Begin("Debug Menu", nullptr, ImGuiWindowFlags_None))
	{
		// UI
		if (ImGui::CollapsingHeader("UI", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// 位置
			ImGui::DragFloat2("UI_pos", &pos.x, 1.0f);
		}

		// UI_Easing
		if (ImGui::CollapsingHeader("UI_Easing", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button("Start"))
			{
				easingData.startFlag = true;
			}
			else if (ImGui::Button("Reset"))
			{
				easingData.startFlag = false;

				easingData.time = easingData.resetTime;
			}

			ImGui::RadioButton("OutBounce", &easingData.mode, OutBounce); ImGui::RadioButton("OutSine", &easingData.mode, OutSine);

			switch (easingData.mode)
			{
			case OutBounce:
				pos.x = Easing::OutBounce(easingData.time, easingData.totalTime, easingData.maxValue, easingData.minValue);
				break;

			case OutSine:
				pos.x = Easing::OutCubic(easingData.time, easingData.totalTime, easingData.maxValue, easingData.minValue);
				break;
			}

			// 始まりの時間
			ImGui::DragFloat("EasingTime", &easingData.time, 0.1f);
			// 終わりの時間
			ImGui::DragFloat("EasingTotalTime", &easingData.totalTime, 0.1f);
			// 最大の値
			ImGui::DragFloat("EasingMaxValue", &easingData.maxValue, 0.1f);
			// 最小の値
			ImGui::DragFloat("EasingMinValue", &easingData.minValue, 0.1f);

		}
		ImGui::DragFloat("amount", &amount, 0.01f, 0, 1);

		ImGui::DragFloat("nameAlpha", &nameAlpha, 0.01f, 0, 1);
		ImGui::DragFloat("anyAlpha", &anyAlpha, 0.01f, 0, 1);

		ImGui::DragFloat3("Sepos", &sePos.x, 0.01f);
		ImGui::DragFloat3("Sopos", &soPos.x, 1.0f);
		ImGui::DragFloat3("Sosca", &soScale.x, 1.0f);
	}
	ImGui::End();
}
