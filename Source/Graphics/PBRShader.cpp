#include "System/Misc.h"
#include "GpuResourceUtils.h"
#include "PBRShader.h"
#include <imgui.h>

PBRShader::PBRShader(ID3D11Device* device)
{
	// 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		// ボーン影響データを追加
		// ※並び順をVertex構造体の要素と同じにする
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/PBRVS.cso",
		inputElementDesc,
		_countof(inputElementDesc),
		inputLayout.GetAddressOf(),
		vertexShader.GetAddressOf());

	// ピクセルシェーダー
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/PBRPS.cso",
		pixelShader.GetAddressOf());

	// メッシュ用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbMesh),
		meshConstantBuffer.GetAddressOf());

	// カラー用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbColor),
		colorConstantBuffer.GetAddressOf());

	// マテリアル用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbMaterial),
		materialConstantBuffer.GetAddressOf());

	//PBRセットアップ用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSetUp),
		setUpConstantBuffer.GetAddressOf());

	//IBLテクスチャを読み込み
	{
		D3D11_TEXTURE2D_DESC texture2dDesc{};

		GpuResourceUtils::LoadTexture(device, "Data/SkyBox/sunset_sky/sunsetSkyDiffuseHDR.dds",
			diffuseIemShaderResourceView.GetAddressOf(), &texture2dDesc);

		GpuResourceUtils::LoadTexture(device, "Data/SkyBox/sunset_sky/sunsetSkySpecularHDR.dds",
			specularPmremShaderResourceView.GetAddressOf(), &texture2dDesc);

		GpuResourceUtils::LoadTexture(device, "Data/SkyBox/sunset_sky/sunsetSkyBrdf.dds",
			lutGgxShaderResourceView.GetAddressOf(), &texture2dDesc);
	}
	SetUpConstant = std::make_unique<CbSetUp>();
	cbSetUp.IBLDiffuseScale = 1.0f;
	cbSetUp.IBLSpecularScale = 1.0f;
}

// 描画開始
void PBRShader::Begin(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->IASetInputLayout(inputLayout.Get());
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);

	// 定数バッファ設定
	ID3D11Buffer* constantBuffers[] =
	{
		meshConstantBuffer.Get(),
		materialConstantBuffer.Get(),
		colorConstantBuffer.Get(),
		setUpConstantBuffer.Get(),
	};
	dc->VSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);
	// ピクセルシェーダーにも定数バッファを設定する
	dc->PSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);

	// サンプラーステート設定
	ID3D11SamplerState* samplerStates[] =
	{
		rc.renderState->GetSamplerState(SamplerState::LinearWrap),
		rc.renderState->GetSamplerState(SamplerState::Anisotropic),
		rc.renderState->GetSamplerState(SamplerState::SHADOW),
	};
	dc->PSSetSamplers(0, _countof(samplerStates), samplerStates);

	// レンダーステート設定
	const float blend_factor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), blend_factor, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	//IBLテクスチャを設定
	dc->PSSetShaderResources(33, 1, diffuseIemShaderResourceView.GetAddressOf());
	dc->PSSetShaderResources(34, 1, specularPmremShaderResourceView.GetAddressOf());
	dc->PSSetShaderResources(35, 1, lutGgxShaderResourceView.GetAddressOf());
}

// 描画
void PBRShader::Update(const RenderContext& rc, const ModelResource::Mesh& mesh, const std::shared_ptr<Model> model)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	if (!IsDraw) IsDraw = !IsDraw;

	//セットアップ用定数バッファ更新
	{
		rc.deviceContext->UpdateSubresource(setUpConstantBuffer.Get(), 0, 0, &cbSetUp, 0, 0);
	}

	{
		// 頂点バッファ設定
		UINT stride = sizeof(ModelResource::Vertex);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// メッシュ用定数バッファ更新
		CbMesh cbMesh{};
		cbMesh.materialColor = mesh.material->baseColor;
		dc->UpdateSubresource(meshConstantBuffer.Get(), 0, 0, &cbMesh, 0, 0);

		//カラー用定数バッファ更新
		CbColor cbColor{};
		cbColor.isEmissive = mesh.material->IsEmissive;
		cbColor.adjustColor = mesh.material->emissiveColor;
		dc->UpdateSubresource(colorConstantBuffer.Get(), 0, 0, &cbColor, 0, 0);

		//マテリアル用定数バッファ更新
		CbMaterial cbMaterial{};
		cbMaterial.normalScale = 1.0f;
		cbMaterial.adjustMetalness = model->GetAdMetalness(); // 金属質調整
		cbMaterial.adjustRoughness = model->GetAdRoughness(); // 粗さ調整
		cbMaterial.metalicFactor = mesh.material->metalness;
		cbMaterial.roughnessFactor = mesh.material->roughness;
		cbMaterial.occlusionStrength = 1;
		cbMaterial.emissiveColor = mesh.material->emissiveColor;
		cbMaterial.metalicindex = mesh.material->metalness;
		dc->UpdateSubresource(materialConstantBuffer.Get(), 0, 0, &cbMaterial, 0, 0);

		// シェーダーリソースビュー設定
		ID3D11ShaderResourceView* srvs[] =
		{
			mesh.material->baseMap.Get(),
			mesh.material->normalMap.Get(),
			rc.shadowMap->GetShaderResourceView(),
			mesh.material->emissiveMap.Get(),
			mesh.material->metalnessRoughnessMap.Get(),
			mesh.material->occlusionMap.Get(),
		};
		//dc->PSSetShaderResources(0, 1, mesh.material->diffuseMap.GetAddressOf());
		dc->PSSetShaderResources(0, _countof(srvs), srvs);
	}
}

// 描画終了
void PBRShader::End(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	// 設定されているシェーダーリソースを解除
	// シャドウマップのシェーダーリソースを解除しないとエラーが出力されてしまう
	ClearShaderResourceViews(0, dc);
	ClearShaderSlots(dc);
}

void PBRShader::ImGui()
{
	if (ImGui::CollapsingHeader("PBRSetUp", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("IBLDiffuse", &cbSetUp.IBLDiffuseScale, 0.1f);
		ImGui::DragFloat("IBLSpecular", &cbSetUp.IBLSpecularScale, 0.1f);
	}
}
