#include "System/Misc.h"
#include "GpuResourceUtils.h"
#include "RenderContext.h"
#include "ShadowMap.h"

// コンストラクタ
ShadowMap::ShadowMap(ID3D11Device* device)
{
	// 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		// 頂点座標変換しかしないので必要な要素だけを記述。
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/ShadowMapVS.cso",
		inputElementDesc,
		_countof(inputElementDesc),
		inputLayout.GetAddressOf(),
		vertexShader.GetAddressOf());

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

	// 深度ステンシルビュー＆シェーダーリソースビューの作成
	{
		// テクスチャ作成
		D3D11_TEXTURE2D_DESC texture2dDesc{};
		texture2dDesc.Width = textureSize;
		texture2dDesc.Height = textureSize;
		texture2dDesc.MipLevels = 1;
		texture2dDesc.ArraySize = 1;
		texture2dDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
		// 深度値を保存するテクスチャ。
		// シェーダーリソースとして扱えるようにしておく
		texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texture2dDesc.CPUAccessFlags = 0;
		texture2dDesc.MiscFlags = 0;

		for (int index = 0; index < ShadowBuffer; ++index)
		{
			//テクスチャ作成
			Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
			HRESULT hr = device->CreateTexture2D(&texture2dDesc, nullptr, texture2d.GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

			// 深度ステンシルビューの作成
			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
			depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Texture2D.MipSlice = 0;
			hr = device->CreateDepthStencilView(texture2d.Get(), &depthStencilViewDesc, depthStencilView[index].GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

			// シェーダーリソースビューの作成
			D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
			shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
			shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
			shaderResourceViewDesc.Texture2D.MipLevels = 1;
			hr = device->CreateShaderResourceView(texture2d.Get(), &shaderResourceViewDesc, shaderResourceView[index].GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}
	}

	// サンプラステート
	{
		D3D11_SAMPLER_DESC desc{};
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_LESS;
		desc.MinLOD = 0;
		desc.MaxLOD = 0;
		// シャドウマップから深度値を取り出す際、範囲外(0.0～1.0以外)の場合はFloatの最大値になるようにする
		desc.BorderColor[0] = D3D11_FLOAT32_MAX;
		desc.BorderColor[1] = D3D11_FLOAT32_MAX;
		desc.BorderColor[2] = D3D11_FLOAT32_MAX;
		desc.BorderColor[3] = D3D11_FLOAT32_MAX;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		// 比較用のサンプラステートを指定
		desc.Filter = D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
		HRESULT hr = device->CreateSamplerState(&desc, samplerState.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
}

// 開始処理
void ShadowMap::Begin(const RenderContext& rc, const DirectX::XMFLOAT3& position)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// 設定中のレンダーターゲットを一時的に保持する
	UINT numViewports = 1;
	dc->RSGetViewports(&numViewports, &cacheViewport);
	dc->OMGetRenderTargets(1, cachedRenderTargetView.ReleaseAndGetAddressOf(), cachedDepthStencilView.ReleaseAndGetAddressOf());

	ID3D11ShaderResourceView* clear_shader_resource_view[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{};
	dc->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
	dc->HSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
	dc->DSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
	dc->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
	dc->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);
	dc->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clear_shader_resource_view);

	// レンダーターゲット＆深度ステンシル設定
	// 深度ステンシルビューだけ設定
	{
		dc->OMSetRenderTargets(0, nullptr, depthStencilView[0].Get());
		dc->ClearDepthStencilView(depthStencilView[0].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}
	// ビューポート設定
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(textureSize);
	viewport.Height = static_cast<float>(textureSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	dc->RSSetViewports(1, &viewport);

	// シェーダー設定
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	// ピクセルシェーダーは使用しないのでnullptrにしておく
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(inputLayout.Get());

	// レンダーステート設定
	const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Opaque), blendFactor, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(nullptr, 0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	// 定数バッファ設定
	ID3D11Buffer* constantBuffers[] =
	{
		skeletonConstantBuffer.Get(),
		sceneConstantBuffer.Get(),
	};
	dc->VSSetConstantBuffers(6, _countof(constantBuffers), constantBuffers);

	// ライトビュープロジェクション行列を作成
	// 光源から見たビュー行列を作成
	const DirectionalLight& directionalLight = rc.lightManager->GetDirectionalLight();
	DirectX::XMVECTOR LightDirection = DirectX::XMLoadFloat4(&directionalLight.direction);
	LightDirection = DirectX::XMVector3Normalize(LightDirection);
	DirectX::XMVECTOR Up = DirectX::XMVectorSet(0, 1, 0, 0);
	DirectX::XMVECTOR Focus = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR Eye = DirectX::XMVectorSubtract(Focus, DirectX::XMVectorScale(LightDirection, 50.0f));
	DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, Focus, Up);
	// プロジェクション行列は平行投影で作成
	DirectX::XMMATRIX Projection = DirectX::XMMatrixOrthographicLH(drawRect, drawRect, 0.1f, 200.0f);
	DirectX::XMMATRIX ViewProjection = DirectX::XMMatrixMultiply(View, Projection);
	DirectX::XMStoreFloat4x4(&lightViewProjection, ViewProjection);

	// シーン用定数バッファ
	CbScene cbScene;
	DirectX::XMStoreFloat4x4(&cbScene.lightViewProjection, ViewProjection);
	dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);
}

// 描画実行
void ShadowMap::Draw(const RenderContext& rc, const Model* model)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	const ModelResource* resource = model->GetResource();
	for (const ModelResource::Mesh& mesh : resource->GetMeshes())
	{
		// 頂点バッファ設定
		UINT stride = sizeof(ModelResource::Vertex);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// スケルトン用定数バッファ更新
		CbSkeleton cbSkeleton{};
		if (mesh.bones.size() > 0)
		{
			for (size_t i = 0; i < mesh.bones.size(); ++i)
			{
				const ModelResource::Bone& bone = mesh.bones.at(i);
				DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&model->GetNodes()[bone.nodeIndex].worldTransform);
				DirectX::XMMATRIX OffsetTransform = DirectX::XMLoadFloat4x4(&bone.offsetTransform);
				DirectX::XMMATRIX BoneTransform = OffsetTransform * WorldTransform;
				DirectX::XMStoreFloat4x4(&cbSkeleton.boneTransforms[i], BoneTransform);
			}
		}
		else
		{
			cbSkeleton.boneTransforms[0] = model->GetNodes()[mesh.nodeIndex].worldTransform;
		}
		rc.deviceContext->UpdateSubresource(skeletonConstantBuffer.Get(), 0, 0, &cbSkeleton, 0, 0);

		// 描画
		//dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}
}

// 終了処理
void ShadowMap::End(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	if (modelList.size() > 0) modelList.clear();

	// レンダーターゲットを元の状態に戻す
	dc->OMSetRenderTargets(0, nullptr, nullptr);
	dc->OMSetRenderTargets(1, cachedRenderTargetView.GetAddressOf(), cachedDepthStencilView.Get());
	dc->VSSetShader(nullptr, nullptr, 0);
	dc->RSSetViewports(1, &cacheViewport);
}

void ShadowMap::DrawDebugGUI()
{
	if (ImGui::CollapsingHeader("ShadowMap", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImVec2 availableSize = ImGui::GetContentRegionAvail();

		{
			// 画像のアスペクト比（例: 256x144）
			float aspectRatio = 256.0f / 144.0f;
			if (availableSize.x / availableSize.y > aspectRatio) {
				availableSize.x = availableSize.y * aspectRatio; // 高さに合わせて幅を調整
			}
			else {
				availableSize.y = availableSize.x / aspectRatio; // 幅に合わせて高さを調整
			}

			ImGui::Image(shaderResourceView[0].Get(), availableSize);
		}
		ImGui::ColorEdit4("ShadowColor", &shadowColor.x);

		ImGui::DragFloat("DrawRect", &drawRect, 1.0f, 1.0f, 1000.0f);
		ImGui::DragFloat("Attenuation", &shadowAttenuation, 0.1f);
		ImGui::DragFloat("Bias", &shadowBias, 0.0001f, 0.0f, 0.1f);
	}
}
