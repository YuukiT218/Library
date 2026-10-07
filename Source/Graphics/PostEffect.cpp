#include <imgui.h>
#include "PostEffect.h"
#include "GpuResourceUtils.h"
#include "Graphics/Graphics.h"
#include <map>
#include <stdlib.h>

namespace
{
	// フルスクリーンクアッド（三角形ストリップ）の頂点数
	constexpr UINT FULL_SCREEN_QUAD_VERTEX_COUNT = 4;

	// ブラー用バッファのクリア色
	const DirectX::XMFLOAT4 BLUR_CLEAR_COLOR = { 1.0f, 1.0f, 1.0f, 1.0f };

	// ビネットの設定値をシェーダーに渡す値へ変換する係数
	constexpr float VIGNETTE_INTENSITY_SCALE = 3.0f;
	constexpr float VIGNETTE_SMOOTHNESS_SCALE = 5.0f;
	constexpr float VIGNETTE_MIN_SMOOTHNESS = 0.000001f;
	constexpr float VIGNETTE_MAX_ROUNDNESS = 6.0f;

	// ガウスフィルターをかけるテクスチャのサイズ
	constexpr float GAUSSIAN_TEXTURE_WIDTH = 1280.0f;
	constexpr float GAUSSIAN_TEXTURE_HEIGHT = 720.0f;
}

PostEffect::PostEffect(ID3D11Device* device)
{
	// フルスクリーンクアッド頂点シェーダー読み込み
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/FullScreenQuadVS.cso",
		nullptr, 0,
		nullptr,
		fullscreenQuadVS.ReleaseAndGetAddressOf());

	// 輝度抽出ピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		//"Data/Shader/LuminanceExtractionPS.cso",
		"Data/Shader/GlowExtractionPS.cso",
		luminanceExtractionPS.ReleaseAndGetAddressOf());

	// 定数バッファ作成
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbPostEffect),
		constantBuffer.GetAddressOf());

	// ブルームピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/BloomPS.cso",
		bloomPS.GetAddressOf());

	// ガウシアンフィルターピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/GaussianBlurPS.cso",
		gaussianPS.GetAddressOf());

	// 定数バッファ作成
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbGaussianFilter),
		gaussianConstantBuffer.GetAddressOf());

	// トーンマッピングピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/ToneMappingPS.cso",
		toneMappingPS.GetAddressOf());

	// アップサンプリングピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/GaussianBlurUpsamplingPS.cso",
		upSamplePS.GetAddressOf());

	// ダウンサンプリングピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/GaussianBlurDownsamplingPS.cso",
		downSamplePS.GetAddressOf());

	// 水平ブラーピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/GaussianBlurHorizontalPS.cso",
		horizontalPS.GetAddressOf());

	// 垂直ブラーピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/GaussianBlurVerticalPS.cso",
		verticalPS.GetAddressOf());

	// ブラー合成ピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/FinalPostPS.cso",
		finalPostPS.GetAddressOf());

	// ラジアルブラーピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/RadialBlurPS.cso",
		radialBlurPS.GetAddressOf());
	// 色収差ピクセルシェーダー読み込み
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/ChromaticAberrationPS.cso",
		chromaticPS.GetAddressOf());

	// 定数バッファ作成
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbFpost),
		cbFpostConstantBuffer.GetAddressOf());

	uint32_t width = static_cast<uint32_t>(Graphics::Instance().GetScreenWidth());
	uint32_t height = static_cast<uint32_t>(Graphics::Instance().GetScreenHeight());
	for (size_t downsampledIndex = 0; downsampledIndex < DOWNSAMPLED_COUNT; ++downsampledIndex)
	{
		gaussianBlur[downsampledIndex][0] = std::make_unique<FrameBuffer>(device, width >> downsampledIndex, height >> downsampledIndex);
		gaussianBlur[downsampledIndex][1] = std::make_unique<FrameBuffer>(device, width >> downsampledIndex, height >> downsampledIndex);
	}

	//CbPostEffect
	cbPostEffect.luminanceExtractionLowerEdge = 0.6f;
	cbPostEffect.luminanceExtractionHigherEdge = 0.8f;
	cbPostEffect.gaussianSigma = 1.0f;
	cbPostEffect.bloomIntensity = 0.2f;
	cbPostEffect.exposure = 10.0f;
	cbPostEffect.fExposureLevel = 0.55f;
	cbPostEffect.threshold = 1.25f;
	cbPostEffect.time = 0.0f;
	cbPostEffect.oldTime = 0.0f;
	//cbGaussianFilter
	cbGaussianFilter.kernelSize = 0;
	cbGaussianFilter.texcel = { 0,0 };
	gaussianFilterDatas.kernelSize = 1;
	gaussianFilterDatas.sigma = 10.0f;
	gaussianFilterDatas.textureSize = { 0,0 };
	vignetteSetData.vignetteColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	vignetteSetData.vignetteCenter = { 0.5f, 0.5f };
	vignetteSetData.vignetteIntensity = 0.0f;
	vignetteSetData.vignetteSmoothness = 0.2f;
	vignetteSetData.vignetteRounded = false;
	vignetteSetData.vignetteRoundness = 1.0f;
	cbFpost.contrast = 1.0f;
	cbFpost.saturation = 1.0f;
	cbFpost.chromatic_aberration = 0.000f;
	cbFpost.flashAmount = 0.0f;//0.0 ~　1.0で白さを調整
	cbFpost.colorFilter = { 1.05f, 0.9f, 0.9f };
	cbFpost.chromaticMask = 0.0f;
	cbFpost.flashMask = 0.0f;
	cbFpost.AccelWeight = 0.0f;//アクセルフィルターのウェイト
	cbFpost.GrayWeight = 0.0f;//グレースケール
}

void PostEffect::SetUp(float elapsedTime)
{
	//if (isParryFlash)
	//{
	//	flashTimer += elapsedTime;

	//	// 0.0～1.0で減衰
	//	float duration = 0.2f; // 200ms
	//	// FlashParams に送る
	//	if (cbFpost.flashAmount < 1.0f)
	//	{
	//		cbFpost.flashAmount = flashTimer * 11;
	//		cbFpost.chromatic_aberration = 0.050;
	//		if (radialRadius < 100.0f)radialRadius = 100.0f;
	//		cbFpost.concentratedLineDatas.intensity = 1.4f;
	//		cbFpost.concentratedLineDatas.useAnimation = 1.0f;
	//	}
	//	else
	//	{
	//		cbFpost.flashAmount = 1.0f;
	//		cbFpost.flashMask = (flashTimer - cbFpost.flashAmount / 11) * 2500;
	//		cbFpost.radialBlurDatas.maskRadius = (flashTimer - cbFpost.flashAmount / 11) * 2500;
	//		cbFpost.chromaticMask = (flashTimer - cbFpost.flashAmount / 11) * 2500;
	//		if (cbFpost.flashMask >= 1000.0f)
	//		{
	//			cbFpost.flashAmount = 0.0f;
	//			flashTimer = 0.0f;
	//			cbFpost.flashMask = 0.0f;
	//			cbFpost.chromatic_aberration = 0.005;
	//			radialRadius = 0.0f;
	//			cbFpost.concentratedLineDatas.intensity = 0.0f;
	//			cbFpost.concentratedLineDatas.useAnimation = 0.0f;
	//			isParryFlash = false;
	//		}
	//	}

	//}

	//accelTimer = Camera::Instance().GetAcceleration();

	//if (accelTimer > 0.0f)
	//{
	//	accelTimer -= elapsedTime;

	//	cbFpost.AccelWeight = 1.0f;
	//}
	//else
	//{
	//	cbFpost.AccelWeight = 0.0f;
	//}
	//Camera::Instance().SetAcceleration(accelTimer);

	//ElderDragon* dragon = EnemyManager::Instance().FindElderDragon();
	//if (dragon->GetIsRoarUsing())
	//{
	//	if (radialRadius < 200.0f)radialRadius = 200.0f;
	//}
	//static bool RoarUsed = false;
	//if (dragon->GetRoarUsed())
	//{
	//	RoarUsed = dragon->GetRoarUsed();
	//}
	//if (RoarUsed)
	//{
	//	if (radialRadius >= 0.0f)
	//	{
	//		float Minus = 200.0f;
	//		radialRadius -= elapsedTime * Minus;
	//	}
	//	else { RoarUsed = false; }
	//}

	//if (Camera::Instance().GetIsParry())
	//{
	//	cbFpost.GrayWeight = 1.0f;
	//}
	//else
	//{
	//	cbFpost.GrayWeight = 0.0f;
	//}
}

// 開始処理
void PostEffect::Begin(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;
	const RenderState* renderState = rc.renderState;

	// ブレンドステート設定
	FLOAT blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), blendFactor, 0xFFFFFFFF);

	// 深度ステンシルステート設定
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);

	// ラスタライザステート設定
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// 頂点バッファ設定（使用しない）
	dc->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	// 頂点バッファを使わない
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	dc->IASetInputLayout(nullptr);

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

	ID3D11Buffer* buffers[] =
	{
		constantBuffer.Get(),
		gaussianConstantBuffer.Get(),
		cbFpostConstantBuffer.Get(),
	};

	// 定数バッファ設定
	dc->PSSetConstantBuffers(0, _countof(buffers), buffers);
	cbPostEffect.oldTime = cbPostEffect.time;
	cbPostEffect.time = rc.timer;
	// 定数バッファ更新
	dc->UpdateSubresource(constantBuffer.Get(), 0, 0, &cbPostEffect, 0, 0);
}

// 輝度抽出処理
void PostEffect::LuminanceExtraction(const RenderContext& rc, ID3D11ShaderResourceView* colorMap)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->VSSetShader(fullscreenQuadVS.Get(), 0, 0);
	dc->PSSetShader(luminanceExtractionPS.Get(), 0, 0);

	// シェーダーリソース設定
	ID3D11ShaderResourceView* srvs[] = { colorMap };
	dc->PSSetShaderResources(0, _countof(srvs), srvs);

	// 描画
	// 4頂点で描画をする
	dc->Draw(FULL_SCREEN_QUAD_VERTEX_COUNT, 0);
}

// ブルーム処理
void PostEffect::Bloom(const RenderContext& rc, ID3D11ShaderResourceView* colorMap, ID3D11ShaderResourceView* luminanceMap)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->VSSetShader(fullscreenQuadVS.Get(), 0, 0);
	dc->PSSetShader(bloomPS.Get(), 0, 0);

	// シェーダーリソース設定
	ID3D11ShaderResourceView* srvs[] = { colorMap, luminanceMap };
	dc->PSSetShaderResources(0, _countof(srvs), srvs);

	// 描画
	dc->Draw(FULL_SCREEN_QUAD_VERTEX_COUNT, 0);
}

//ブラー
void PostEffect::KawaseBloom(const RenderContext& rc, ID3D11ShaderResourceView* colorMap, ID3D11ShaderResourceView* luminanceMap, FrameBuffer* display)
{
	ID3D11DeviceContext* dc = rc.deviceContext;
	ID3D11ShaderResourceView* nullShaderResourceView{};
	
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

	dc->VSSetShader(fullscreenQuadVS.Get(), 0, 0);
	dc->PSSetShader(downSamplePS.Get(), 0, 0);

	FrameBuffer* buffer;
	ID3D11ShaderResourceView* shaderMap;

	auto DrawBuffer = [&](int x, int y, ID3D11ShaderResourceView* map)
		{
			buffer = gaussianBlur[x][y].get();
			buffer->SetRenderTargets(dc);
			buffer->Clear(dc, BLUR_CLEAR_COLOR);
			ID3D11ShaderResourceView* resourceMap = map;
			dc->PSSetShaderResources(0, 1, &resourceMap);
			dc->Draw(FULL_SCREEN_QUAD_VERTEX_COUNT, 0);
		};

	DrawBuffer(0, 0, luminanceMap);

	dc->PSSetShader(horizontalPS.Get(), 0, 0);

	DrawBuffer(0, 1, luminanceMap);

	dc->PSSetShader(verticalPS.Get(), 0, 0);

	DrawBuffer(0, 0, gaussianBlur[0][1]->GetColorMap());

	for (size_t downsampledIndex = 1; downsampledIndex < DOWNSAMPLED_COUNT; ++downsampledIndex)
	{
		// Downsampling
		dc->PSSetShader(downSamplePS.Get(), 0, 0);

		DrawBuffer(static_cast<int>(downsampledIndex), 0, gaussianBlur[downsampledIndex - 1][0]->GetColorMap());

		// Ping-pong gaussian blur
		dc->PSSetShader(horizontalPS.Get(), 0, 0);

		DrawBuffer(static_cast<int>(downsampledIndex), 1, gaussianBlur[downsampledIndex][0]->GetColorMap());
		dc->PSSetShaderResources(0, 1, &nullShaderResourceView);

		dc->PSSetShader(verticalPS.Get(), 0, 0);

		DrawBuffer(static_cast<int>(downsampledIndex), 0, gaussianBlur[downsampledIndex][1]->GetColorMap());

	}

	// DownSampling
	FrameBuffer* luminance = Graphics::Instance().GetFrameBuffer(FrameBufferId::Luminance);
	luminance->SetRenderTargets(dc);
	luminance->Clear(dc, BLUR_CLEAR_COLOR);

	dc->PSSetShader(upSamplePS.Get(), 0, 0);

	//// シェーダーリソース設定
	std::vector<ID3D11ShaderResourceView*> shaderResourceViews;
	for (size_t downsampledIndex = 0; downsampledIndex < DOWNSAMPLED_COUNT; ++downsampledIndex)
	{
		shaderResourceViews.push_back(gaussianBlur[downsampledIndex][0]->GetColorMap());
	}
	dc->PSSetShaderResources(0, static_cast<UINT>(shaderResourceViews.size()), shaderResourceViews.data());

	dc->Draw(FULL_SCREEN_QUAD_VERTEX_COUNT, 0);

	for (int i = 0; i < DOWNSAMPLED_COUNT; ++i)
	{
		dc->PSSetShaderResources(i, 1, &nullShaderResourceView);
	}


	// 最終的に合成
	display->SetRenderTargets(dc);
	display->Clear(dc, BLUR_CLEAR_COLOR);

	dc->PSSetShader(finalPostPS.Get(), 0, 0);

	VignetteDatas constant;
	constant.vignetteColor = vignetteSetData.vignetteColor;
	constant.vignetteCenter = vignetteSetData.vignetteCenter;
	constant.vignetteIntensity = vignetteSetData.vignetteIntensity * VIGNETTE_INTENSITY_SCALE;
	constant.vignetteSmoothness = max(VIGNETTE_MIN_SMOOTHNESS, vignetteSetData.vignetteSmoothness * VIGNETTE_SMOOTHNESS_SCALE);
	constant.vignetteRounded = vignetteSetData.vignetteRounded ? 1.0f : 0.0f;
	constant.vignetteRoundness = VIGNETTE_MAX_ROUNDNESS * (1.0f - vignetteSetData.vignetteRoundness) + vignetteSetData.vignetteRoundness;

	cbFpost.vignetteData = constant;
	cbFpost.radialBlurDatas.radius = radialRadius;
	dc->UpdateSubresource(cbFpostConstantBuffer.Get(), 0, 0, &cbFpost, 0, 0);

	shaderMap = luminance->GetColorMap();
	// シェーダーリソース設定
	ID3D11ShaderResourceView* srvs[] = { colorMap, shaderMap };
	dc->PSSetShaderResources(0, _countof(srvs), srvs);

	dc->Draw(FULL_SCREEN_QUAD_VERTEX_COUNT, 0);
}

void PostEffect::GaussianFilter(const RenderContext& rc, ID3D11ShaderResourceView* colorMap)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->VSSetShader(fullscreenQuadVS.Get(), 0, 0);
	dc->PSSetShader(gaussianPS.Get(), 0, 0);

	// シェーダーリソース設定
	ID3D11ShaderResourceView* srvs[] = { colorMap };
	dc->PSSetShaderResources(0, _countof(srvs), srvs);

	//	偶数の場合は奇数に直す
	int kernelSize = gaussianFilterDatas.kernelSize;
	if (kernelSize % 2 == 0)
		kernelSize++;

	gaussianFilterDatas.textureSize.x = GAUSSIAN_TEXTURE_WIDTH;
	gaussianFilterDatas.textureSize.y = GAUSSIAN_TEXTURE_HEIGHT;

	cbGaussianFilter.kernelSize = static_cast<float>(kernelSize);
	cbGaussianFilter.texcel.x = 1.0f / gaussianFilterDatas.textureSize.x;
	cbGaussianFilter.texcel.y = 1.0f / gaussianFilterDatas.textureSize.y;
	//	重みを算出
	float sum = 0.0f;
	int id = 0;
	for (int y = -kernelSize / 2; y <= kernelSize / 2; y++)
	{
		for (int x = -kernelSize / 2; x <= kernelSize / 2; x++)
		{
			cbGaussianFilter.weights[id].x = (float)x;
			cbGaussianFilter.weights[id].y = (float)y;
			cbGaussianFilter.weights[id].z = (float)exp(-(x * x + y * y) / (2.0f * gaussianFilterDatas.sigma * gaussianFilterDatas.sigma)) / (2.0f * DirectX::XM_PI * gaussianFilterDatas.sigma);
			sum += cbGaussianFilter.weights[id].z;
			id++;
		}
	}
	//	平均化
	for (int i = 0; i < kernelSize * kernelSize; i++)
	{
		cbGaussianFilter.weights[i].z /= sum;
	}

	dc->UpdateSubresource(gaussianConstantBuffer.Get(), 0, 0, &cbGaussianFilter, 0, 0);

	// 描画
	dc->Draw(FULL_SCREEN_QUAD_VERTEX_COUNT, 0);
}

void PostEffect::ToneMapping(const RenderContext& rc, ID3D11ShaderResourceView* colorMap, ID3D11ShaderResourceView* blurMap)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->VSSetShader(fullscreenQuadVS.Get(), 0, 0);
	dc->PSSetShader(toneMappingPS.Get(), 0, 0);

	// シェーダーリソース設定
	ID3D11ShaderResourceView* srvs[] = { colorMap, blurMap };
	dc->PSSetShaderResources(0, _countof(srvs), srvs);

	// 描画
	dc->Draw(FULL_SCREEN_QUAD_VERTEX_COUNT, 0);
}

void PostEffect::RadialBlur(const RenderContext& rc, ID3D11ShaderResourceView* colorMap)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->VSSetShader(fullscreenQuadVS.Get(), 0, 0);
	dc->PSSetShader(radialBlurPS.Get(), 0, 0);

	// シェーダーリソース設定
	ID3D11ShaderResourceView* srvs[] = { colorMap };
	dc->PSSetShaderResources(0, _countof(srvs), srvs);

	// 描画
	dc->Draw(FULL_SCREEN_QUAD_VERTEX_COUNT, 0);
}

void PostEffect::ChromaticAberration(const RenderContext& rc, ID3D11ShaderResourceView* colorMap)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シェーダー設定
	dc->VSSetShader(fullscreenQuadVS.Get(), 0, 0);
	dc->PSSetShader(chromaticPS.Get(), 0, 0);

	// シェーダーリソース設定
	ID3D11ShaderResourceView* srvs[] = { colorMap };
	dc->PSSetShaderResources(0, _countof(srvs), srvs);

	// 描画
	dc->Draw(FULL_SCREEN_QUAD_VERTEX_COUNT, 0);
}

void PostEffect::Draw(const RenderContext& rc)
{
	//本来こうした方がいいけど時間ないからこんな設計にしてね！！！！！
	Graphics& graphics = Graphics::Instance();
	ID3D11DeviceContext* dc = graphics.GetDeviceContext();

	// 画面クリア＆レンダーターゲット設定
	DirectX::XMFLOAT4 color = { 0.2f, 0.2f, 0.2f, 1.0f };	// RGBA(0.0～1.0);
	std::map<FrameBufferId, FrameBuffer*> buffers;
	for (int i = 0; i < static_cast<int>(FrameBufferId::EnumCount); i++)
	{
		buffers[static_cast<FrameBufferId>(i)] = graphics.GetFrameBuffer(static_cast<FrameBufferId>(i));
	}

	buffers[FrameBufferId::Luminance]->SetRenderTargets(dc);
	LuminanceExtraction(rc, buffers[FrameBufferId::Scene]->GetColorMap());

	//川瀬の場合下記をコメントアウト
	KawaseBloom(rc, buffers[FrameBufferId::Scene]->GetColorMap(), buffers[FrameBufferId::Luminance]->GetColorMap(), buffers[FrameBufferId::RadialBlur]);

	buffers[FrameBufferId::Chromatic]->SetRenderTargets(dc);
	RadialBlur(rc, buffers[FrameBufferId::RadialBlur]->GetColorMap());

	ChromaticAberration(rc, buffers[FrameBufferId::Chromatic]->GetColorMap());
}

// 終了処理
void PostEffect::End(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// 設定されているシェーダーリソースを解除
	// シェーダーリソースの解除をせずにレンダーターゲットに書き込みをしてしまうとエラーが起きるので使用した後は解除する
	//ID3D11ShaderResourceView* srvs[] = { nullptr };
	// 設定するシェーダーリソースが2つになったので全て解除する
	ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, };
	dc->PSSetShaderResources(0, _countof(srvs), srvs);

	//Graphics::Instance().GetFrameBuffer(FrameBufferId::Display)->SetRenderTargets(dc);
}

// デバックGUI描画
void PostEffect::DrawDebugGUI()
{
	if (ImGui::CollapsingHeader("PostEffect", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("LuminanceLowerEdge", &cbPostEffect.luminanceExtractionLowerEdge, 0.01f, 0, 1.0f);
		ImGui::DragFloat("LuminanceHigherEdge", &cbPostEffect.luminanceExtractionHigherEdge, 0.01f, 0, 1.0f);
		ImGui::DragFloat("GaussianSigma", &cbPostEffect.gaussianSigma, 0.01f, 0, 10.0f);
		ImGui::DragFloat("BloomIntensity", &cbPostEffect.bloomIntensity, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("Exposure", &cbPostEffect.exposure, 0.1f, 0, 10.0f);
		ImGui::DragFloat("FExposureLevel", &cbPostEffect.fExposureLevel, 0.1f, 0, 50.0f);
		ImGui::DragFloat("Threshold", &cbPostEffect.threshold, 0.01f, 0, 10.0f);

		ImGui::SliderInt("kernel", &gaussianFilterDatas.kernelSize, 1, KERNEL_MAX);
		ImGui::SliderFloat("sigma", &gaussianFilterDatas.sigma, 1.0f, 10.0f);

		ImGui::DragFloat("contrast", &cbFpost.contrast, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("saturation", &cbFpost.saturation, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("FlashAmount", &cbFpost.flashAmount, 0.1f, 0.0f, 1.0f);
		ImGui::ColorEdit3("colorFilter", &cbFpost.colorFilter.x);

		ImGui::DragFloat("AccelWeight", &cbFpost.AccelWeight, 0.1f, 0.0f, 1.0f);
		ImGui::DragFloat("GrayWeight", &cbFpost.GrayWeight, 0.1f, 0.0f, 1.0f);

		ImGui::Checkbox("Parry", &isParryFlash);

		if (ImGui::TreeNode("chromatic aberration"))
		{
			ImGui::DragFloat("chromatic", &cbFpost.chromatic_aberration, 0.01f, 0.0f, 1.0f);

			ImGui::SliderFloat3("shift 0", &cbFpost.chromaticAberrationShift[0].x, 0, +1);
			ImGui::SliderFloat3("shift 1", &cbFpost.chromaticAberrationShift[1].x, 0, +1);
			ImGui::SliderFloat3("shift 2", &cbFpost.chromaticAberrationShift[2].x, 0, +1);

			ImGui::TreePop();
		}
		ImGui::Separator();

		if (ImGui::TreeNode("vignette"))
		{
			ImGui::ColorEdit3("color", &vignetteSetData.vignetteColor.x);
			ImGui::SliderFloat2("center", &vignetteSetData.vignetteCenter.x, 0, 1);
			ImGui::SliderFloat("intensity", &vignetteSetData.vignetteIntensity, 0.0f, +1.0f);
			ImGui::SliderFloat("smoothness", &vignetteSetData.vignetteSmoothness, 0.0f, +1.0f);
			ImGui::Checkbox("rounded", &vignetteSetData.vignetteRounded);
			ImGui::SliderFloat("roundness", &vignetteSetData.vignetteRoundness, 0.0f, +1.0f);
			ImGui::TreePop();
		}
		ImGui::Separator();

		if (ImGui::TreeNode("RadialBlurDatas"))
		{
			ImGui::DragFloat("Radius", &cbFpost.radialBlurDatas.radius, 0.1f);
			ImGui::DragInt("SamplCount", &cbFpost.radialBlurDatas.samplingCount);
			ImGui::DragFloat2("Center", &cbFpost.radialBlurDatas.center.x);
			ImGui::DragFloat("MaskRadius", &cbFpost.radialBlurDatas.maskRadius, 0.1f);

			ImGui::DragFloat2("TextureSize", &cbFpost.radialBlurDatas.textureSize.x);
			ImGui::TreePop();

		}
		ImGui::Separator();

		if (ImGui::TreeNode("ConcentratedLineDatas"))
		{
			ImGui::DragFloat("Intensity", &cbFpost.concentratedLineDatas.intensity, 0.1f);// 色調補正適応量
			static bool useAnimation = false;
			ImGui::Checkbox("UseAnimation", &useAnimation); // アニメーション使用フラグ
			cbFpost.concentratedLineDatas.useAnimation = (useAnimation) ? 1.0f : 0.0f;
			ImGui::DragFloat("AnimationSpeed", &cbFpost.concentratedLineDatas.animationSpeed, 0.1f);// アニメーション速度
			ImGui::DragFloat("PatternSeed", &cbFpost.concentratedLineDatas.patternSeed, 0.1f);// パターンシード値

			ImGui::DragFloat("NoiseScale", &cbFpost.concentratedLineDatas.noiseScale, 0.1f);// ノイズ拡大値

			ImGui::ColorEdit3("Color", &cbFpost.concentratedLineDatas.color.x);// 集中線色
			ImGui::DragFloat2("Edge", &cbFpost.concentratedLineDatas.edge.x);
			ImGui::TreePop();

		}
		ImGui::Separator();

	}
}
