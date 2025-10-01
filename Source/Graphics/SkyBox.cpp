#include "SkyBox.h"
#include "GpuResourceUtils.h"
#include "Camera/Camera.h"
#include "System/Misc.h"

SkyBox::SkyBox(ID3D11Device* device)
{
	D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
	{
		// ※並び順をVertex構造体の要素と同じにする
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	// 空用描画スプライト
	skyBoxRenderSprite = std::make_unique<Sprite>(device, "Data/SkyBox/night_sky/nightSkyEnvHDR.dds");

	// 空描画用シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/SkyBoxVS.cso",
		inputElementDesc,
		_countof(inputElementDesc),
		skyBoxInputLayout.GetAddressOf(),
		skyBoxVertexShader.GetAddressOf());

	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/SkyBoxPS.cso",
		skyBoxPixelShader.GetAddressOf());

	//コンスタントバッファのバッファ作成
// シーン用バッファ
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = sizeof(SceneConstants);
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	HRESULT hr = device->CreateBuffer(&desc, nullptr, sceneConstantBuffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
}

void SkyBox::Begin(const RenderContext& rc)
{
	//	シーン関係の情報
	SceneConstants scene{};
	//scene.options.x = static_cast<float>(cursor_position.x);
	//scene.options.y = static_cast<float>(cursor_position.y);
	//scene.options.z = timer;
	//scene.options.w = flag;
	scene.cameraPosition.x = rc.camera->GetEye().x;
	scene.cameraPosition.y = rc.camera->GetEye().y;
	scene.cameraPosition.z = rc.camera->GetEye().z;
	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
	DirectX::XMStoreFloat4x4(&scene.viewProjection, V * P);
	DirectX::XMStoreFloat4x4(&scene.inverseViewProjection, DirectX::XMMatrixInverse(nullptr, V * P));
	rc.deviceContext->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &scene, 0, 0);
	rc.deviceContext->VSSetConstantBuffers(1, 1, sceneConstantBuffer.GetAddressOf());
	rc.deviceContext->PSSetConstantBuffers(1, 1, sceneConstantBuffer.GetAddressOf());

	rc.deviceContext->PSSetShaderResources(34, 1, specular_pmrem_shader_resource_view.GetAddressOf());

	//// 必要に応じて、スカイボックス用のステート設定や準備処理を行う
	//rc.deviceContext->IASetInputLayout(skyBoxInputLayout.Get());
	//rc.deviceContext->VSSetShader(skyBoxVertexShader.Get(), nullptr, 0);
	//rc.deviceContext->PSSetShader(skyBoxPixelShader.Get(), nullptr, 0);

	//// 深度ステンシル設定の準備（スカイボックスは奥に描画するための設定）
	//rc.deviceContext->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);

	//// ラスタライザーステート設定
	//rc.deviceContext->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullNode));

}

void SkyBox::Render(const RenderContext& rc)
{
	// ブレンドステート設定
	rc.deviceContext->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	// 深度ステンシルステート設定
	rc.deviceContext->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	// ラスタライザステート設定
	rc.deviceContext->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// サンプラステート設定
	ID3D11SamplerState* samplerStates[] =
	{
		rc.renderState->GetSamplerState(SamplerState::LinearWrap),
		rc.renderState->GetSamplerState(SamplerState::Anisotropic),
		rc.renderState->GetSamplerState(SamplerState::SHADOW),
	};
	rc.deviceContext->PSSetSamplers(0, _countof(samplerStates), samplerStates);

	// シェーダー設定
	rc.deviceContext->IASetInputLayout(skyBoxInputLayout.Get());
	rc.deviceContext->VSSetShader(skyBoxVertexShader.Get(), nullptr, 0);
	rc.deviceContext->PSSetShader(skyBoxPixelShader.Get(), nullptr, 0);

	// 描画
	//skyBoxRenderSprite->Render(rc.deviceContext, 0.0f, 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT), 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	skyBoxRenderSprite->Render(rc.deviceContext, 0.0f, 0.0f, 0.0f, 1280, 720, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, skyBoxVertexShader.Get(), skyBoxPixelShader.Get());

}

void SkyBox::End(const RenderContext& rc)
{
	rc.deviceContext->VSSetShader(nullptr, nullptr, 0);
	rc.deviceContext->PSSetShader(nullptr, nullptr, 0);
	rc.deviceContext->IASetInputLayout(nullptr);

	//rc.deviceContext->PSSetShaderResources(0, nullptr, nullptr);

	// 必要に応じて、スカイボックス後の状態復元を行う
	// (例: 深度ステンシル設定やラスタライザーステートのリセット)
	//rc.context->OMSetDepthStencilState(rc.depthStencilStates[DEPTH_STATE::ZT_ON_ZW_ON].Get(), 0);
	//rc.context->RSSetState(rc.rasterizerStates[RASTER_STATE::CULL_BACK].Get());
}
