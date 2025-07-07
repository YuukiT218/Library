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
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
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

	// シーン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbScene),
		sceneConstantBuffer.GetAddressOf());

	// メッシュ用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbMesh),
		meshConstantBuffer.GetAddressOf());

	// スケルトン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSkeleton),
		skeletonConstantBuffer.GetAddressOf());

	// マテリアル用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbMaterial),
		materialConstantBuffer.GetAddressOf());

	// カラー用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbColor),
		colorConstantBuffer.GetAddressOf());

	//PBRセットアップ用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSetUp),
		setUpConstantBuffer.GetAddressOf());

	//IBLテクスチャを読み込み
	{
		D3D11_TEXTURE2D_DESC texture2dDesc{};

		GpuResourceUtils::LoadTexture(device, "Data/SkyBox/Sky/diffuse_iem.dds",
			diffuseIemShaderResourceView.GetAddressOf(), &texture2dDesc);

		GpuResourceUtils::LoadTexture(device, "Data/SkyBox/Sky/specular_pmrem.dds",
			specularPmremShaderResourceView.GetAddressOf(), &texture2dDesc);

		GpuResourceUtils::LoadTexture(device, "Data/SkyBox/Sky/lut_ggx.dds",
			lutGgxShaderResourceView.GetAddressOf(), &texture2dDesc);
	}

	// ディゾルブ定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbConstants),
		dissolveConstantBuffer.GetAddressOf());

	//ディゾルブテクスチャを読み込み
	{
		D3D11_TEXTURE2D_DESC texture2dDesc{};

		GpuResourceUtils::LoadTexture(device, "Data/Mask/dissolve_animation2.png",
			emissivedissolveShaderResourceView.GetAddressOf(), &texture2dDesc);

		GpuResourceUtils::LoadTexture(device, "Data/Mask/dissolve_animation.png",
			dissolveShaderResourceView.GetAddressOf(), &texture2dDesc);
	}

	//カスケードシャドウマップ用定数バッファ
	{
		GpuResourceUtils::CreateConstantBuffer(
			device,
			sizeof(CbShadow),
			shadowConstantBuffer.GetAddressOf());
	}

	SetUpConstant = std::make_unique<CbSetUp>();
}

// 描画開始
void PBRShader::Begin(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->IASetInputLayout(inputLayout.Get());
	rc.deviceContext->VSSetShader(vertexShader.Get(), nullptr, 0);
	rc.deviceContext->PSSetShader(pixelShader.Get(), nullptr, 0);

	// 定数バッファ設定
	ID3D11Buffer* constantBuffers[] =
	{
		sceneConstantBuffer.Get(),
		meshConstantBuffer.Get(),
		skeletonConstantBuffer.Get(),
		materialConstantBuffer.Get(),
		colorConstantBuffer.Get(),
		dissolveConstantBuffer.Get(),
		setUpConstantBuffer.Get(),
		shadowConstantBuffer.Get(),
	};
	rc.deviceContext->VSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);
	// ピクセルシェーダーにも定数バッファを設定する
	rc.deviceContext->PSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);

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

	// シーン用定数バッファ更新
	CbScene cbScene{};
	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
	DirectX::XMStoreFloat4x4(&cbScene.viewProjection, V * P);
	const DirectionalLight& directionalLight = rc.lightManager->GetDirectionalLight();
	cbScene.lightDirection.x = directionalLight.direction.x;
	cbScene.lightDirection.y = directionalLight.direction.y;
	cbScene.lightDirection.z = directionalLight.direction.z;
	cbScene.lightColor = directionalLight.color;
	const DirectX::XMFLOAT3& eye = rc.camera->GetEye();
	cbScene.cameraPosition.x = eye.x;
	cbScene.cameraPosition.y = eye.y;
	cbScene.cameraPosition.z = eye.z;
	const ShadowMap* shadowMap = rc.shadowMap;
	cbScene.lightViewProjection = shadowMap->GetLightViewProjection();
	for (int i = 0; i < POINT_MAX; i++)
	{
		cbScene.pointLight[i] = rc.lightManager->GetPointLight(i).position;
		cbScene.pointColor[i] = rc.lightManager->GetPointLight(i).color;
	}

	dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);

	//カスケードシャドウマップ
	CbShadow cbShadow{};
	float BiasCash[ShadowBufferSize]{};
	ID3D11ShaderResourceView* cascadeShadows[4];
	for (int i = 0; i < shadowMap->GetBufferSize(); i++)
	{
		cbShadow.CascadeLightViewProjection[i] = shadowMap->GetCascadeLVP(i);
		BiasCash[i] = shadowMap->GetCascadeBias(i);
		cascadeShadows[i] = shadowMap->GetShaderResourceView(i);
	}
	cbShadow.CascadeShadowBias = { BiasCash[0],BiasCash[1] ,BiasCash[2] ,BiasCash[3] };
	cbShadow.cascadeFlags.x = shadowMap->GetCascadeArea() ? 1 : 0;
	cbShadow.cascadeFlags.y = shadowMap->GetCascade() ? 1 : 0;
	//シャドウマップ
	cbShadow.shadowColor = shadowMap->GetColor();
	cbShadow.shadowAttenuation = shadowMap->GetAttenuation();
	cbShadow.shadowTexelSize = shadowMap->GetTexelSize();
	cbShadow.shadowBias = shadowMap->GetBias();


	dc->UpdateSubresource(shadowConstantBuffer.Get(), 0, 0, &cbShadow, 0, 0);
	dc->PSSetShaderResources(6, shadowMap->GetBufferSize(), cascadeShadows);
}

// 描画
void PBRShader::Update(const RenderContext& rc, const std::shared_ptr<Model> model)
{
	ID3D11DeviceContext* dc = rc.deviceContext;
	const ModelResource* resource = model->GetResource();
	const std::vector<Model::Node>& nodes = model->GetNodes();

	if (!IsDraw) IsDraw = !IsDraw;

	//ディゾルブ用定数バッファ更新
	{
		CbConstants cbconstants{};
		Model::DissolveConstants disconstants = model->GetEmissiveConstants();
		cbconstants.emissivedissolve = disconstants.emissivedissolve;
		cbconstants.dissolve = disconstants.dissolve;
		cbconstants.alphaFactor = disconstants.alphaFactor;
		cbconstants.OverwriteColor = disconstants.OverwriteColor;
		rc.deviceContext->UpdateSubresource(dissolveConstantBuffer.Get(), 0, 0, &cbconstants, 0, 0);
	}

	//セットアップ用定数バッファ更新
	{
		rc.deviceContext->UpdateSubresource(setUpConstantBuffer.Get(), 0, 0, SetUpConstant.get(), 0, 0);
	}

	for (const ModelResource::Mesh& mesh : resource->GetMeshes())
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
		// ワールド行列データをシェーダーに渡す
		//cbMesh.worldTransform = mesh.worldTransform;
		cbMesh.worldTransform = model->GetNodes()[mesh.nodeIndex].worldTransform;
		dc->UpdateSubresource(meshConstantBuffer.Get(), 0, 0, &cbMesh, 0, 0);

		//カラー用定数バッファ更新
		{
			CbColor cbcolor{};
			cbcolor.isEmissive = mesh.material->IsEmissive;
			cbcolor.emissiveFactor = model->GetEmissiveColors().emissiveFactor;
			cbcolor.adjustColor = model->GetEmissiveColors().adjustColor;
			rc.deviceContext->UpdateSubresource(colorConstantBuffer.Get(), 0, 0, &cbcolor, 0, 0);
		}

		// スケルトン用定数バッファ更新
		CbSkeleton cbSkeleton{};
		if (mesh.bones.size() > 0)
		{
			for (size_t i = 0; i < mesh.bones.size(); ++i)
			{
				const ModelResource::Bone& bone = mesh.bones.at(i);
				// ボーン行列を計算し、定数バッファに入れる
				DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&model->GetNodes()[bone.nodeIndex].worldTransform);
				DirectX::XMMATRIX OffsetTransform = DirectX::XMLoadFloat4x4(&bone.offsetTransform);
				DirectX::XMMATRIX BoneTransform = OffsetTransform * WorldTransform;
				DirectX::XMStoreFloat4x4(&cbSkeleton.boneTransforms[i], BoneTransform);
			}
		}
		else
		{
			// ボーンがない場合はワールド行列を入れる
			//cbSkeleton.boneTransforms[0] = mesh.node->worldTransform;
			cbSkeleton.boneTransforms[0] = model->GetNodes()[mesh.nodeIndex].worldTransform;
		}
		rc.deviceContext->UpdateSubresource(skeletonConstantBuffer.Get(), 0, 0, &cbSkeleton, 0, 0);

		//マテリアル用定数バッファ更新
		CbMaterial cbMaterial{};
		cbMaterial.adjustMetalness = model->GetAdMetalness();
		cbMaterial.adjustRoughness = model->GetAdRoughness();
		//cbMaterial.normalScale = 1.0f;
		cbMaterial.metalicFactor = mesh.material->metalness;
		cbMaterial.roughnessFactor = mesh.material->roughness;
		cbMaterial.occlusionStrength = 1;
		cbMaterial.emissiveColor = mesh.material->emissiveColor;
		cbMaterial.metalicindex = mesh.material->metalness;
		rc.deviceContext->UpdateSubresource(materialConstantBuffer.Get(), 0, 0, &cbMaterial, 0, 0);

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

		//IBLテクスチャを設定
		dc->PSSetShaderResources(33, 1, diffuseIemShaderResourceView.GetAddressOf());
		dc->PSSetShaderResources(34, 1, specularPmremShaderResourceView.GetAddressOf());
		dc->PSSetShaderResources(35, 1, lutGgxShaderResourceView.GetAddressOf());

		//ディゾルブテクスチャを設定
		dc->PSSetShaderResources(11, 1, emissivedissolveShaderResourceView.GetAddressOf());
		dc->PSSetShaderResources(12, 1, dissolveShaderResourceView.GetAddressOf());

		// 描画
		dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
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

//void PBRShader::ImGui()
//{
//	if (ImGui::CollapsingHeader("PBRSetUp", ImGuiTreeNodeFlags_DefaultOpen))
//	{
//		ImGui::DragFloat("IBLDiffuse", &SetUpConstant->IBLDiffuseScale, 0.1f);
//		ImGui::DragFloat("IBLSpecular", &SetUpConstant->IBLSpecularScale, 0.1f);
//	}
//}
