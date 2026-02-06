#pragma once

#include "Scene.h"
#include "Sprite/Sprite.h"
#include <memory>

// タイトルシーン
class SceneResult : public Scene
{
public:
	SceneResult() {}
	~SceneResult() override {}

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
	D3D11_TEXTURE2D_DESC mask_texture2dDesc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mask_texture;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> sprite_vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> sprite_input_layout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> sprite_pixel_shader;
private:
	std::unique_ptr<Sprite> Result;

	float timer = 0;
};