#include "Graphics/Graphics.h"
#include "SceneTitle.h"
#include "Input/Input.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneLoading.h"
#include "Scene/SceneGame.h"
#include <map>

SceneTitle::SceneTitle()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	//スプライト初期化
	sprite = std::make_unique<Sprite>(device, "Data/Sprite/Title.png");
	//character = std::make_shared<Model>(device, "Data/Model/RPG-Character/RPG-Character.glb");
}

void SceneTitle::Initialize()
{
	
}

void SceneTitle::Finalize()
{
	
}

//更新処理
void SceneTitle::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	//何かボタンを押したらゲームシーンへ切り替え
	const GamePadButton anyButton =
		GamePad::BTN_A
		| GamePad::BTN_B
		| GamePad::BTN_X
		| GamePad::BTN_Y;

	if (gamePad.GetButtonDown() & anyButton)
	{
		//SceneManager::Instance().ChangeScene(new SceneLoading);
		SceneManager::Instance().ChangeScene(new SceneLoading(new SceneGame));
	}
}

//描画処理
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

	//2Dスプライト描画
	{
		float screenWidth = static_cast<float>(graphics.GetScreenWidth());
		float screenHeight = static_cast<float>(graphics.GetScreenHeight());
		float textureWidth = static_cast<float>(sprite->GetTextureWidth());
		float textureHeight = static_cast<float>(sprite->GetTextureHeight());
		//タイトルスプライト描画
		sprite->Render(dc,
			0, 0, 0, screenWidth, screenHeight,
			0, 0, textureWidth, textureHeight,
			0,
			1, 1, 1, 1);
	}

	// 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = graphics.GetDeviceContext();
	rc.renderState = graphics.GetRenderState();

	/*modelRenderer->Draw(ShaderId::Lambert, character);
	modelRenderer->Render(rc);*/
}