#pragma once

#include "Graphics/Graphics.h"
#include "Math/Collision.h"
#include <DirectXCollision.h>
#include <imgui.h>

// ステージ
class Stage
{
public:
	Stage() {};
	virtual ~Stage() {};

	// 更新処理
	virtual void Update(float elapsedTime) = 0;

	// 描画処理
	virtual void Render(const RenderContext& rc, ShaderId shaderId) = 0;

	// レイキャスト
	virtual bool RayCast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit) = 0;
	//空間分割レイキャスト
	virtual bool SpaceDivisionRaycast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit) { return false; }
	virtual bool SpaceDivisionSphereCast(const DirectX::XMFLOAT3& origin, const DirectX::XMFLOAT3& direction, float radius, float& distance, DirectX::XMFLOAT3& hitPosition, DirectX::XMFLOAT3& hitNormal) { return false; }

	//デバッグ用分割された空間の表示
	virtual void Debug(const RenderContext& rc) = 0;

	//デバッグ用
	virtual void DebugImGui() = 0;

	Model* GetModel() const { return model.get(); }

	bool IsDivisionEnabled() const { return isDivisionStage; }
	void SetDivisionEnabled(bool enabled) { isDivisionStage = enabled; }

protected:
	struct CollisionMesh
	{
		struct Triangle
		{
			DirectX::XMFLOAT3	positions[3];
			DirectX::XMFLOAT3	normal;
		};
		struct Area
		{
			DirectX::BoundingBox	boundingBox;
			std::vector<int>		triangleIndices;
		};

		std::vector<Triangle>	triangles;
		std::vector<Area>		areas;
	};


protected:
	//空間分割用判定メッシュ
	CollisionMesh	collisionMesh;

	//空間分割可能なステージかどうか
	bool isDivisionStage = true;

	//モデル
	std::shared_ptr<Model> model;
};