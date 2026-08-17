#include "System/Misc.h"
#include "GpuResourceUtils.h"
#include "PBRShader.h"
#include <imgui.h>


#include <stdlib.h>



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

	// PBRセットアップ用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSetUp),
		setUpConstantBuffer.GetAddressOf());

	// リムライト用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbRimLight),
		rimLightConstantBuffer.GetAddressOf());

	// カスケードシャドウ用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbShadow),
		shadowConstantBuffer.GetAddressOf());

	//IBLテクスチャを読み込み
	{
		D3D11_TEXTURE2D_DESC texture2dDesc{};

		GpuResourceUtils::LoadTexture(device, "Data/SkyBox/dusk_sky/DiffuseHDR.dds",
			diffuseIemShaderResourceView.GetAddressOf(), &texture2dDesc);

		GpuResourceUtils::LoadTexture(device, "Data/SkyBox/dusk_sky/SpecularHDR.dds",
			specularPmremShaderResourceView.GetAddressOf(), &texture2dDesc);

		GpuResourceUtils::LoadTexture(device, "Data/SkyBox/dusk_sky/Brdf.dds",
			lutGgxShaderResourceView.GetAddressOf(), &texture2dDesc);
	}
}

// 描画開始
void PBRShader::Begin(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->IASetInputLayout(inputLayout.Get());
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);

	// カスケードシャドウ用定数バッファ更新
	{
		const ShadowMap* shadowMap = rc.shadowMap;

		CbShadow cbShadow{};
		for (int i = 0; i < SHADOW_BUFFER_SIZE; ++i)
		{
			cbShadow.CascadeLightViewProjection[i] = shadowMap->GetCascadeLightViewProjection(i);
		}
		cbShadow.cascadeSplits = shadowMap->GetCascadeSplits();
		cbShadow.cascadeFlags.x = shadowMap->IsCascadeDebugView() ? 1.0f : 0.0f;
		cbShadow.shadowColor = shadowMap->GetColor();
		cbShadow.shadowTexelSize = shadowMap->GetTexelSize();
		cbShadow.shadowAttenuation = shadowMap->GetAttenuation();
		cbShadow.shadowBias = shadowMap->GetBias();
		cbShadow.shadowNormalOffset = shadowMap->GetNormalOffset();
		cbShadow.indirectShadowStrength = shadowMap->GetIndirectShadowStrength();
		cbShadow.cascadeTexelWorldSize = shadowMap->GetCascadeTexelWorldSize();
		dc->UpdateSubresource(shadowConstantBuffer.Get(), 0, 0, &cbShadow, 0, 0);
	}

	// 定数バッファ設定
	ID3D11Buffer* constantBuffers[] =
	{
		meshConstantBuffer.Get(),
		materialConstantBuffer.Get(),
		colorConstantBuffer.Get(),
		setUpConstantBuffer.Get(),
		shadowConstantBuffer.Get(),
	};
	dc->VSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);
	// ピクセルシェーダーにも定数バッファを設定する
	dc->PSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);
	dc->PSSetConstantBuffers(8, 1, rimLightConstantBuffer.GetAddressOf());

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

	if (!isDraw) isDraw = !isDraw;

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

		const Model::EmissiveColors modelEmissive =
			model->GetEmissiveColors();

		const DirectX::XMFLOAT4& materialEmissive =
			mesh.material->emissiveColor;

		const bool hasMaterialEmission =
			!mesh.material->emissiveTextureFileName.empty() ||
			materialEmissive.x > 0.0001f ||
			materialEmissive.y > 0.0001f ||
			materialEmissive.z > 0.0001f;

		//カラー用定数バッファ更新
		CbColor cbColor{};
		cbColor.isEmissive =
			hasMaterialEmission &&
			modelEmissive.emissiveFactor > 0.0f
			? 1.0f
			: 0.0f;
		cbColor.emissiveFactor = (std::max)(0.0f, modelEmissive.emissiveFactor);
		// エミッシブテクスチャが無い場合はシェーダー側で白として扱う
		cbColor.hasEmissiveTexture =
			mesh.material->emissiveMap ? 1.0f : 0.0f;
		// モデル側で指定した色味を使う（ImGuiのWeaponEmissiveColorが効くようになる）
		cbColor.adjustColor = modelEmissive.adjustColor;
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

		//リムライト用定数バッファ更新
		CbRimLight cbRimLight{};
		cbRimLight.rimPower = model->rimLightConstants.rimPower;
		cbRimLight.rimIntensity = model->rimLightConstants.rimIntensity;
		cbRimLight.rimColor.x = model->rimLightConstants.rimColor.x;
		cbRimLight.rimColor.y = model->rimLightConstants.rimColor.y;
		cbRimLight.rimColor.z = model->rimLightConstants.rimColor.z;
		cbRimLight.rimColor.w = model->rimLightConstants.rimColor.w;
		dc->UpdateSubresource(rimLightConstantBuffer.Get(), 0, 0, &cbRimLight, 0, 0);

		CbSetUp setUpConstants{};
		setUpConstants.IBLDiffuseScale = cbSetUp.IBLDiffuseScale;
		setUpConstants.IBLSpecularScale = cbSetUp.IBLSpecularScale;
		dc->UpdateSubresource(setUpConstantBuffer.Get(), 0, 0, &setUpConstants, 0, 0);

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
