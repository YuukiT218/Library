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
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
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
	if (!cascade)
	{
		dc->OMSetRenderTargets(0, nullptr, depthStencilView[0].Get());
		dc->ClearDepthStencilView(depthStencilView[0].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}
	else
	{
		for (int i = 0; i < ShadowBuffer; i++)
		{
			dc->ClearDepthStencilView(depthStencilView[i].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		}
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
		sceneConstantBuffer.Get(),
		nullptr,
		skeletonConstantBuffer.Get(),
	};
	dc->VSSetConstantBuffers(0, _countof(constantBuffers), constantBuffers);

	// ライトビュープロジェクション行列を作成
	// 光源から見たビュー行列を作成
	const DirectionalLight& directionalLight = rc.lightManager->GetDirectionalLight();
	DirectX::XMVECTOR LightDirection = DirectX::XMLoadFloat3(&directionalLight.direction);
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
		dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}
}

void ShadowMap::SetShadowModel(const Model* model)
{
	if (model)
	{
		modelList.push_back(model); // 順番を保持して追加
	}
}

void ShadowMap::CascadeDraw(const RenderContext& rc)
{
	const Camera* camera = rc.camera;

	ID3D11DeviceContext* dc = rc.deviceContext;

	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::LinearAlpha), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::Solid));

	DirectX::XMVECTOR CameraUp{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) }, CameraRight, CameraFront, CameraPosition;
	{
		CameraPosition = DirectX::XMLoadFloat3(&camera->GetEye());
		//ビュー行列（View Matrix）作成
			//XMMatrixLookAtLH を使ってビュー行列を作成する
			//LookAtLH は左手座標系（Left-Handed）での視点行列を構築する関数です。
		DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(CameraPosition,
			DirectX::XMLoadFloat3(&camera->GetFocus()),
			CameraUp);
		//ビュー行列の逆行列
			//つまりカメラのワールド行列（World Matrix）
		//ビュー行列は、「カメラから見た座標系へ変換」するための行列
		//つまり、ワールド空間 → ビュー空間への変換です。
		//例えば、カメラがワールド空間の (10, 0, 0) にあったとし
		/*ビュー行列はこれを原点(0, 0, 0) に移動させるための変換
			その逆行列は、「ビュー空間の原点（カメラ自身）を、ワールド空間の(10, 0, 0) に戻す」変換になります。
			つまり、カメラがワールド空間でどこにあり、どの方向を向いているかを示す行列（ワールド行列）になります。*/
		View = DirectX::XMMatrixInverse(nullptr, View);
		//ワールド空間におけるカメラ右、上、前
		CameraRight = DirectX::XMVector3Normalize(View.r[0]);
		CameraUp = DirectX::XMVector3Normalize(View.r[1]);
		CameraFront = DirectX::XMVector3Normalize(View.r[2]);
	}

	// ライトの位置から見た視線行列を生成
	const DirectionalLight& directionalLight = rc.lightManager->GetDirectionalLight();
	DirectX::XMVECTOR LightPosition = DirectX::XMLoadFloat3(&directionalLight.direction);
	LightPosition = DirectX::XMVectorScale(LightPosition, -250);
	//ライトから見た視線行列　ビュー行列（View Matrix）作成
	DirectX::XMMATRIX LightView = DirectX::XMMatrixLookAtLH(LightPosition,
		DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

	// シャドウマップに描画したい範囲の射影行列を生成
		//XMMatrixOrthographicLH
			//直交射影行列（Orthographic Projection）LH は左手座標系（Left-Handed）
			//直交射影とは、カメラやライトの視点から物体を投影する際に、遠近感がない投影方法
	DirectX::XMMATRIX Projection = DirectX::XMMatrixOrthographicLH(
		drawRect,//これは射影矩形の幅 シャドウマップに描画したい範囲の幅
		drawRect,//これは射影矩形の高さ 
		0.1f,//近距離クリッピング面 描画対象として扱う最小の距離
		1000.0f);//遠距離クリッピング面 どれくらい遠くの物体まで影を描画するか

	// ライトビュープロジェクション行列を保存
	DirectX::XMMATRIX LightViewProjection;
	LightViewProjection = LightView * Projection;

	//	シャドウマップの分割エリア定義
	static constexpr float SplitAreaTable[] =
	{
		0.1f,
		25.0f,
		100.0f,
		250.0f,
		500.0f,
	};

	//カメラ画角
	float fov_y = camera->GetFov();
	//シーンスクリーン大きさ
	float aspect_ratio = static_cast<float>(cacheViewport.Width) / static_cast<float>(cacheViewport.Height);
	for (int index = 0; index < ShadowBuffer; ++index)
	{
		// シャドウマップ用の深度バッファに設定
		dc->ClearDepthStencilView(depthStencilView[index].Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		dc->OMSetRenderTargets(0, nullptr, depthStencilView[index].Get());

		float	near_depth = SplitAreaTable[index + 0];
		float	far_depth = SplitAreaTable[index + 1];

		//	エリアを内包する視推台の8頂点を算出する
		DirectX::XMVECTOR	vertex[8];
		{
			//	エリアの近平面の中心からの上面までの距離を求める
			float	nearY = tanf(fov_y * 0.5f) * near_depth;

			//	エリアの近平面の中心からの右面までの距離を求める
			float	nearX = nearY * aspect_ratio;

			//	エリアの遠平面の中心からの上面までの距離を求める
			float	farY = tanf(fov_y * 0.5f) * far_depth;

			//	エリアの遠平面の中心からの右面までの距離を求める
			float	farX = farY * aspect_ratio;

			//	エリアの近平面の中心座標を求める
			DirectX::XMVECTOR	NearPosition = DirectX::XMVectorAdd(CameraPosition,
				DirectX::XMVectorScale(CameraFront, near_depth));

			//	エリアの遠平面の中心座標を求める
			DirectX::XMVECTOR	FarPosition = DirectX::XMVectorAdd(CameraPosition,
				DirectX::XMVectorScale(CameraFront, far_depth));

			//	8頂点を求める
			{
				// 近平面の右上
				vertex[0] = DirectX::XMVectorAdd(NearPosition,
					DirectX::XMVectorAdd(
						DirectX::XMVectorScale(CameraUp, nearY),
						DirectX::XMVectorScale(CameraRight, nearX)));
				// 近平面の左上
				vertex[1] = DirectX::XMVectorAdd(NearPosition,
					DirectX::XMVectorAdd(
						DirectX::XMVectorScale(CameraUp, nearY),
						DirectX::XMVectorScale(CameraRight, -nearX)));
				// 近平面の右下
				vertex[2] = DirectX::XMVectorAdd(NearPosition,
					DirectX::XMVectorAdd(
						DirectX::XMVectorScale(CameraUp, -nearY),
						DirectX::XMVectorScale(CameraRight, nearX)));
				// 近平面の左下
				vertex[3] = DirectX::XMVectorAdd(NearPosition,
					DirectX::XMVectorAdd(
						DirectX::XMVectorScale(CameraUp, -nearY),
						DirectX::XMVectorScale(CameraRight, -nearX)));

				// 遠平面の右上
				vertex[4] = DirectX::XMVectorAdd(FarPosition,
					DirectX::XMVectorAdd(DirectX::XMVectorScale(CameraUp, farY),
						DirectX::XMVectorScale(CameraRight, farX)));
				// 遠平面の左上
				vertex[5] = DirectX::XMVectorAdd(FarPosition,
					DirectX::XMVectorAdd(DirectX::XMVectorScale(CameraUp, farY),
						DirectX::XMVectorScale(CameraRight, -farX)));
				// 遠平面の右下
				vertex[6] = DirectX::XMVectorAdd(FarPosition,
					DirectX::XMVectorAdd(DirectX::XMVectorScale(CameraUp, -farY),
						DirectX::XMVectorScale(CameraRight, farX)));
				// 遠平面の左下
				vertex[7] = DirectX::XMVectorAdd(FarPosition,
					DirectX::XMVectorAdd(DirectX::XMVectorScale(CameraUp, -farY),
						DirectX::XMVectorScale(CameraRight, -farX)));
			}
		}

		//	8頂点をライトビュープロジェクション空間に変換して、最大値、最小値を求める
		DirectX::XMFLOAT2	vertex_min(FLT_MAX, FLT_MAX), vertex_max(-FLT_MAX, -FLT_MAX);
		for (auto& it : vertex)
		{
			DirectX::XMFLOAT3	p;
			DirectX::XMStoreFloat3(&p, DirectX::XMVector3TransformCoord(it, LightViewProjection));

			vertex_min.x = min(p.x, vertex_min.x);
			vertex_min.y = min(p.y, vertex_min.y);
			vertex_max.x = max(p.x, vertex_max.x);
			vertex_max.y = max(p.y, vertex_max.y);
		}

		//	クロップ行列を求める
		DirectX::XMMATRIX	ClopMatrix = DirectX::XMMatrixIdentity();
		{
			float	xScale = 2.0f / (vertex_max.x - vertex_min.x);
			float	yScale = 2.0f / (vertex_max.y - vertex_min.y);
			float	xOffset = -0.5f * (vertex_max.x + vertex_min.x) * xScale;
			float	yOffset = -0.5f * (vertex_max.y + vertex_min.y) * yScale;
			DirectX::XMFLOAT4X4	clopMatrix;
			DirectX::XMStoreFloat4x4(&clopMatrix, ClopMatrix);
			clopMatrix._11 = xScale;
			clopMatrix._22 = yScale;
			clopMatrix._41 = xOffset;
			clopMatrix._42 = yOffset;
			ClopMatrix = DirectX::XMLoadFloat4x4(&clopMatrix);
		}

		//	ライトビュープロジェクション行列にクロップ行列を乗算
		DirectX::XMFLOAT4X4 light_view_projection;
		DirectX::XMStoreFloat4x4(&light_view_projection, LightViewProjection * ClopMatrix);

		cascadeLightViewProjection[index] = light_view_projection;

		//	シーン定数バッファの更新
		{

			CbScene cbScene;
			cbScene.lightViewProjection = light_view_projection;
			dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);
		}

		for (int i = 0; i < modelList.size(); i++)
		{
			const Model* model = modelList.at(i);

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
				dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
			}
		}
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
	if (ImGui::CollapsingHeader("ShadoMap", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImVec2 availableSize = ImGui::GetContentRegionAvail();

		ImGui::Checkbox("CascadeShadowMap", &cascade);

		ImGui::Checkbox("CascadeArea", &cascadeArea);

		if (cascade)
		{
			for (int index = 0; index < ShadowBuffer; index++)
			{

				//テクスチャ毎に表示
				if (ImGui::CollapsingHeader((std::string("shadow_map") + std::to_string(index)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
				{

					// 画像のアスペクト比（例: 256x144）
					float aspectRatio = 256.0f / 144.0f;
					if (availableSize.x / availableSize.y > aspectRatio) {
						availableSize.x = availableSize.y * aspectRatio; // 高さに合わせて幅を調整
					}
					else {
						availableSize.y = availableSize.x / aspectRatio; // 幅に合わせて高さを調整
					}

					ImGui::Image(shaderResourceView[index].Get(), availableSize);
				}

				ImGui::DragFloat((std::string("CascadeBias") + std::to_string(index)).c_str(), &cascadeShadowBias[index], 0.0001f, 0.0f, 0.1f);
			}

		}
		else
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
