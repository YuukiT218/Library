#include "SkyBox.h"
#include "System/ScreenSize.h"
#include "ShaderSlot.h"
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
	skyBoxRenderSprite = std::make_unique<Sprite>(device, "Data/SkyBox/dusk_sky/EnvHDR.dds");

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

	rc.deviceContext->PSSetShaderResources(ShaderSlot::SPECULAR_PMREM_TEXTURE, 1, specularPmremShaderResourceView.GetAddressOf());
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
	skyBoxRenderSprite->Render(rc.deviceContext, 0.0f, 0.0f, 0.0f, static_cast<float>(ScreenSize::WIDTH), static_cast<float>(ScreenSize::HEIGHT), 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, skyBoxVertexShader.Get(), skyBoxPixelShader.Get());
}

void SkyBox::End(const RenderContext& rc)
{
	rc.deviceContext->VSSetShader(nullptr, nullptr, 0);
	rc.deviceContext->PSSetShader(nullptr, nullptr, 0);
	rc.deviceContext->IASetInputLayout(nullptr);
}
