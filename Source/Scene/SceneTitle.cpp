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


// 初期化
void SceneTitle::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// スプライト初期化
	TitleBack = std::make_unique<Sprite>(device, "Data/Sprite/Title_Back.png");
	TitleName = std::make_unique<Sprite>(device, "Data/Sprite/TitleLogo.png");
	AnyButton = std::make_unique<Sprite>(device, "Data/Sprite/PressAnyKey.png");

	EndPause = std::make_unique<Sprite>(device, "Data/Sprite/EndPause.png");
	EndKey = std::make_unique<Sprite>(device, "Data/Sprite/EndKey.png");
	EndCon = std::make_unique<Sprite>(device, "Data/Sprite/EndCon.png");
	EndYes = std::make_unique<Sprite>(device, "Data/Sprite/EndYes.png");
	EndNo = std::make_unique<Sprite>(device, "Data/Sprite/EndNo.png");
	EndSele = std::make_unique<Sprite>(device, "Data/Sprite/EndSele.png");

	BGM = Audio::Instance().LoadAudioSource("Data/Sound/BGM/Dearly in Dreams.wav");

	GpuResourceUtils::LoadTexture(device, "Data/Mask/dissolve_animation.png", mask_texture.GetAddressOf(), &mask_texture2dDesc);

	// sprite用デフォルト描画シェーダー
	D3D11_INPUT_ELEMENT_DESC input_element_desc[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	GpuResourceUtils::LoadVertexShader(device, "Data/Shader/SpriteDissolveVS.cso", input_element_desc, _countof(input_element_desc), sprite_input_layout.GetAddressOf(), sprite_vertex_shader.GetAddressOf());
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/SpriteDissolvePS.cso", sprite_pixel_shader.GetAddressOf());
}

// 終了化
void SceneTitle::Finalize()
{
	delete BGM;
}

// 更新処理
void SceneTitle::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	Mouse& mouse = Input::Instance().GetMouse();

	BGM->Play(true, 0.5f);

	// なにかボタンを押したらローディングシーンを挟んでゲームシーンへ切り替え
	const GamePadButton anyButton =
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
			if (NameAlpha >= 0.3f)
			{
				if (Input::Instance().GetAnyButton() || (gamePad.GetButtonDown() & anyButton))
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
			sePos.x = -285.0f;
			if (gamePad.GetButtonDown() & GamePad::BTN_A_EMU || gamePad.GetButtonDown() & GamePad::BTN_A)
			{
				//ゲームを落とす
				PostQuitMessage(0);  // メインループでWM_QUITを受け取って終了する
			}
		}
		else
		{
			sePos.x = 0;
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
		easingData.time += 60.0f * elapsedTime;
	}

	if (easingData.time >= easingData.totalTime)
	{
		easingData.time = easingData.totalTime;
	}

	timer += elapsedTime;
	if (timer >= 1.f)
	{
		NameAlpha = Mathf::Lerp(NameAlpha, 1.0f, 0.3f * elapsedTime);

		if (timer >= 3.f)
		{
			alphaTime += elapsedTime * alphaSpeed;

			// サイン波で透明度を変化させる
			AnyAlpha = (std::sin(alphaTime - DirectX::XM_PIDIV2) * 0.5f) + 0.5f;
		}
	}

	bool isController = Input::Instance().GetIsLastGamePad();

	float lerpSpeed = 5.0f;  // 数値大きいほど速い（調整可）

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
	DirectX::XMFLOAT4 color = { 0.2f, 0.2f, 0.2f, 1.0f };	// RGBA(0.0～1.0);
	std::map<FrameBufferId, FrameBuffer*> buffers;
	for (int i = 0; i < static_cast<int>(FrameBufferId::EnumCount); i++)
	{
		buffers[static_cast<FrameBufferId>(i)] = Graphics::Instance().GetFrameBuffer(static_cast<FrameBufferId>(i));
		buffers[static_cast<FrameBufferId>(i)]->Clear(dc, color);
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

	dc->PSSetShaderResources(1, 1, mask_texture.GetAddressOf());

	const float screenW = graphics.GetScreenWidth();
	const float screenH = graphics.GetScreenHeight();

	TitleBack->Render(dc, 0, 0, 0, 1980, 1080, 0, 0, 1980, 1080, 0, 1, 1, 1, 1);
	TitleName->Render(dc, 0, 0, 0, 1228, 819, 0, 0, 1536, 1024, 0, 1, 1, 1, NameAlpha);
	AnyButton->Render(dc, 0, 0, 0, 1980, 1080, 0, 0, 1920, 1080, 0, 1, 1, 1, AnyAlpha);

	EndKey->Render(dc, 0, 0, 0, 1980, 1080, 0, 0, 1980, 1080, 0, 1, 1, 1, keyAlpha);
	EndCon->Render(dc, 0, 0, 0, 1980, 1080, 0, 0, 1980, 1080, 0, 1, 1, 1, conAlpha);


	if (pauseAlpha > 0.01f)
	{
		EndPause->Render(dc, 0, 0, 0, 1280, 720, 0, 0, 1280, 720, 0, 1, 1, 1, pauseAlpha);
		EndSele->Render(dc, sePos.x, sePos.y, sePos.z, 1280, 720, 0, 0, 1280, 720, 0, 1, 1, 1, pauseAlpha);

		// 選択中は白、非選択は黒で描画
		if (isYesSelected)
		{
			// Yesが選択中
			EndYes->Render(dc, 0, 0, 0, 1280, 720, 0, 0, 1280, 720, 0, 1, 1, 1, pauseAlpha);
			EndNo->Render(dc, 0, 0, 0, 1280, 720, 0, 0, 1280, 720, 0, 0, 0, 0, pauseAlpha);
		}
		else
		{
			// Noが選択中
			EndYes->Render(dc, 0, 0, 0, 1280, 720, 0, 0, 1280, 720, 0, 0, 0, 0, pauseAlpha);
			EndNo->Render(dc, 0, 0, 0, 1280, 720, 0, 0, 1280, 720, 0, 1, 1, 1, pauseAlpha);
		}
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
			case 0:
				pos.x = Easing::OutBounce(easingData.time, easingData.totalTime, easingData.maxValue, easingData.minValue);
				break;

			case 1:
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

		ImGui::DragFloat("NameAlpha", &NameAlpha, 0.01f, 0, 1);
		ImGui::DragFloat("AnyAlpha", &AnyAlpha, 0.01f, 0, 1);

		ImGui::DragFloat3("Sepos", &sePos.x, 0.01f);
		ImGui::DragFloat3("Sopos", &soPos.x, 1.0f);
		ImGui::DragFloat3("Sosca", &soscale.x, 1.0f);
	}
	ImGui::End();
}
