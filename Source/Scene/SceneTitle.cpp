#include "Graphics/Graphics.h"
#include "SceneTitle.h"

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
	//GamePad& gamePad = Input::Instance().GetGamePad();
	////何かボタンを押したらゲームシーンへ切り替え
	//const GamePadButton anyButton =
	//	GamePad::BTN_A
	//	| GamePad::BTN_B
	//	| GamePad::BTN_X
	//	| GamePad::BTN_Y;

	//if (gamePad.GetButtonDown() & anyButton)
	//{
	//	//SceneManager::Instance().ChangeScene(new SceneLoading);
	//	SceneManager::Instance().ChangeScene(new SceneLoading(new SceneGame));
	//}
}

//描画処理
void SceneTitle::Render(float elapsedTime)
{
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();
	ID3D11RenderTargetView* rtv = graphics.GetRenderTargetView();
	ID3D11DepthStencilView* dsv = graphics.GetDepthStencilView();
	ModelRenderer* modelRenderer = graphics.GetModelRenderer();

	//画面クリア＆レンダーターゲット設定
	FLOAT color[] = { 0.0f, 0.0f, 0.5f, 1.0f };
	dc->ClearRenderTargetView(rtv, color);
	dc->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	dc->OMSetRenderTargets(1, &rtv, dsv);

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