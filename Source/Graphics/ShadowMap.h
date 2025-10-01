#pragma once

#include <wrl.h>
#include <d3d11.h>
#include "Model/Model.h"

// 前方宣言
struct RenderContext;

class ShadowMap
{
public:
	ShadowMap(ID3D11Device* device);
	~ShadowMap() = default;

	// 開始処理
	void Begin(const RenderContext& rc, const DirectX::XMFLOAT3& position);

	// 描画実行
	void Draw(const RenderContext& rc, const Model* model);

	//カスケードシャドウモデル設定
	void SetShadowModel(const Model* model);

	//カスケードシャドウ用描画コマンド
	void CascadeDraw(const RenderContext& rc);

	// 終了処理
	void End(const RenderContext& rc);

	//デバッグ用ImGui
	void DrawDebugGUI();

	//カスケードシャドウマップ判別用フラグ
	bool GetCascade() const { return cascade; }

	//カスケードシャドウマップエリアデバッグ用フラグ
	bool GetCascadeArea() const { return cascadeArea; }

	// シェーダーリソースビュー取得
	ID3D11ShaderResourceView* GetShaderResourceView(int i = 0) const { return shaderResourceView[i].Get(); }

	//視錐台分割数取得
	int GetBufferSize() const { return ShadowBuffer; }

	// サンプラステート取得
	ID3D11SamplerState* GetSamplerState() const { return samplerState.Get(); }

	// ライトビュープロジェクション行列
	const DirectX::XMFLOAT4X4& GetLightViewProjection() const { return lightViewProjection; }

	//カスケードシャドウ用ライトビュープロジェクション行列
	const DirectX::XMFLOAT4X4& GetCascadeLVP(int index) const { return cascadeLightViewProjection[index]; }

	//カスケードシャドウ用バイアス
	float GetCascadeBias(int index) const { return cascadeShadowBias[index]; }

	// テクセルサイズ取得
	float GetTexelSize() const { return 1.0f / textureSize; }

	//影の強さを取得
	float GetAttenuation() const { return shadowAttenuation; }

	//影バイアス
	float GetBias() const { return shadowBias; }

	//影の色
	DirectX::XMFLOAT4 GetColor() const { return shadowColor; }

private:
	struct CbScene
	{
		DirectX::XMFLOAT4X4 lightViewProjection;
	};

	struct CbSkeleton
	{
		DirectX::XMFLOAT4X4 boneTransforms[256];
	};
	const UINT textureSize = 4096;


	float drawRect = 120.0f;//影保存用テクスチャサイズ

	float shadowAttenuation = 0.3f;//影の濃さ
	float shadowBias = 0.001f;//深度値補正

	//カスケードシャドウマップ用
		//視錘台を何分割するか
	static constexpr int ShadowBuffer = 4;

	bool cascade = false;

	bool cascadeArea = false;

	DirectX::XMFLOAT4 shadowColor{ 0.5f,0.5f,0.5f,0.0f };

	D3D11_VIEWPORT cacheViewport;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> cachedRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> cachedDepthStencilView;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView[ShadowBuffer];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView[ShadowBuffer];

	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> skeletonConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
	//通常シャドウ用
	DirectX::XMFLOAT4X4 lightViewProjection;

	//カスケードシャドウ用
	DirectX::XMFLOAT4X4 cascadeLightViewProjection[ShadowBuffer];
	float cascadeShadowBias[ShadowBuffer] = 		//深度値補正
	{
		0.0001f,
		0.0001f,
		0.0001f,
		0.0001f,
	};

	std::vector<const Model*> modelList;
};