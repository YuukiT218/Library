#include <algorithm>
#include "System/Misc.h"
#include "Graphics/GpuResourceUtils.h"
#include "ModelRenderer.h"
#include "Graphics/BasicShader.h"
#include "Graphics/LambertShader.h"
#include "Graphics/PBRShader.h"


#include <stdlib.h>




// コンストラクタ
ModelRenderer::ModelRenderer(ID3D11Device* device)
{
	// シーン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbScene),
		sceneConstantBuffer.GetAddressOf());

	// スケルトン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSkeleton),
		skeletonConstantBuffer.GetAddressOf());

	// テレポート用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbTeleport),
		teleportConstantBuffer.GetAddressOf());

	// シェーダー生成
	shaders[static_cast<int>(ShaderId::Basic)] = std::make_unique<BasicShader>(device);
	shaders[static_cast<int>(ShaderId::Lambert)] = std::make_unique<LambertShader>(device);
	shaders[static_cast<int>(ShaderId::PBR)] = std::make_unique<PBRShader>(device);

	// ノイズテクスチャ生成
	GenerateNoiseTexture(device);
}

void ModelRenderer::GenerateNoiseTexture(ID3D11Device* device)
{
	const int width = 256;
	const int height = 256;
	std::vector<uint8_t> noiseData(width * height);

	// シンプルなランダムノイズ生成
	srand(0); // 固定シード（同じパターンを生成）
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			// より滑らかなノイズパターン
			int index = y * width + x;
			float fx = static_cast<float>(x) / width;
			float fy = static_cast<float>(y) / height;

			// 複数の周波数を重ねる
			float noise = 0.0f;
			noise += (rand() % 256) / 255.0f * 0.5f;
			noise += sin(fx * 6.28f * 4.0f) * 0.25f + 0.25f;
			noise += cos(fy * 6.28f * 3.0f) * 0.25f + 0.25f;

			noiseData[index] = static_cast<uint8_t>(noise * 255.0f);
		}
	}

	// テクスチャ作成
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = noiseData.data();
	initData.SysMemPitch = width;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	HRESULT hr = device->CreateTexture2D(&desc, &initData, texture.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

	// ShaderResourceView作成
	hr = device->CreateShaderResourceView(texture.Get(), nullptr, noiseTextureSRV.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
}

// 箱描画
void ModelRenderer::Draw(ShaderId shaderId, std::shared_ptr<Model> model, bool enableDither)
{
	DrawInfo& drawInfo = drawInfos.emplace_back();
	drawInfo.shaderId = shaderId;
	drawInfo.model = model;
	drawInfo.hasTeleportEffect = false;
	drawInfo.enableDither = enableDither;
}

void ModelRenderer::DrawWithTeleport(ShaderId shaderId, std::shared_ptr<Model> model,
	float teleportProgress, float teleportTime,
	TeleportRenderMode mode)
{
	DrawInfo& drawInfo = drawInfos.emplace_back();
	drawInfo.shaderId = shaderId;
	drawInfo.model = model;
	drawInfo.hasTeleportEffect = true;
	drawInfo.teleportMode = mode;

	// テレポートデータを設定
	drawInfo.teleportData.teleportProgress = teleportProgress;
	drawInfo.teleportData.teleportTime = teleportTime;
	drawInfo.teleportData.dissolveEdgeWidth = 0.1f;
	drawInfo.teleportData.distortionIntensity = 0.5f;
	drawInfo.teleportData.dissolveEdgeColor = DirectX::XMFLOAT3(0.8f, 0.3f, 1.0f);
	drawInfo.teleportData.afterimageAlpha = 0.0f;
	drawInfo.teleportData.afterimageDarkness = 0.8f;

	switch (mode)
	{
	case TeleportRenderMode::DissolveDistortion:
		drawInfo.teleportData.enableDissolve = 1.0f;
		drawInfo.teleportData.enableDistortion = 1.0f;
		break;

	case TeleportRenderMode::FadeOnly:
		drawInfo.teleportData.enableDissolve = 0.0f;
		drawInfo.teleportData.enableDistortion = 0.0f;
		// アルファ値だけ設定
		drawInfo.teleportData.afterimageAlpha = 1.0f - teleportProgress;
		break;

	default:
		drawInfo.teleportData.enableDissolve = 0.0f;
		drawInfo.teleportData.enableDistortion = 0.0f;
		break;
	}

	auto& nodes = model->GetNodes();
	if (!nodes.empty())
	{
		drawInfo.teleportData.teleportCenterY = nodes[0].worldTransform._42 + 1.0f;
	}
}

void ModelRenderer::DrawAfterimage(ShaderId shaderId, std::shared_ptr<Model> model,
	const std::vector<Model::Node>& nodes,
	const DirectX::XMFLOAT4X4& transform,
	float alpha, float darkness, const DirectX::XMFLOAT4& color)
{
	DrawInfo& drawInfo = drawInfos.emplace_back();
	drawInfo.shaderId = shaderId;
	drawInfo.model = model;
	drawInfo.hasTeleportEffect = false;
	drawInfo.isAfterimage = true;
	drawInfo.afterimageNodes = nodes;
	drawInfo.afterimageTransform = transform;
	drawInfo.afterimageAlpha = alpha;
	drawInfo.afterimageColor = color;

	// テレポートデータは空で初期化
	drawInfo.teleportData = {};
	drawInfo.teleportData.teleportProgress = 0.0f;
	drawInfo.teleportData.teleportTime = 0.0f;
	drawInfo.teleportData.dissolveEdgeWidth = 0.0f;
	drawInfo.teleportData.distortionIntensity = 0.0f;
	drawInfo.teleportData.dissolveEdgeColor = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	drawInfo.teleportData.teleportCenterY = 0.0f;
	drawInfo.teleportData.afterimageAlpha = alpha;
	drawInfo.teleportData.afterimageDarkness = darkness;
	drawInfo.teleportData.afterimageColor = color;
	drawInfo.teleportData.enableDissolve = 0.0f;
	drawInfo.teleportData.enableDistortion = 0.0f;
}

// 粒子分解用の描画
// 姿勢を固定して描く仕組みは残像と同じだが、
// 残像のような色の塗り潰しはせず、モデルの見た目のままディゾルブで削っていく
void ModelRenderer::DrawDisintegration(ShaderId shaderId, std::shared_ptr<Model> model,
	const std::vector<Model::Node>& nodes,
	float dissolveProgress, float dissolveTime,
	const DirectX::XMFLOAT3& edgeColor,
	float edgeWidth)
{
	DrawInfo& drawInfo = drawInfos.emplace_back();
	drawInfo.shaderId = shaderId;
	drawInfo.model = model;
	drawInfo.hasTeleportEffect = false;
	drawInfo.isAfterimage = true;
	drawInfo.afterimageNodes = nodes;
	DirectX::XMStoreFloat4x4(&drawInfo.afterimageTransform, DirectX::XMMatrixIdentity());

	// アルファは1.0のまま。消えていくのはディゾルブの担当なので、
	// ここを下げるとシェーダー側で残像の色に塗り替えられてしまう
	drawInfo.afterimageAlpha = 1.0f;
	drawInfo.afterimageColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	drawInfo.teleportData = {};
	drawInfo.teleportData.teleportProgress = dissolveProgress;
	drawInfo.teleportData.teleportTime = dissolveTime;
	drawInfo.teleportData.dissolveEdgeWidth = edgeWidth;
	drawInfo.teleportData.distortionIntensity = 0.0f;
	drawInfo.teleportData.dissolveEdgeColor = edgeColor;
	drawInfo.teleportData.teleportCenterY = 0.0f;
	drawInfo.teleportData.afterimageAlpha = 1.0f;
	drawInfo.teleportData.afterimageDarkness = 0.0f;
	drawInfo.teleportData.afterimageColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	drawInfo.teleportData.enableDissolve = 1.0f;
	drawInfo.teleportData.enableDistortion = 0.0f;
}

void ModelRenderer::DrawWithAlpha(ShaderId shaderId, std::shared_ptr<Model> model, float alpha)
{
	DrawInfo& drawInfo = drawInfos.emplace_back();
	drawInfo.shaderId = shaderId;
	drawInfo.model = model;
	drawInfo.hasTeleportEffect = false;
	drawInfo.isAfterimage = false;

	// アルファ値だけ設定
	drawInfo.teleportData = {};
	drawInfo.teleportData.teleportProgress = 0.0f;
	drawInfo.teleportData.teleportTime = 0.0f;
	drawInfo.teleportData.dissolveEdgeWidth = 0.0f;
	drawInfo.teleportData.distortionIntensity = 0.0f;
	drawInfo.teleportData.dissolveEdgeColor = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	drawInfo.teleportData.teleportCenterY = 0.0f;
	drawInfo.teleportData.afterimageAlpha = alpha;        // アルファ値を設定
	drawInfo.teleportData.afterimageDarkness = 0.0f;      // 暗くしない
	drawInfo.teleportData.enableDissolve = 0.0f;
	drawInfo.teleportData.enableDistortion = 0.0f;
}

// 描画実行
void ModelRenderer::Render(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シーン用定数バッファ更新
	{
		static LightManager defaultLightManager;
		const LightManager* lightManager = rc.lightManager ? rc.lightManager : &defaultLightManager;

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

		cbScene.targetPosition.x = rc.targetPosition.x;
		cbScene.targetPosition.y = rc.targetPosition.y;
		cbScene.targetPosition.z = rc.targetPosition.z;
		cbScene.targetPosition.w = rc.enableWallTransparency ? 1.0f : 0.0f;

		dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);
	}

	// 定数バッファ設定
	ID3D11Buffer* vsConstantBuffers[] =
	{
		skeletonConstantBuffer.Get(),
		sceneConstantBuffer.Get(),
		nullptr,
		teleportConstantBuffer.Get(),
	};
	ID3D11Buffer* psConstantBuffers[] =
	{
		sceneConstantBuffer.Get(),
		nullptr,
		teleportConstantBuffer.Get(),
	};
	dc->VSSetConstantBuffers(6, _countof(vsConstantBuffers), vsConstantBuffers);
	dc->PSSetConstantBuffers(7, _countof(psConstantBuffers), psConstantBuffers);

	// サンプラステート設定
	ID3D11SamplerState* samplerStates[] =
	{
		rc.renderState->GetSamplerState(SamplerState::LinearWrap),
		rc.renderState->GetSamplerState(SamplerState::Anisotropic),
		rc.renderState->GetSamplerState(SamplerState::SHADOW),
	};
	dc->PSSetSamplers(0, _countof(samplerStates), samplerStates);

	// ノイズテクスチャをt36にバインド
	dc->PSSetShaderResources(36, 1, noiseTextureSRV.GetAddressOf());

	// レンダーステート設定
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	// メッシュ描画関数
	auto drawMesh = [&](std::vector<Model::Node> nodes, const ModelResource::Mesh& mesh,
		Shader* shader, const std::shared_ptr<Model> model,
		bool hasTeleport, TeleportRenderMode teleportMode, const CbTeleport& teleportData,
		bool isAfterimage, float afterimageAlpha, const DirectX::XMFLOAT4& afterimageColor,
		bool enableDither)
	{
		// テレポートエフェクト or 残像アルファ設定
		CbTeleport effectData = teleportData;

		if (isAfterimage)
		{
			// 残像・粒子分解は姿勢を止めて描くだけなので、頂点を歪ませる処理は使わない
			// ディゾルブ関係(teleportProgress / dissolveEdge*)は予約時の指定をそのまま使う
			// （DrawAfterimageは0で予約しているので、これまでの残像の見た目は変わらない）
			effectData.distortionIntensity = 0.0f;
			effectData.teleportCenterY = 0.0f;
			effectData.afterimageAlpha = afterimageAlpha;  // アルファ値を設定
			effectData.afterimageColor = afterimageColor;
			effectData.enableDistortion = 0.0f;
		}
		else if (hasTeleport)
		{
			effectData = teleportData;
		}
		else
		{
			effectData.afterimageAlpha = 1.0f;
			effectData.afterimageDarkness = 0.0f;
			effectData.afterimageColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		}
		effectData.enableDither = enableDither ? 1.0f : 0.0f;

		dc->UpdateSubresource(teleportConstantBuffer.Get(), 0, 0, &effectData, 0, 0);

		// 頂点バッファ設定
		UINT stride = sizeof(ModelResource::Vertex);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// スケルトン用定数バッファ更新
		// 未使用スロットはゼロ行列だと頂点が原点に潰れてしまうため単位行列で埋める
		CbSkeleton cbSkeleton;
		for (DirectX::XMFLOAT4X4& boneTransform : cbSkeleton.boneTransforms)
		{
			DirectX::XMStoreFloat4x4(&boneTransform, DirectX::XMMatrixIdentity());
		}

		if (mesh.bones.size() > 0)
		{
			// 定数バッファの範囲を超えないように制限する
			const size_t boneCount = (std::min)(mesh.bones.size(), static_cast<size_t>(MAX_BONES));
			for (size_t i = 0; i < boneCount; ++i)
			{
				const ModelResource::Bone& bone = mesh.bones.at(i);
				DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&nodes[bone.nodeIndex].worldTransform);
				DirectX::XMMATRIX OffsetTransform = DirectX::XMLoadFloat4x4(&bone.offsetTransform);
				DirectX::XMMATRIX BoneTransform = OffsetTransform * WorldTransform;
				DirectX::XMStoreFloat4x4(&cbSkeleton.boneTransforms[i], BoneTransform);
			}
		}
		else
		{
			cbSkeleton.boneTransforms[0] = nodes[mesh.nodeIndex].worldTransform;
		}
		dc->UpdateSubresource(skeletonConstantBuffer.Get(), 0, 0, &cbSkeleton, 0, 0);

		// 更新
		shader->Update(rc, mesh, model);

		// ノイズテクスチャをt36にバインド
		// Shader::End()で全スロットのシェーダーリソースが解除されるため、
		// 描画直前に毎回張り直しておく
		// (張り直さないと2つ目以降の描画予約でノイズが真っ黒になり、
		//  ステージのディザ透過やディゾルブが効かなくなる)
		dc->PSSetShaderResources(36, 1, noiseTextureSRV.GetAddressOf());

		// 描画
		dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	};

	DirectX::XMVECTOR CameraPosition = DirectX::XMLoadFloat3(&rc.camera->GetEye());
	DirectX::XMVECTOR CameraFront = DirectX::XMLoadFloat3(&rc.camera->GetFront());

	// ブレンドステート設定
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);

	// 不透明描画処理
	for (DrawInfo& drawInfo : drawInfos)
	{
		Shader* shader = shaders[static_cast<int>(drawInfo.shaderId)].get();
		shader->Begin(rc);

		auto& nodes = drawInfo.isAfterimage ? drawInfo.afterimageNodes : drawInfo.model->GetNodes();
		for (const ModelResource::Mesh& mesh : drawInfo.model->GetResource()->GetMeshes())
		{
			// 半透明メッシュ登録
			if (mesh.material->alphaMode == ModelResource::AlphaMode::Blend ||
				(mesh.material->baseColor.w > 0.01f && mesh.material->baseColor.w < 0.99f) || 
				drawInfo.isAfterimage)
			{
				TransparencyDrawInfo& transparencyDrawInfo = transparencyDrawInfos.emplace_back();
				transparencyDrawInfo.model = drawInfo.model;
				transparencyDrawInfo.nodes = nodes;
				transparencyDrawInfo.mesh = &mesh;
				transparencyDrawInfo.shaderId = drawInfo.shaderId;
				transparencyDrawInfo.hasTeleportEffect = drawInfo.hasTeleportEffect;
				transparencyDrawInfo.teleportData = drawInfo.teleportData;
				transparencyDrawInfo.isAfterimage = drawInfo.isAfterimage;
				transparencyDrawInfo.afterimageAlpha = drawInfo.afterimageAlpha;
				transparencyDrawInfo.teleportMode = drawInfo.teleportMode;
				// 半透明メッシュでも障害物のディザ透過を効かせる
				transparencyDrawInfo.enableDither = drawInfo.enableDither;
				// カメラとの距離を算出
				DirectX::XMVECTOR Position = DirectX::XMVectorSet(
					nodes[mesh.nodeIndex].worldTransform._41,
					nodes[mesh.nodeIndex].worldTransform._42,
					nodes[mesh.nodeIndex].worldTransform._43,
					0.0f);
				DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Position, CameraPosition);
				transparencyDrawInfo.distance = DirectX::XMVectorGetX(DirectX::XMVector3Dot(CameraFront, Vec));

				continue;
			}

			// 描画
			drawMesh(nodes, mesh, shader, drawInfo.model,
				drawInfo.hasTeleportEffect, drawInfo.teleportMode, drawInfo.teleportData,
				drawInfo.isAfterimage, drawInfo.afterimageAlpha, drawInfo.afterimageColor,
				drawInfo.enableDither);
		}

		shader->End(rc);
	}
	drawInfos.clear();

	// ブレンドステート設定
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);

	// カメラから遠い順にソート
	std::sort(transparencyDrawInfos.begin(), transparencyDrawInfos.end(),
		[](const TransparencyDrawInfo& lhs, const TransparencyDrawInfo& rhs)
		{
			return lhs.distance > rhs.distance;
		});

	// 半透明描画処理
	for (const TransparencyDrawInfo& transparencyDrawInfo : transparencyDrawInfos)
	{
		Shader* shader = shaders[static_cast<int>(transparencyDrawInfo.shaderId)].get();

		shader->Begin(rc);

		drawMesh(transparencyDrawInfo.nodes, *transparencyDrawInfo.mesh, shader, transparencyDrawInfo.model,
			transparencyDrawInfo.hasTeleportEffect, transparencyDrawInfo.teleportMode, transparencyDrawInfo.teleportData,
			transparencyDrawInfo.isAfterimage, transparencyDrawInfo.afterimageAlpha,
			transparencyDrawInfo.teleportData.afterimageColor,
			transparencyDrawInfo.enableDither);

		shader->End(rc);
	}
	transparencyDrawInfos.clear();

	// 定数バッファ設定解除
	for (ID3D11Buffer*& vsConstantBuffer : vsConstantBuffers) { vsConstantBuffer = nullptr; }
	for (ID3D11Buffer*& psConstantBuffer : psConstantBuffers) { psConstantBuffer = nullptr; }
	dc->VSSetConstantBuffers(6, _countof(vsConstantBuffers), vsConstantBuffers);
	dc->PSSetConstantBuffers(7, _countof(psConstantBuffers), psConstantBuffers);

	// サンプラステート設定解除
	for (ID3D11SamplerState*& samplerState : samplerStates) { samplerState = nullptr; }
	dc->PSSetSamplers(0, _countof(samplerStates), samplerStates);

	// ノイズテクスチャ解除
	ID3D11ShaderResourceView* nullSRV = nullptr;
	dc->PSSetShaderResources(36, 1, &nullSRV);
}
