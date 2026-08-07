#include "SceneClear.h"
#include "Graphics/Graphics.h"
#include "SceneTitle.h"
#include "SceneManager.h"
#include "Input/Input.h"
#include "Graphics/GpuResourceUtils.h"
#include "Math/Mathf.h"
#include <map>

void SceneClear::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// スプライト初期化
	Result = std::make_unique<Sprite>(device, "Data/Sprite/Result.png");


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

void SceneClear::Finalize()
{
}

void SceneClear::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	Mouse& mouse = Input::Instance().GetMouse();

	// なにかボタンを押したらローディングシーンを挟んでゲームシーンへ切り替え
	const GamePadButton anyButton =
		GamePad::BTN_A
		| GamePad::BTN_B
		| GamePad::BTN_X
		| GamePad::BTN_Y
		;
	if (ImGui::IsAnyItemHovered() == false && ImGui::GetIO().WantCaptureMouse == false)
	{
		if (timer > 1)
		{
			if (Input::Instance().GetAnyButton() || gamePad.GetButtonDown() & anyButton || mouse.GetButtonDown() & Mouse::BTN_LEFT)
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
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), blendFactor, sampleMask);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	dc->PSSetShaderResources(1, 1, mask_texture.GetAddressOf());

	const float screenW = graphics.GetScreenWidth();
	const float screenH = graphics.GetScreenHeight();

	Result->Render(dc, 0, 0, 0, screenW, screenH, 0, 0, 1920, 1080, 0, 1, 1, 1, 1);

#ifdef _DEBUG
	DrawDebugGUI();
#endif
}

void SceneClear::DrawDebugGUI()
{
}
