#include "SceneClear.h"
#include "Graphics/Graphics.h"
#include "SceneTitle.h"
#include "SceneManager.h"
#include "Input/Input.h"
#include "Graphics/GpuResourceUtils.h"
#include "Math/Mathf.h"
#include "System/ScreenSize.h"
#include <map>

namespace
{
	// 全画面の1枚絵の画像サイズ
	constexpr float FULL_SCREEN_TEXTURE_WIDTH = static_cast<float>(ScreenSize::WIDTH);
	constexpr float FULL_SCREEN_TEXTURE_HEIGHT = static_cast<float>(ScreenSize::HEIGHT);

	// 遷移直後の誤入力を防ぐため、入力を受け付けない時間
	constexpr float INPUT_WAIT_SECONDS = 1.0f;

	// 画面クリア色
	const DirectX::XMFLOAT4 CLEAR_COLOR = { 0.2f, 0.2f, 0.2f, 1.0f };
}

void SceneClear::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// スプライト初期化
	resultSprite = std::make_unique<Sprite>(device, "Data/Sprite/Result.png");


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

void SceneClear::Finalize()
{
}

void SceneClear::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	Mouse& mouse = Input::Instance().GetMouse();

	// なにかボタンを押したらローディングシーンを挟んでゲームシーンへ切り替え
	const GamePadButton anyButtonMask =
		GamePad::BTN_A
		| GamePad::BTN_B
		| GamePad::BTN_X
		| GamePad::BTN_Y
		;
	if (ImGui::IsAnyItemHovered() == false && ImGui::GetIO().WantCaptureMouse == false)
	{
		if (timer > INPUT_WAIT_SECONDS)
		{
			if (Input::Instance().IsAnyButtonPressed() || gamePad.GetButtonDown() & anyButtonMask || mouse.GetButtonDown() & Mouse::BTN_LEFT)
			{
				SceneManager::Instance().ChangeScene(new SceneTitle);
			}
		}
	}

	timer += elapsedTime;
}

void SceneClear::Render(float elapsedTime)
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
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), blendFactor, sampleMask);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	dc->PSSetShaderResources(1, 1, maskTexture.GetAddressOf());

	const float screenW = graphics.GetScreenWidth();
	const float screenH = graphics.GetScreenHeight();

	resultSprite->Render(dc, 0, 0, 0, screenW, screenH, 0, 0, FULL_SCREEN_TEXTURE_WIDTH, FULL_SCREEN_TEXTURE_HEIGHT, 0, 1, 1, 1, 1);

#ifdef _DEBUG
	DrawDebugGUI();
#endif
}

void SceneClear::DrawDebugGUI()
{
}
