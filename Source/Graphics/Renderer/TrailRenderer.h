#pragma once

#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "Graphics/RenderContext.h"

class TrailRenderer
{
public:
	TrailRenderer(ID3D11Device* device);

	// 頂点追加
	void AddVertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color, const DirectX::XMFLOAT2& texcoord, float dissolve = 0);

	void TrailRenderer::CreateIndexBuffer(ID3D11Device* device, size_t maxIndexCount);

	// 軸描画(D3D11_PRIMITIVE_TOPOLOGY_LINELIST)
	void DrawAxis(const DirectX::XMFLOAT4X4& transform, const DirectX::XMFLOAT4& color);

	// グリッド描画(D3D11_PRIMITIVE_TOPOLOGY_LINELIST)
	void DrawGrid(int subdivisions, float scale);

	// 描画実行
	void Render(
		ID3D11DeviceContext* dc,
		const RenderContext& rc,
		D3D11_PRIMITIVE_TOPOLOGY primitiveTopology);

	//ImGui
	void ImGui();

private:
	static const UINT VERTEX_CAPACITY = 3 * 1024;

	struct CbScene
	{
		DirectX::XMFLOAT4X4		viewProjection;
		DirectX::XMFLOAT2		 direction; //ディゾルブのUVスクロール
		float					 Timer; //UVスクロール用タイマー
		float					 dummy;
		DirectX::XMFLOAT4 lightDirection;
		DirectX::XMFLOAT4 lightColor;
	};

	struct Vertex
	{
		DirectX::XMFLOAT3	position;
		DirectX::XMFLOAT4	color;
		DirectX::XMFLOAT2	texcoord;
		float				dissolve;
	};
	std::vector<Vertex>		vertices;

	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		constantBuffer;

	//	剣トレイル用テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  trailShaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  normalShaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  dissolveShaderResourceView;

	//ディゾルブUVスクロール用
	DirectX::XMFLOAT2 scrollDirection{};

	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	std::vector<uint32_t> indices;
};
