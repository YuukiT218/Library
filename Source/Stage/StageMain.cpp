#include "StageMain.h"
#include "Graphics/Graphics.h"
#include <execution>


#include <stdlib.h>



// コンストラクタ
StageMain::StageMain()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	model = std::make_unique<Model>(device, "Data/Model/boss_fight_arena/BattleArena.gltf", 0.15f);
	model->SetAdMetalness(1.0f);
	model->SetAdRoughness(1.0f);

	DirectX::XMVECTOR VolumeMin = DirectX::XMVectorReplicate(FLT_MAX);
	DirectX::XMVECTOR VolumeMax = DirectX::XMVectorReplicate(-FLT_MAX);

	// 頂点データをワールド空間変換し、三角形データを作成
	for (const ModelResource::Mesh& mesh : model->GetResource()->GetMeshes())
	{
		const Model::Node& node = model->GetNodes().at(mesh.nodeIndex);
		DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&node.worldTransform);
		//DirectX::XMMATRIX WorldTransform = DirectX::XMMatrixIdentity();
		for (size_t i = 0; i < mesh.indices.size(); i += 3)
		{
			// 頂点データをワールド空間変換
				//ステージがFBXのため左手系(DirectX基準)で描画するために右手系を左手系に変換している
				//そのため通常の三角形の保存方法をすると三角形が裏面になり法線が反転して
				//プレイヤーが反転する
			uint32_t a = mesh.indices.at(i + 0);//uint32_t a = mesh.indices.at(i + 2);
			uint32_t b = mesh.indices.at(i + 1);
			uint32_t c = mesh.indices.at(i + 2);//uint32_t c = mesh.indices.at(i + 0);
			DirectX::XMVECTOR A = DirectX::XMLoadFloat3(&mesh.vertices.at(a).position);
			DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&mesh.vertices.at(b).position);
			DirectX::XMVECTOR C = DirectX::XMLoadFloat3(&mesh.vertices.at(c).position);
			A = DirectX::XMVector3Transform(A, WorldTransform);
			B = DirectX::XMVector3Transform(B, WorldTransform);
			C = DirectX::XMVector3Transform(C, WorldTransform);

			// 法線ベクトルを算出
			DirectX::XMVECTOR N = DirectX::XMVector3Cross(DirectX::XMVectorSubtract(B, A), DirectX::XMVectorSubtract(C, A));
			if (DirectX::XMVector3Equal(N, DirectX::XMVectorZero()))
			{
				// 面を構成できない場合は除外
				continue;
			}
			N = DirectX::XMVector3Normalize(N);

			// 三角形データを格納
			CollisionMesh::Triangle& triangle = collisionMesh.triangles.emplace_back();
			DirectX::XMStoreFloat3(&triangle.positions[0], A);
			DirectX::XMStoreFloat3(&triangle.positions[1], B);
			DirectX::XMStoreFloat3(&triangle.positions[2], C);
			DirectX::XMStoreFloat3(&triangle.normal, N);

			// モデル全体のAABBを計測
			VolumeMin = DirectX::XMVectorMin(VolumeMin, A);
			VolumeMin = DirectX::XMVectorMin(VolumeMin, B);
			VolumeMin = DirectX::XMVectorMin(VolumeMin, C);
			VolumeMax = DirectX::XMVectorMax(VolumeMax, A);
			VolumeMax = DirectX::XMVectorMax(VolumeMax, B);
			VolumeMax = DirectX::XMVectorMax(VolumeMax, C);
		}
	}

	// モデル全体のAABB
	DirectX::XMFLOAT3 volumeMin, volumeMax;
	DirectX::XMStoreFloat3(&volumeMin, VolumeMin);
	DirectX::XMStoreFloat3(&volumeMax, VolumeMax);

	// モデル全体のAABBからXZ平面に指定のサイズで分割されたコリジョンエリアを作成する
	{
		const int cellSize = 4;
		for (float x = volumeMin.x; x < volumeMax.x; x += cellSize)
		{
			for (float z = volumeMin.z; z < volumeMax.z; z += cellSize)
			{
				//コリジョンエリア生成
				CollisionMesh::Area& area = collisionMesh.areas.emplace_back();
				area.boundingBox.Center = DirectX::XMFLOAT3(x + cellSize / 2.0f, 0.0f, z + cellSize / 2.0f);
				area.boundingBox.Extents = DirectX::XMFLOAT3(cellSize / 2.0f, (volumeMax.y - volumeMin.y) / 2.0f, cellSize / 2.0f);
			}
		}

		// 並列処理でAABBに所属する三角形を抽出
		std::for_each(std::execution::par, collisionMesh.areas.begin(), collisionMesh.areas.end(), [&](CollisionMesh::Area& area)
			{
				std::vector<int> localTriangleIndices;
				localTriangleIndices.reserve(200); // 予測される三角形の数に合わせて調整

				for (int i = 0; i < collisionMesh.triangles.size(); ++i)
				{
					const auto& triangle = collisionMesh.triangles[i];
					DirectX::BoundingBox triangleBox;
					DirectX::BoundingBox::CreateFromPoints(triangleBox, 3, triangle.positions, sizeof(DirectX::XMFLOAT3));

					if (area.boundingBox.Intersects(triangleBox))
					{
						localTriangleIndices.push_back(i);
					}
				}

				area.triangleIndices.insert(area.triangleIndices.end(), localTriangleIndices.begin(), localTriangleIndices.end());
			});


	}
}

StageMain::~StageMain()
{

}

void StageMain::Update(float elapsedTime)
{

	DirectX::XMFLOAT4X4 transform;
	DirectX::XMStoreFloat4x4(&transform, DirectX::XMMatrixIdentity());

	// モデル行列更新
	model->UpdateTransform(transform);
}

void StageMain::Render(const RenderContext& rc, ShaderId shaderId)
{
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	modelRenderer->Draw(shaderId, model, true);
	modelRenderer->Render(rc);
}

bool StageMain::RayCast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit)
{
	return Collision::IntersectRayVsModel(start, end, model.get(), hit);
}

bool StageMain::SpaceDivisionRaycast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& result)
{
	bool hit = false;
	DirectX::XMVECTOR Start = DirectX::XMLoadFloat3(&start);
	DirectX::XMVECTOR End = DirectX::XMLoadFloat3(&end);
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(End, Start);
	DirectX::XMVECTOR Direction = DirectX::XMVector3Normalize(Vec);
	float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(Vec));

	if (distance == 0) return false;

	// 空間分割したデータを使い、レイキャストを高速に処理する

	for (const auto& area : collisionMesh.areas)
	{
		// レイがエリアのAABBと交差するか確認
		float areaDistance;
		if (!area.boundingBox.Intersects(Start, Direction, areaDistance) || areaDistance > distance)
			continue;

		// エリア内の各三角形とレイキャスト判定
		for (int triangleIndex : area.triangleIndices)
		{
			const auto& triangle = collisionMesh.triangles[triangleIndex];
			DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&triangle.positions[0]);
			DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&triangle.positions[1]);
			DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&triangle.positions[2]);

			float tempDistance;
			if (DirectX::TriangleTests::Intersects(Start, Direction, v0, v1, v2, tempDistance) && tempDistance < distance)
			{
				distance = tempDistance;
				hit = true;
				DirectX::XMStoreFloat3(&result.position, DirectX::XMVectorAdd(Start, DirectX::XMVectorScale(Direction, distance)));
				result.normal = triangle.normal;
			}
		}
	}

	if (hit)
	{
		DirectX::XMVECTOR HitPosition = DirectX::XMVectorAdd(Start, DirectX::XMVectorScale(Direction, distance));
		DirectX::XMStoreFloat3(&result.position, HitPosition);
	}
	return hit;

	return false;
}

bool StageMain::SpaceDivisionSphereCast(const DirectX::XMFLOAT3& origin, const DirectX::XMFLOAT3& direction, float radius, float& distance, DirectX::XMFLOAT3& hitPosition, DirectX::XMFLOAT3& hitNormal)
{
	return false;
}

void StageMain::Debug(const RenderContext& rc)
{
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();

	// 三角形エッジ描画
	const DirectX::XMFLOAT4 edgeColor = { 0, 0, 1, 1 };
	for (CollisionMesh::Triangle& triangle : collisionMesh.triangles)
	{
		primitiveRenderer->AddVertex(triangle.positions[0], edgeColor);
		primitiveRenderer->AddVertex(triangle.positions[1], edgeColor);
		primitiveRenderer->AddVertex(triangle.positions[1], edgeColor);
		primitiveRenderer->AddVertex(triangle.positions[2], edgeColor);
		primitiveRenderer->AddVertex(triangle.positions[2], edgeColor);
		primitiveRenderer->AddVertex(triangle.positions[0], edgeColor);
	}
	primitiveRenderer->Render(rc.deviceContext, Camera::Instance().GetView(), Camera::Instance().GetProjection(), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 三角形ポリゴン描画
	const DirectX::XMFLOAT4 polygonColor = { 1, 0, 0, 0.5f };
	for (CollisionMesh::Triangle& triangle : collisionMesh.triangles)
	{
		primitiveRenderer->AddVertex(triangle.positions[0], polygonColor);
		primitiveRenderer->AddVertex(triangle.positions[1], polygonColor);
		primitiveRenderer->AddVertex(triangle.positions[2], polygonColor);
	}
	primitiveRenderer->Render(rc.deviceContext, Camera::Instance().GetView(), Camera::Instance().GetProjection(), D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// バウンディングボックス描画
	const DirectX::XMFLOAT4 boxColor = { 0, 1, 0, 1 };
	const DirectX::XMFLOAT3 boxAngle = { 0, 0, 0 };
	for (CollisionMesh::Area& area : collisionMesh.areas)
	{
		shapeRenderer->DrawBox(area.boundingBox.Center, boxAngle, area.boundingBox.Extents, boxColor);
	}
	shapeRenderer->Render(rc.deviceContext, Camera::Instance().GetView(), Camera::Instance().GetProjection());
}

void StageMain::DebugImGui()
{
	model->DebugGui(u8"メインステージ");
}