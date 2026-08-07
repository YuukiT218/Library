#pragma once

#include <memory>
#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "Model.h"
#include "Graphics/Shader.h"

enum class ShaderId
{
	Basic,
	Lambert,
	PBR,

	EnumCount
};

enum class TeleportRenderMode
{
	None,              // エフェクトなし
	DissolveDistortion,// ディゾルブ+ディストーション
	FadeOnly,          // フェードのみ（残像用）
};

class ModelRenderer
{
public:
	ModelRenderer(ID3D11Device* device);
	~ModelRenderer() {}

	// 予約描画
	void Draw(ShaderId shaderId, std::shared_ptr<Model> model, bool enableDither = false);

	void DrawWithTeleport(ShaderId shaderId, std::shared_ptr<Model> model,
		float teleportProgress, float teleportTime,
		TeleportRenderMode mode = TeleportRenderMode::DissolveDistortion);

	void DrawAfterimage(ShaderId shaderId, std::shared_ptr<Model> model,
		const std::vector<Model::Node>& nodes,
		const DirectX::XMFLOAT4X4& transform,
		float alpha, float darkness = 0.8f);

	void DrawWithAlpha(ShaderId shaderId, std::shared_ptr<Model> model, float alpha);

	// 描画実行
	void Render(const RenderContext& rc);

private:
	struct CbScene
	{
		DirectX::XMFLOAT4X4		viewProjection;
		DirectX::XMFLOAT4		lightDirection;
		DirectX::XMFLOAT4		lightColor;
		DirectX::XMFLOAT4		cameraPosition;
		DirectX::XMFLOAT4X4 lightViewProjection;
		DirectX::XMFLOAT4 pointLight[POINT_MAX];
		DirectX::XMFLOAT4 pointColor[POINT_MAX];
		DirectX::XMFLOAT4       targetPosition;
	};

	struct CbSkeleton
	{
		DirectX::XMFLOAT4X4		boneTransforms[256];
	};

	struct CbTeleport
	{
		float teleportProgress;      // 0.0〜1.0
		float teleportTime;          // アニメーション時間
		float dissolveEdgeWidth;     // ディゾルブエッジ幅
		float distortionIntensity;   // 歪み強度

		DirectX::XMFLOAT3 dissolveEdgeColor;  // エッジの発光色
		float teleportCenterY;       // テレポート中心のY座標

		float afterimageAlpha;
		float afterimageDarkness;
		float enableDissolve;
		float enableDistortion;
		float enableDither;
		DirectX::XMFLOAT3 teleportDummy;
	};

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

	struct DrawInfo
	{
		ShaderId				shaderId;
		std::shared_ptr<Model>	model;

		bool hasTeleportEffect = false;
		TeleportRenderMode teleportMode = TeleportRenderMode::None;
		CbTeleport teleportData = {};
		bool isAfterimage = false;
		std::vector<Model::Node> afterimageNodes;
		DirectX::XMFLOAT4X4 afterimageTransform;
		float afterimageAlpha = 1.0f;
		float afterimageDarkness = 0.8f;
		bool enableDither = false;
	};

	struct TransparencyDrawInfo
	{
		ShaderId				shaderId;
		std::shared_ptr<Model>  model;
		std::vector<Model::Node> nodes = {};
		const ModelResource::Mesh*		mesh;
		float					distance;

		bool hasTeleportEffect = false;
		TeleportRenderMode teleportMode = TeleportRenderMode::None;
		CbTeleport teleportData = {};
		bool isAfterimage = false;
		float afterimageAlpha = 1.0f;
		float afterimageDarkness = 0.8f;
		bool enableDither = false;
	};

	std::unique_ptr<Shader>					shaders[static_cast<int>(ShaderId::EnumCount)];
	std::vector<DrawInfo>					drawInfos;
	std::vector<TransparencyDrawInfo>		transparencyDrawInfos;

	Microsoft::WRL::ComPtr<ID3D11Buffer>	sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	skeletonConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>    teleportConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> noiseTextureSRV;

	void GenerateNoiseTexture(ID3D11Device* device);
};
