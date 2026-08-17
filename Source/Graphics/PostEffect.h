#pragma once

#include <wrl.h>
#include <d3d11.h>
#include "RenderContext.h"
#include "FrameBuffer.h"

class PostEffect
{
public:
	PostEffect(ID3D11Device* device);

	// 定数アップデート用
	void SetUp(float elapsedTime);

	// 開始処理
	void Begin(const RenderContext& rc);

	// 輝度抽出処理
	//void LuminanceExtraction(const RenderContext& rc);
	void LuminanceExtraction(const RenderContext& rc, ID3D11ShaderResourceView* colorMap);

	// ブルーム処理
	void Bloom(const RenderContext& rc, ID3D11ShaderResourceView* colorMap, ID3D11ShaderResourceView* luminanceMap);

	//川瀬ブラー
	void KawaseBloom(const RenderContext& rc, ID3D11ShaderResourceView* colorMap, ID3D11ShaderResourceView* luminanceMap, FrameBuffer* display);

	// ガウスフィルター処理
	void GaussianFilter(const RenderContext& rc, ID3D11ShaderResourceView* colorMap);

	//トーンマッピング処理
	void ToneMapping(const RenderContext& rc, ID3D11ShaderResourceView* colorMap, ID3D11ShaderResourceView* blurMap);

	//ラジアルブラー
	void RadialBlur(const RenderContext& rc, ID3D11ShaderResourceView* colorMap);

	//色収差
	void ChromaticAberration(const RenderContext& rc, ID3D11ShaderResourceView* colorMap);

	//描画処理
	void Draw(const RenderContext& rc);

	// 終了処理
	void End(const RenderContext& rc);

	// デバックGUI描画
	void DrawDebugGUI();

private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> fullscreenQuadVS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> luminanceExtractionPS;

	struct CbPostEffect
	{
		float luminanceExtractionLowerEdge = 0.6f;
		float luminanceExtractionHigherEdge = 0.8f;
		// 定数バッファは16バイトアライメントでしか作成できないのでパディングする。
		//float padding[2];
		float gaussianSigma = 1.0f;
		float bloomIntensity = 0.3f;

		float exposure = 1.2f;
		float fExposureLevel = 0.5f;
		float threshold = 0.8f;//闘値
		//時間
		float time = 0;//現在の時間
		float oldTime = 0;//前フレームの時間

		//今後も実装していく
		DirectX::XMFLOAT3 postDummy;
	};
	CbPostEffect cbPostEffect;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;

	// ガウスフィルター
#define KERNEL_MAX 25
	struct CbGaussianFilter
	{
		DirectX::XMFLOAT4 weights[KERNEL_MAX * KERNEL_MAX]{};
		float kernelSize = 0;
		DirectX::XMFLOAT2 texcel = { 0, 0 };
		float dummy = 0;
	};
	CbGaussianFilter cbGaussianFilter;
	struct GaussianFilterDatas
	{
		int kernelSize{ 1 };
		float sigma{ 10.0f };
		DirectX::XMFLOAT2 textureSize = { 0, 0 };
	};
	GaussianFilterDatas gaussianFilterDatas;

	//	ラジアルブラー情報
	struct RadialBlurDatas
	{
		float radius = 0.0f;
		int samplingCount = 10;
		DirectX::XMFLOAT2 center = { 0.5f, 0.5f };

		float maskRadius = 0.0f;
		DirectX::XMFLOAT2 textureSize{};

		float dummy;
	};

	//	集中線処理
	struct ConcentratedLineDatas
	{
		float intensity = 0.0f; // 色調補正適応量
		float useAnimation = 1.0f; // アニメーション使用フラグ
		float animationSpeed = 10.0f; // アニメーション速度
		float patternSeed = 10.0f; // パターンシード値

		float noiseScale = 500.0f; // ノイズ拡大値
		DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f); // 集中線色
		DirectX::XMFLOAT2 edge = DirectX::XMFLOAT2(0.5f, 1.0f);
		DirectX::XMFLOAT2 dummy;
	};

	struct VignetteSetData
	{
		DirectX::XMFLOAT4 vignetteColor = { 0.2f, 0.2f, 0.2f, 1.0f };
		DirectX::XMFLOAT2 vignetteCenter = { 0.5f, 0.5f };
		float vignetteIntensity = 0.0f;
		float vignetteSmoothness = 0.2f;

		bool vignetteRounded = false;
		float vignetteRoundness = 1.0f;
	};
	VignetteSetData vignetteSetData;

	//ビネット　減光処理
	struct VigenetteDatas
	{
		DirectX::XMFLOAT4 vignetteColor{ 0.0f,0.0f,0.0f,1.0f };
		DirectX::XMFLOAT2 vignetteCenter{ 0.5f,0.5f };
		float vignetteIntensity = 0.0f;
		float vignetteSmoothness = 0.0f;

		float vignetteRounded = 0.0f;
		float vignetteRoundness = 0.0f;
		float vignetteOpacity = 0.0f;
		float vignette_dummy;
	};

	//最後に画面に描画させるポストエフェクト
	struct CbFpost
	{
		float contrast = 1.2f;
		float saturation = 1.4f;
		float chromatic_aberration = 0.006f;
		float flashAmount = 0.0f;//0.0 ~　1.0で白さを調整

		DirectX::XMFLOAT4 chromaticAberrationShift[3] =
		{
			{1.0f, 0.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, 0.0f, 0.0f},
			{0.0f, 0.0f, 1.0f, 0.0f}
		};

		DirectX::XMFLOAT3 colorFilter = { 1.05f, 0.9f, 0.9f };
		float chromaticMask = 0.0f;

		float flashMask = 0.0f;

		float AccelWeight = 0.0f;//アクセルフィルターのウェイト

		float GrayWeight = 0.0f;//グレースケール

		float dummy; //4個区切り用

		RadialBlurDatas radialBlurDatas{};

		ConcentratedLineDatas concentratedLineDatas{};

		VigenetteDatas vignetteData{};
	};
	CbFpost cbFpost;

	bool isParryFlash = false;
	bool Acceleration = false;

	float flashTimer = 0;
	float accelTimer = 0;

	float radialRadius = 0;

	Microsoft::WRL::ComPtr<ID3D11Buffer> gaussianconstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> cbFpostconstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11PixelShader> bloomPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussianPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> toneMappingPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  downSamplePS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  upSamplePS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  horizontalPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  verticalPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  FinalPostPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  RadialBlurPS;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  ChromaticPS;

	static const size_t downsampled_count = 6;
	std::unique_ptr<FrameBuffer> gaussian_blur[downsampled_count][2];
	std::unique_ptr<FrameBuffer> glow_extraction;
};