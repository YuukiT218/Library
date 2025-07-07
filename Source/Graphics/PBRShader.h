#pragma once

#include "Shader.h"

class PBRShader : public Shader
{
public:
	PBRShader(ID3D11Device* device);
	~PBRShader() override = default;

	// 描画開始
	void Begin(const RenderContext& rc) override;

	// 更新処理
	void Update(const RenderContext& rc, const ModelResource::Mesh& mesh) override {};
	void Update(const RenderContext& rc, const std::shared_ptr<Model> model) override;

	// 描画終了
	void End(const RenderContext& rc) override;

	//デバッグ用GUI
	//void ImGui() override;

private:
	struct CbScene
	{
		DirectX::XMFLOAT4X4 viewProjection;
		DirectX::XMFLOAT4 lightDirection;
		DirectX::XMFLOAT4 lightColor;
		DirectX::XMFLOAT4 cameraPosition;
		DirectX::XMFLOAT4X4 lightViewProjection;
		DirectX::XMFLOAT4 pointLight[POINT_MAX];
		DirectX::XMFLOAT4 pointColor[POINT_MAX];
	};

	struct CbMesh
	{
		DirectX::XMFLOAT4 materialColor;
		DirectX::XMFLOAT4X4 worldTransform;
	};

	struct CbSkeleton
	{
		DirectX::XMFLOAT4X4 boneTransforms[256];
	};

	struct CbMaterial
	{
		float adjustMetalness; //  金属質調整
		float adjustRoughness; //  粗さ調整
		//float normalScale;     //法線マップのスケール
		float metalicFactor;         //金属度
		float roughnessFactor;       //粗さ
		float occlusionStrength;//強度
		float metalicindex;
		DirectX::XMFLOAT2 material_dummy;//16bite区切り用ダミー
		DirectX::XMFLOAT4 emissiveColor;//エミッシブ色
	};

	struct CbColor
	{
		float isEmissive = 0;//強調発光するかどうか
		float emissiveFactor = 0;//発光度
		DirectX::XMFLOAT2 color_dummy{};
		DirectX::XMFLOAT4 adjustColor{};//色の調整
	};

	struct CbConstants
	{
		float emissivedissolve;
		float dissolve;
		float alphaFactor;
		float dummyconstants;
		DirectX::XMFLOAT4 OverwriteColor;
	};

	struct CbSetUp
	{
		float IBLDiffuseScale = 1.0f;//ディフューズ調整用
		float IBLSpecularScale = 1.0f; //スペキュラー調整用

		DirectX::XMFLOAT2 SetUpDummy;
		DirectX::XMFLOAT4 Setupdummy;
	};
	std::unique_ptr<CbSetUp>			SetUpConstant{};

	static const int ShadowBufferSize = 4;
	struct CbShadow
	{
		DirectX::XMFLOAT4X4 CascadeLightViewProjection[ShadowBufferSize];
		DirectX::XMFLOAT4 CascadeShadowBias;

		DirectX::XMFLOAT4 cascadeFlags; // DisplayCascadeArea, IsCascade を float にし、余りを使う

		DirectX::XMFLOAT4 shadowColor;
		float shadowTexelSize;
		float shadowAttenuation;
		float shadowBias;
		float dummy;
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer>	   sceneConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>  pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>  inputLayout;

	Microsoft::WRL::ComPtr<ID3D11Buffer>	   meshConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11Buffer>	   skeletonConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11Buffer>	   materialConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11Buffer>	   colorConstantBuffer;

	//ディゾルブ
	Microsoft::WRL::ComPtr<ID3D11Buffer>	   dissolveConstantBuffer;

	//カスケードシャドウ
	Microsoft::WRL::ComPtr<ID3D11Buffer>	   shadowConstantBuffer;

	//IBL Diffuse　補正
	Microsoft::WRL::ComPtr<ID3D11Buffer>	   setUpConstantBuffer;

	//	ImageBasedLighting用テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> diffuseIemShaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> specularPmremShaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> lutGgxShaderResourceView;

	//dissolve用テクスチャ
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> emissivedissolveShaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> dissolveShaderResourceView;
};