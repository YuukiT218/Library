#pragma once

#include "Model/Model.h"
#include "stage.h"

// メインステージ
class StageMain : public Stage
{
public:
	StageMain();
	~StageMain() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画更新
	void Render(const RenderContext& rc, ShaderId shaderId) override;

	// レイキャスト
	bool RayCast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit) override;
	bool SpaceDivisionRaycast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit) override;
	bool SpaceDivisionSphereCast(const DirectX::XMFLOAT3& origin, const DirectX::XMFLOAT3& direction, float radius, float& distance, DirectX::XMFLOAT3& hitPosition, DirectX::XMFLOAT3& hitNormal) override;

	void Debug(const RenderContext& rc) override;

	void DebugImGui() override;

};