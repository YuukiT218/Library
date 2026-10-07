#pragma once

#include "Scene.h"
#include "Sprite/Sprite.h"
#include <memory>

// タイトルシーン
class SceneClear : public Scene
{
public:
	SceneClear() {}
	~SceneClear() override {}

	// 初期化
	void Initialize() override;

	// 終了化
	void Finalize() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI();

public:
	D3D11_TEXTURE2D_DESC maskTexture2dDesc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> maskTexture;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePixelShader;
private:
	std::unique_ptr<Sprite> resultSprite;

	float timer = 0.0f;
};