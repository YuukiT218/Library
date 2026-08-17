#pragma once

#include <wrl.h>
#include <d3d11.h>
#include "Model/Model.h"

// 前方宣言
struct RenderContext;
class Camera;

// カスケードシャドウマップ
//
// カメラの視錐台を奥行きで複数段に分割し、手前ほど狭い範囲を
// 高い解像度で覆うことで、広い距離を扱いつつ影の粗さを抑える。
class ShadowMap
{
public:
	// 視錐台の分割数
	static constexpr int CASCADE_COUNT = 4;

	ShadowMap(ID3D11Device* device);
	~ShadowMap() = default;

	// 開始処理（カスケードの行列を計算し、描画設定を行う）
	void Begin(const RenderContext& rc, const DirectX::XMFLOAT3& position);

	// 影を落とすモデルを登録する
	// カスケードごとに描画し直すため、この時点では描画せず溜めておく
	void Draw(const RenderContext& rc, const Model* model);

	// 終了処理（登録されたモデルをカスケードごとに描画する）
	void End(const RenderContext& rc);

	// デバッグ用ImGui
	void DrawDebugGUI();

	// シェーダーリソースビュー取得（4枚分のテクスチャ配列）
	ID3D11ShaderResourceView* GetShaderResourceView() const { return shaderResourceView.Get(); }

	// サンプラステート取得
	ID3D11SamplerState* GetSamplerState() const { return samplerState.Get(); }

	// カスケード0のライトビュープロジェクション行列
	const DirectX::XMFLOAT4X4& GetLightViewProjection() const { return cascadeLightViewProjection[0]; }

	// 指定カスケードのライトビュープロジェクション行列
	const DirectX::XMFLOAT4X4& GetCascadeLightViewProjection(int index) const { return cascadeLightViewProjection[index]; }

	// 各カスケードの終端距離（ビュー空間）
	const DirectX::XMFLOAT4& GetCascadeSplits() const { return cascadeSplits; }

	// 各カスケードのシャドウマップ1テクセルのワールドサイズ
	const DirectX::XMFLOAT4& GetCascadeTexelWorldSize() const { return cascadeTexelWorldSize; }

	// テクセルサイズ取得
	float GetTexelSize() const { return 1.0f / textureSize; }

	// 影の濃さ取得
	float GetAttenuation() const { return shadowAttenuation; }

	// 深度値補正取得
	float GetBias() const { return shadowBias; }

	// 法線方向へずらす量取得
	float GetNormalOffset() const { return shadowNormalOffset; }

	// 間接光に影を効かせる割合取得
	float GetIndirectShadowStrength() const { return indirectShadowStrength; }

	// 影の色取得
	const DirectX::XMFLOAT4& GetColor() const { return shadowColor; }

	// カスケードを色分け表示するか
	bool IsCascadeDebugView() const { return cascadeDebugView; }

private:
	struct CbScene
	{
		DirectX::XMFLOAT4X4 lightViewProjection;
	};

	struct CbSkeleton
	{
		DirectX::XMFLOAT4X4 boneTransforms[256];
	};

	// カメラの視錐台から各カスケードのライト行列を求める
	void UpdateCascades(const RenderContext& rc);

	// 登録済みモデルを1枚のシャドウマップへ描画する
	void DrawModel(const RenderContext& rc, const Model* model);

private:
	// シャドウマップ1枚あたりの解像度
	const UINT textureSize = 2048;

	// 影を落とす最大距離（これより奥はシャドウマップの対象外）
	float shadowDistance = 120.0f;

	// 分割位置を対数分割寄りにするか均等分割寄りにするかの割合
	// 0で均等分割、1で対数分割
	float splitLambda = 0.85f;

	// 視錐台より手前にある影の落とし主も含めるための余白
	float casterMargin = 80.0f;

	float shadowAttenuation = 1.0f; // 影の濃さ
	float shadowBias = 0.0008f;     // 深度値補正

	// シャドウマップを引く位置を法線方向へずらす量（テクセル数）
	// ワールド固定量にすると、カスケードによってテクセル数百個分ずれてしまい
	// 曲面の多いキャラクターで影の縁がとげ状になるため、テクセル基準で指定する
	float shadowNormalOffset = 2.0f;

	// 間接光(IBL)に影をどれだけ効かせるか
	// 0だと影の中でも間接光が満額残り、法線マップの陰影に影が埋もれてしまう
	float indirectShadowStrength = 0.75f;

	bool cascadeDebugView = false;  // カスケードを色分け表示する

	DirectX::XMFLOAT4 shadowColor{ 0.1f, 0.1f, 0.1f, 0.0f };

	D3D11_VIEWPORT cacheViewport;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> cachedRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> cachedDepthStencilView;

	// 4枚をまとめたテクスチャ配列と、その各スライスへの深度ステンシルビュー
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView[CASCADE_COUNT];

	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> skeletonConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;

	// カスケードごとのライトビュープロジェクション行列
	DirectX::XMFLOAT4X4 cascadeLightViewProjection[CASCADE_COUNT];

	// 各カスケードの終端距離（ビュー空間）
	DirectX::XMFLOAT4 cascadeSplits{};

	// 各カスケードのシャドウマップ1テクセルのワールドサイズ
	DirectX::XMFLOAT4 cascadeTexelWorldSize{};

	// このフレームで影を落とすモデル
	std::vector<const Model*> modelList;
};
