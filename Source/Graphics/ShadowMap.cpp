#include "System/Misc.h"
#include "GpuResourceUtils.h"
#include "RenderContext.h"
#include "ShadowMap.h"
#include "ShaderSlot.h"
#include "Camera/Camera.h"

#include <algorithm>

namespace
{
	// ライト方向が真上・真下に近いときにLookAtが破綻しないよう上方向を切り替える閾値
	constexpr float UP_VECTOR_SWITCH_THRESHOLD = 0.99f;

	// 境界球の半径を量子化する単位（カメラの微小な動きで影が揺れるのを防ぐ）
	constexpr float RADIUS_QUANTIZE_STEP = 16.0f;
}


// コンストラクタ
ShadowMap::ShadowMap(ID3D11Device* device)
{
	// 入力レイアウト
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		// ボーン影響データを追加
		// ※並び順をVertex構造体の要素と同じにする
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONE_WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "BONE_INDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
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
		// カスケード分をまとめた1枚のテクスチャ配列として作成する
		D3D11_TEXTURE2D_DESC texture2dDesc{};
		texture2dDesc.Width = textureSize;
		texture2dDesc.Height = textureSize;
		texture2dDesc.MipLevels = 1;
		texture2dDesc.ArraySize = CASCADE_COUNT;
		texture2dDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		texture2dDesc.SampleDesc.Count = 1;
		texture2dDesc.SampleDesc.Quality = 0;
		texture2dDesc.Usage = D3D11_USAGE_DEFAULT;
		// 深度値を保存するテクスチャ。
		// シェーダーリソースとして扱えるようにしておく
		texture2dDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		texture2dDesc.CPUAccessFlags = 0;
		texture2dDesc.MiscFlags = 0;

		Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
		HRESULT hr = device->CreateTexture2D(&texture2dDesc, nullptr, texture2d.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

		// スライスごとに深度ステンシルビューを作成する
		for (int index = 0; index < CASCADE_COUNT; ++index)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
			depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			depthStencilViewDesc.Texture2DArray.MipSlice = 0;
			depthStencilViewDesc.Texture2DArray.FirstArraySlice = index;
			depthStencilViewDesc.Texture2DArray.ArraySize = 1;
			hr = device->CreateDepthStencilView(texture2d.Get(), &depthStencilViewDesc, depthStencilView[index].GetAddressOf());
			_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
		}

		// シェーダーリソースビューは配列全体を参照する
		D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc{};
		shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
		shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2DArray.MipLevels = 1;
		shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
		shaderResourceViewDesc.Texture2DArray.ArraySize = CASCADE_COUNT;
		hr = device->CreateShaderResourceView(texture2d.Get(), &shaderResourceViewDesc, shaderResourceView.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// サンプラステート
	{
		// シェーダー側で深度値を直接読んで比較するため、比較用ではなく通常のサンプラにする
		D3D11_SAMPLER_DESC desc{};
		desc.MipLODBias = 0.0f;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		// シャドウマップの範囲外を参照した場合は影にならないよう最大深度を返す
		desc.BorderColor[0] = D3D11_FLOAT32_MAX;
		desc.BorderColor[1] = D3D11_FLOAT32_MAX;
		desc.BorderColor[2] = D3D11_FLOAT32_MAX;
		desc.BorderColor[3] = D3D11_FLOAT32_MAX;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		HRESULT hr = device->CreateSamplerState(&desc, samplerState.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}
}

// カメラの視錐台から各カスケードのライト行列を求める
void ShadowMap::UpdateCascades(const RenderContext& rc)
{
	const Camera* camera = rc.camera;

	// 光源から見た向き
	const DirectionalLight& directionalLight = rc.lightManager->GetDirectionalLight();
	DirectX::XMFLOAT3 dir = { directionalLight.direction.x, directionalLight.direction.y, directionalLight.direction.z };
	DirectX::XMVECTOR LightDirection = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&dir));

	// ライトが真上・真下を向いていると上方向が定まらないので切り替える
	float verticality = fabsf(DirectX::XMVectorGetY(LightDirection));
	DirectX::XMVECTOR Up = (verticality > UP_VECTOR_SWITCH_THRESHOLD)
		? DirectX::XMVectorSet(0, 0, 1, 0)
		: DirectX::XMVectorSet(0, 1, 0, 0);

	float nearZ = camera->GetNearZ();
	float farZ = (std::min)(camera->GetFarZ(), shadowDistance);

	const int activeCount = GetActiveCascadeCount();

	// 分割位置を求める
	float splitDistances[CASCADE_COUNT + 1];
	splitDistances[0] = nearZ;

	if (activeCount <= 1)
	{
		// 従来方式：1枚のシャドウマップで影の届く範囲すべてを覆う
		for (int i = 1; i <= CASCADE_COUNT; ++i)
		{
			splitDistances[i] = farZ;
		}
	}
	else
	{
		// 分割位置を対数分割と均等分割の混合で求める
		// 手前ほど細かく分割されるので、近くの影ほど解像度が高くなる
		for (int i = 1; i <= CASCADE_COUNT; ++i)
		{
			float ratio = static_cast<float>(i) / CASCADE_COUNT;
			float logSplit = nearZ * powf(farZ / nearZ, ratio);
			float uniformSplit = nearZ + (farZ - nearZ) * ratio;
			splitDistances[i] = splitLambda * logSplit + (1.0f - splitLambda) * uniformSplit;
		}
	}

	for (int i = 0; i < activeCount; ++i)
	{
		// この段が担当する視錐台の8頂点
		DirectX::XMFLOAT3 corners[Camera::FRUSTUM_CORNER_COUNT];
		camera->GetFrustumCorners(splitDistances[i], splitDistances[i + 1], corners);

		// 視錐台を包む境界球を求める
		// 球で覆うとカメラが回転しても大きさが変わらないため、影のちらつきを防げる
		DirectX::XMVECTOR Center = DirectX::XMVectorZero();
		for (const DirectX::XMFLOAT3& corner : corners)
		{
			Center = DirectX::XMVectorAdd(Center, DirectX::XMLoadFloat3(&corner));
		}
		Center = DirectX::XMVectorScale(Center, 1.0f / _countof(corners));

		float radius = 0.0f;
		for (const DirectX::XMFLOAT3& corner : corners)
		{
			DirectX::XMVECTOR Diff = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&corner), Center);
			radius = (std::max)(radius, DirectX::XMVectorGetX(DirectX::XMVector3Length(Diff)));
		}
		// 半径を量子化して、微小な変化で影が揺れないようにする
		radius = ceilf(radius * RADIUS_QUANTIZE_STEP) / RADIUS_QUANTIZE_STEP;

		// 光源を球の外側に置き、球全体を見下ろすビュー行列を作る
		DirectX::XMVECTOR Eye = DirectX::XMVectorAdd(Center, DirectX::XMVectorScale(LightDirection, radius + casterMargin));
		DirectX::XMMATRIX View = DirectX::XMMatrixLookAtLH(Eye, Center, Up);

		// 影がテクセル単位で動くようにスナップする
		// 中心をライト空間へ移すと必ず原点になるため、
		// スナップ量はワールド座標をライトの向きで測った値から求める
		float texelSize = (radius * 2.0f) / textureSize;
		DirectX::XMVECTOR Right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(Up, LightDirection));
		DirectX::XMVECTOR LightUp = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(LightDirection, Right));
		float centerAlongRight = DirectX::XMVectorGetX(DirectX::XMVector3Dot(Center, Right));
		float centerAlongUp = DirectX::XMVectorGetX(DirectX::XMVector3Dot(Center, LightUp));
		float snapOffsetX = floorf(centerAlongRight / texelSize) * texelSize - centerAlongRight;
		float snapOffsetY = floorf(centerAlongUp / texelSize) * texelSize - centerAlongUp;
		View = DirectX::XMMatrixMultiply(View, DirectX::XMMatrixTranslation(snapOffsetX, snapOffsetY, 0.0f));

		// 平行投影。視錐台より手前の影の落とし主も含めるため奥行きに余白を取る
		DirectX::XMMATRIX Projection = DirectX::XMMatrixOrthographicLH(
			radius * 2.0f, radius * 2.0f, 0.0f, radius * 2.0f + casterMargin);

		DirectX::XMStoreFloat4x4(&cascadeLightViewProjection[i], DirectX::XMMatrixMultiply(View, Projection));

		// シェーダー側で法線オフセットをテクセル基準で計算するために記録しておく
		(&cascadeTexelWorldSize.x)[i] = texelSize;
	}

	// 使わない段には0段目と同じ設定を入れておく
	// （シェーダー側が誤って参照しても破綻しないようにするため）
	for (int i = activeCount; i < CASCADE_COUNT; ++i)
	{
		cascadeLightViewProjection[i] = cascadeLightViewProjection[0];
		(&cascadeTexelWorldSize.x)[i] = cascadeTexelWorldSize.x;
	}

	// 各カスケードの終端距離をシェーダーへ渡すためにまとめる
	cascadeSplits.x = splitDistances[1];
	cascadeSplits.y = splitDistances[2];
	cascadeSplits.z = splitDistances[3];
	cascadeSplits.w = splitDistances[4];
}

// 開始処理
void ShadowMap::Begin(const RenderContext& rc, const DirectX::XMFLOAT3& position)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// 設定中のレンダーターゲットを一時的に保持する
	UINT numViewports = 1;
	dc->RSGetViewports(&numViewports, &cacheViewport);
	dc->OMGetRenderTargets(1, cachedRenderTargetView.ReleaseAndGetAddressOf(), cachedDepthStencilView.ReleaseAndGetAddressOf());

	// シャドウマップをシェーダーリソースから外しておく
	ID3D11ShaderResourceView* clearShaderResourceViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{};
	dc->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clearShaderResourceViews);
	dc->HSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clearShaderResourceViews);
	dc->DSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clearShaderResourceViews);
	dc->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clearShaderResourceViews);
	dc->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clearShaderResourceViews);
	dc->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, clearShaderResourceViews);

	// カスケードごとのライト行列を計算する
	UpdateCascades(rc);

	// すべてのカスケードの深度をクリアしておく
	for (int i = 0; i < CASCADE_COUNT; ++i)
	{
		dc->ClearDepthStencilView(depthStencilView[i].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
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
	dc->VSSetConstantBuffers(ShaderSlot::SKELETON_CONSTANT_BUFFER, _countof(constantBuffers), constantBuffers);
}

// 影を落とすモデルを登録する
void ShadowMap::Draw(const RenderContext& rc, const Model* model)
{
	if (model == nullptr) return;

	// カスケードごとに描き直す必要があるため、ここでは溜めておく
	modelList.emplace_back(model);
}

// 登録済みモデルを1枚のシャドウマップへ描画する
void ShadowMap::DrawModel(const RenderContext& rc, const Model* model)
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
		// 未使用スロットはゼロ行列だと頂点が原点に潰れて
		// 横に伸びた三角形になってしまうため、単位行列で埋めておく
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
		dc->UpdateSubresource(skeletonConstantBuffer.Get(), 0, 0, &cbSkeleton, 0, 0);

		// 描画
		dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	}
}

// 終了処理
void ShadowMap::End(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// カスケードごとに、登録されたモデルをすべて描画する
	const int activeCount = GetActiveCascadeCount();
	for (int i = 0; i < activeCount; ++i)
	{
		dc->OMSetRenderTargets(0, nullptr, depthStencilView[i].Get());

		CbScene cbScene;
		cbScene.lightViewProjection = cascadeLightViewProjection[i];
		dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);

		for (const Model* model : modelList)
		{
			DrawModel(rc, model);
		}
	}

	modelList.clear();

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
		// ポートフォリオ用の比較スクリーンショット向けの切り替え
		ImGui::Checkbox("CascadeShadow", &cascadeEnabled);
		ImGui::SameLine();
		ImGui::TextDisabled(cascadeEnabled ? "(4段カスケード)" : "(通常シャドウマップ1枚)");

		ImGui::Checkbox("CascadeDebugView", &cascadeDebugView);

		ImGui::ColorEdit4("ShadowColor", &shadowColor.x);
		ImGui::DragFloat("Attenuation", &shadowAttenuation, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Bias", &shadowBias, 0.0001f, 0.0f, 0.05f, "%.4f");
		ImGui::DragFloat("NormalOffset(texel)", &shadowNormalOffset, 0.1f, 0.0f, 16.0f, "%.2f");
		ImGui::DragFloat("IndirectShadow", &indirectShadowStrength, 0.01f, 0.0f, 1.0f);

		ImGui::Separator();
		ImGui::DragFloat("ShadowDistance", &shadowDistance, 1.0f, 10.0f, 1000.0f);
		ImGui::DragFloat("SplitLambda", &splitLambda, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("CasterMargin", &casterMargin, 1.0f, 0.0f, 500.0f);

		ImGui::Text("Splits: %.1f / %.1f / %.1f / %.1f",
			cascadeSplits.x, cascadeSplits.y, cascadeSplits.z, cascadeSplits.w);
	}
}
