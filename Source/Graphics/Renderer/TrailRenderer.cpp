#include "System/Misc.h"
#include "Graphics/GpuResourceUtils.h"
#include "TrailRenderer.h"
#include <imgui.h>

// コンストラクタ
TrailRenderer::TrailRenderer(ID3D11Device* device)
{
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[]
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "DISSOLVE", 0, DXGI_FORMAT_R32_FLOAT,			 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/TrailRendererVS.cso",
		inputElementDesc,
		_countof(inputElementDesc),
		inputLayout.GetAddressOf(),
		vertexShader.GetAddressOf());

	// ピクセルシェーダー
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/TrailRendererPS.cso",
		pixelShader.GetAddressOf());

	// 定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbScene),
		constantBuffer.GetAddressOf());

	{//テクスチャ
		D3D11_TEXTURE2D_DESC texture2dDesc{};
		GpuResourceUtils::LoadTexture(device, "Data/Model/Weapon/Trail/Trail.png",
			trailShaderResourceView.GetAddressOf(), &texture2dDesc);
		//dissolve_animation2
		//electric_noise_texture
		GpuResourceUtils::LoadTexture(device, "Data/Mask/cloud_noise_texture.png",
			dissolveShaderResourceView.GetAddressOf(), &texture2dDesc);
	}

	// 頂点バッファ
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = sizeof(Vertex) * VERTEX_CAPACITY;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	HRESULT hr = device->CreateBuffer(&desc, nullptr, vertexBuffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
}

// 頂点追加
void TrailRenderer::AddVertex(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color, const DirectX::XMFLOAT2& texcoord, float dissolve)
{
	Vertex& v = vertices.emplace_back();
	v.position = position;
	v.color = color;
	v.texcoord = texcoord;
	v.dissolve = 1.0f - dissolve;
}

// 軸描画
void TrailRenderer::DrawAxis(const DirectX::XMFLOAT4X4& transform, const DirectX::XMFLOAT4& color)
{
	DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&transform);
	DirectX::XMFLOAT3 p, x, y, z;
	DirectX::XMStoreFloat3(&p, W.r[3]);
	DirectX::XMStoreFloat3(&x, DirectX::XMVector3Transform(DirectX::XMVectorSet(1, 0, 0, 0), W));
	DirectX::XMStoreFloat3(&y, DirectX::XMVector3Transform(DirectX::XMVectorSet(0, 1, 0, 0), W));
	DirectX::XMStoreFloat3(&z, DirectX::XMVector3Transform(DirectX::XMVectorSet(0, 0, 1, 0), W));
	AddVertex(p, { 1, 0, 0, 1 }, { 0, 0 });
	AddVertex(x, { 1, 0, 0, 1 }, { 0, 0 });
	AddVertex(p, { 0, 1, 0, 1 }, { 0, 0 });
	AddVertex(y, { 0, 1, 0, 1 }, { 0, 0 });
	AddVertex(p, { 0, 0, 1, 1 }, { 0, 0 });
	AddVertex(z, { 0, 0, 1, 1 }, { 0, 0 });
}

// グリッド描画
void TrailRenderer::DrawGrid(int subdivisions, float scale)
{
	int numLines = (subdivisions + 1) * 2;
	int vertexCount = numLines * 2;

	float corner = 0.5f;
	float step = 1.0f / static_cast<float>(subdivisions);

	int index = 0;
	float s = -corner;

	const DirectX::XMFLOAT2 texcoord = DirectX::XMFLOAT2(0, 0);
	const DirectX::XMFLOAT4 white = DirectX::XMFLOAT4(1, 1, 1, 1);

	// Create vertical lines
	float scaling = static_cast<float>(subdivisions) * scale;
	DirectX::XMMATRIX M = DirectX::XMMatrixScaling(scaling, scaling, scaling);
	DirectX::XMVECTOR V, P;
	DirectX::XMFLOAT3 position;
	for (int i = 0; i <= subdivisions; i++)
	{
		V = DirectX::XMVectorSet(s, 0, corner, 0);
		P = DirectX::XMVector3TransformCoord(V, M);
		DirectX::XMStoreFloat3(&position, P);
		AddVertex(position, white, texcoord);

		V = DirectX::XMVectorSet(s, 0, -corner, 0);
		P = DirectX::XMVector3TransformCoord(V, M);
		DirectX::XMStoreFloat3(&position, P);
		AddVertex(position, white, texcoord);

		s += step;
	}

	// Create horizontal lines
	s = -corner;
	for (int i = 0; i <= subdivisions; i++)
	{
		V = DirectX::XMVectorSet(corner, 0, s, 0);
		P = DirectX::XMVector3TransformCoord(V, M);
		DirectX::XMStoreFloat3(&position, P);
		AddVertex(position, white, texcoord);

		V = DirectX::XMVectorSet(-corner, 0, s, 0);
		P = DirectX::XMVector3TransformCoord(V, M);
		DirectX::XMStoreFloat3(&position, P);
		AddVertex(position, white, texcoord);

		s += step;
	}

	// X軸
	{
		const DirectX::XMFLOAT4 red = DirectX::XMFLOAT4(1, 0, 0, 1);
		V = DirectX::XMVectorSet(0, 0, 0, 0);
		P = DirectX::XMVector3TransformCoord(V, M);
		DirectX::XMStoreFloat3(&position, P);
		AddVertex(position, red, texcoord);

		V = DirectX::XMVectorSet(corner, 0, 0, 0);
		P = DirectX::XMVector3TransformCoord(V, M);
		DirectX::XMStoreFloat3(&position, P);
		AddVertex(position, red, texcoord);
	}

	// Y軸
	{
		const DirectX::XMFLOAT4 green = DirectX::XMFLOAT4(0, 1, 0, 1);
		V = DirectX::XMVectorSet(0, 0, 0, 0);
		P = DirectX::XMVector3TransformCoord(V, M);
		DirectX::XMStoreFloat3(&position, P);
		AddVertex(position, green, texcoord);

		V = DirectX::XMVectorSet(0, corner, 0, 0);
		P = DirectX::XMVector3TransformCoord(V, M);
		DirectX::XMStoreFloat3(&position, P);
		AddVertex(position, green, texcoord);
	}

	// Z軸
	{
		const DirectX::XMFLOAT4 blue = DirectX::XMFLOAT4(0, 0, 1, 1);
		V = DirectX::XMVectorSet(0, 0, 0, 0);
		P = DirectX::XMVector3TransformCoord(V, M);
		DirectX::XMStoreFloat3(&position, P);
		AddVertex(position, blue, texcoord);

		V = DirectX::XMVectorSet(0, 0, corner, 0);
		P = DirectX::XMVector3TransformCoord(V, M);
		DirectX::XMStoreFloat3(&position, P);
		AddVertex(position, blue, texcoord);
	}
}

// 描画実行
void TrailRenderer::Render(
	ID3D11DeviceContext* dc,
	const RenderContext& rc,
	D3D11_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	// シェーダー設定
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);
	dc->IASetInputLayout(inputLayout.Get());

	//テクスチャをピクセルシェーダーに送信
	dc->PSSetShaderResources(0, 1, trailShaderResourceView.GetAddressOf());
	dc->PSSetShaderResources(1, 1, dissolveShaderResourceView.GetAddressOf());

	// 定数バッファ設定
	dc->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());

	// ビュープロジェクション行列作成
	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
	DirectX::XMMATRIX VP = V * P;

	// サンプラステート設定
	ID3D11SamplerState* samplers[] =
	{
		rc.renderState->GetSamplerState(SamplerState::PointWrap),
		rc.renderState->GetSamplerState(SamplerState::LinearWrap),
		rc.renderState->GetSamplerState(SamplerState::Anisotropic),
		rc.renderState->GetSamplerState(SamplerState::LinearBorder),
		rc.renderState->GetSamplerState(SamplerState::LinearBorder),
	};
	dc->PSSetSamplers(0, _countof(samplers), samplers);

	// 定数バッファ更新
	CbScene cbScene;
	DirectX::XMStoreFloat4x4(&cbScene.viewProjection, VP);
	cbScene.direction = scrollDirection;
	cbScene.Timer = rc.timer;
	dc->UpdateSubresource(constantBuffer.Get(), 0, 0, &cbScene, 0, 0);

	// レンダーステート設定
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// 頂点バッファ設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	dc->IASetPrimitiveTopology(primitiveTopology);
	dc->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);

	// 描画
	UINT totalVertexCount = static_cast<UINT>(vertices.size());
	UINT start = 0;
	UINT count = (totalVertexCount < VERTEX_CAPACITY) ? totalVertexCount : VERTEX_CAPACITY;

	while (start < totalVertexCount)
	{
		D3D11_MAPPED_SUBRESOURCE mappedSubresource;
		HRESULT hr = dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		memcpy(mappedSubresource.pData, &vertices[start], sizeof(Vertex) * count);

		dc->Unmap(vertexBuffer.Get(), 0);

		dc->Draw(count, 0);

		start += count;
		if ((start + count) > totalVertexCount)
		{
			count = totalVertexCount - start;
		}
	}

	vertices.clear();
}

void TrailRenderer::ImGui()
{
	if (ImGui::CollapsingHeader("TrailRenderer", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat2("TrailScroll", &scrollDirection.x, 0.1f);
	}
}

void TrailRenderer::CreateIndexBuffer(ID3D11Device* device, size_t maxIndexCount)
{
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * maxIndexCount);
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA initData = {};
	std::vector<uint32_t> temp(maxIndexCount);
	initData.pSysMem = temp.data();

	HRESULT hr = device->CreateBuffer(&bd, &initData, &indexBuffer);
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
}