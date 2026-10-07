#pragma once

#include <d3d11.h>
#include <memory>

#include "RenderContext.h"
#include "Sprite/Sprite.h"

class SkyBox
{
public:
	SkyBox(ID3D11Device* device);

	// 開始処理
	void Begin(const RenderContext& rc);

	// 描画
	void Render(const RenderContext& rc);

	void End(const RenderContext& rc);

private:
	struct SceneConstants {
		DirectX::XMFLOAT4X4 viewProjection;
		//DirectX::XMFLOAT4 options;         // xy: マウス座標, z: タイマー, w: フラグ
		DirectX::XMFLOAT4 cameraPosition;
		DirectX::XMFLOAT4X4 inverseViewProjection;
	};

	Microsoft::WRL::ComPtr<ID3D11VertexShader> skyBoxVertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> skyBoxInputLayout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> skyBoxPixelShader;
	std::unique_ptr<Sprite> skyBoxRenderSprite;

	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> specularPmremShaderResourceView;
};