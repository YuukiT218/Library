#include "Collision.h"

bool Collision::IntersectSphereVsSphere(
    const DirectX::XMFLOAT3& positionA,
    float radiusA,
    const DirectX::XMFLOAT3& positionB,
    float radiusB,
    DirectX::XMFLOAT3& outPositionB,
    DirectX::XMFLOAT3& outHitPoint)
{
    // A→Bの単位ベクトルを算出
    DirectX::XMVECTOR PositionA = DirectX::XMLoadFloat3(&positionA);
    DirectX::XMVECTOR PositionB = DirectX::XMLoadFloat3(&positionB);
    DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(PositionB, PositionA);
    DirectX::XMVECTOR LengthSq = DirectX::XMVector3LengthSq(Vec);
    float lengthSq;
    DirectX::XMStoreFloat(&lengthSq, LengthSq);

    // 距離判定
    float range = radiusA + radiusB;
    if (lengthSq > range * range)
    {
        return false;
    }

    // AがBを押し出す
    Vec = DirectX::XMVector3Normalize(Vec);
    Vec = DirectX::XMVectorScale(Vec, range);
    PositionB = DirectX::XMVectorAdd(PositionA, Vec);
    DirectX::XMStoreFloat3(&outPositionB, PositionB);

    // 衝突点（Aの表面上）を計算
    DirectX::XMVECTOR HitPoint = DirectX::XMVectorAdd(PositionA, DirectX::XMVectorScale(Vec, radiusA));
    DirectX::XMStoreFloat3(&outHitPoint, HitPoint);

    return true;
}

// 球と球の重み交差判定
bool Collision::IntersectWeightSphereVsWeightSphere(
    const DirectX::XMFLOAT3& positionA,
    float radiusA,
    float weightA,
    const DirectX::XMFLOAT3& positionB,
    float radiusB, float weightB,
    DirectX::XMFLOAT3& outPositionA,
    DirectX::XMFLOAT3& outPositionB)
{
    // 2つの球間のベクトルを求める
    DirectX::XMVECTOR PositionA = DirectX::XMLoadFloat3(&positionA);
    DirectX::XMVECTOR PositionB = DirectX::XMLoadFloat3(&positionB);
    DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(PositionB, PositionA);

    // 距離判定
    DirectX::XMVECTOR LengthSq = DirectX::XMVector3Length(Vec);
    float lengthSq = DirectX::XMVectorGetX(LengthSq);

    float range = radiusA + radiusB;
    if (lengthSq < range * range)
    {
        float length = sqrtf(lengthSq);
        length = (std::max)(length, 0.0001f);

        // 単位ベクトル化
        Vec = DirectX::XMVectorScale(Vec, 1.0f / length);

        // めり込み量を求める
        float diff = range - length;
        Vec = DirectX::XMVectorScale(Vec, diff);

        // 2つの球の重さから押し出し比率を求める
        float rateA = weightA / (weightA + weightB);
        float rateB = 1.0f - rateA;

        // 球Bの補正後の座標
        DirectX::XMVECTOR VelocityB = DirectX::XMVectorScale(Vec, rateA);
        PositionB = DirectX::XMVectorAdd(PositionB, VelocityB);

        // 球Aの補正後の座標
        DirectX::XMVECTOR VelocityA = DirectX::XMVectorScale(Vec, rateB);
        PositionA = DirectX::XMVectorAdd(PositionA, VelocityA);

        // 衝突結果
        DirectX::XMStoreFloat3(&outPositionA, PositionA);
        DirectX::XMStoreFloat3(&outPositionB, PositionB);

        return true;
    }
    return false;
}

bool Collision::IntersectCylinderVsCylinder(
    const DirectX::XMFLOAT3& positionA,
    float radiusA,
    float heightA,
    const DirectX::XMFLOAT3& positionB,
    float radiusB,
    float heightB,
    DirectX::XMFLOAT3& outPositionB)
{
    // Aの足元がBの頭より上なら当たっていない
    if (positionA.y > positionB.y + heightB)
    {
        return false;
    }

    // Aの頭がBの足元より下なら当たっていない
    if (positionA.y + heightA < positionB.y)
    {
        return false;
    }

    // XZ平面での範囲チェック
    float vx = positionB.x - positionA.x;
    float vz = positionB.z - positionA.z;
    float range = radiusA + radiusB;
    float distXZ = sqrtf(vx * vx + vz * vz);
    if (distXZ > range)
    {
        return false;
    }

    // AがBを押し出す

    //DirectX::XMFLOAT3 pos = { VecX, 0, VecZ };
    //DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&pos);
    //position = DirectX::XMVector3Normalize(position);
    vx /= distXZ;
    vz /= distXZ;
    //outPositionB.x = positionB.x + DirectX::XMVectorGetX(position) * (radiusA + radiusB) - VecX;
    //outPositionB.y = positionB.y;
    //outPositionB.z = positionB.z + DirectX::XMVectorGetZ(position) * (radiusA + radiusB) - VecZ;
    outPositionB.x = positionA.x + (vx * range);
    outPositionB.y = positionB.y;
    outPositionB.z = positionA.z + (vz * range);

    return true;
}

bool Collision::IntersectSphereVsCylinder(
    const DirectX::XMFLOAT3& spherePosition,
    float sphereRadius,
    const DirectX::XMFLOAT3& cylinderPosition,
    float cylinderRadius,
    float cylinderHeight,
    DirectX::XMFLOAT3& outCylinderPosition)
{
    // 球と円柱の中心の距離を計算
    DirectX::XMVECTOR SphereCenter = DirectX::XMLoadFloat3(&spherePosition);
    DirectX::XMVECTOR CylinderCenter = DirectX::XMLoadFloat3(&cylinderPosition);
    float distanceY = fabs(spherePosition.y - cylinderPosition.y);

    // 球と円柱の半径の和
    float sumRadius = sphereRadius + cylinderRadius;

    // 球と円柱の中心の距離が半径の和より大きい場合は交差していない
    if (distanceY > cylinderHeight / 2 + sphereRadius)
        return false;

    // XZ平面上での球と円柱の距離を計算
    float distanceXZ = sqrtf((spherePosition.x - cylinderPosition.x) * (spherePosition.x - cylinderPosition.x) +
        (spherePosition.z - cylinderPosition.z) * (spherePosition.z - cylinderPosition.z));

    // XZ平面上での球と円柱の半径の和
    float sumRadiusXZ = sphereRadius + cylinderRadius;

    // XZ平面上での球と円柱の距離が半径の和より大きい場合は交差していない
    if (distanceXZ > sumRadiusXZ)
        return false;

    // 球が円柱の高さ内にあり、かつXZ平面上で交差している場合は交差している
    // この場合、球が円柱を押し出す必要がある
    float penetrationDepth = sumRadiusXZ - distanceXZ;
    float penetrationRatio = penetrationDepth / distanceXZ;

    // 球の中心と円柱の中心の間のベクトル
    DirectX::XMVECTOR centerVector = DirectX::XMVectorSubtract(SphereCenter, CylinderCenter);
    DirectX::XMVECTOR normalizedCenterVector = DirectX::XMVector3Normalize(centerVector);

    // 球が押し出される距離
    DirectX::XMVECTOR pushOut = DirectX::XMVectorScale(normalizedCenterVector, penetrationDepth);
    DirectX::XMVECTOR newCylinderPosition = DirectX::XMVectorAdd(CylinderCenter, pushOut);

    // 新しい円柱の位置をoutCylinderPositionに保存
    DirectX::XMStoreFloat3(&outCylinderPosition, newCylinderPosition);

    return true;
}

bool Collision::IntersectRayVsModel(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, const Model* model, HitResult& result)
{
    DirectX::XMVECTOR WorldStart = DirectX::XMLoadFloat3(&start);
    DirectX::XMVECTOR WorldEnd = DirectX::XMLoadFloat3(&end);
    DirectX::XMVECTOR WorldRayVec = DirectX::XMVectorSubtract(WorldEnd, WorldStart);
    DirectX::XMVECTOR WorldRayLength = DirectX::XMVector3Length(WorldRayVec);

    // ワールド空間のレイの長さ
    DirectX::XMStoreFloat(&result.distance, WorldRayLength);

    bool hit = false;
    const ModelResource* resource = model->GetResource();
    for (const ModelResource::Mesh& mesh : resource->GetMeshes())
    {
        // メッシュノード取得
        const Model::Node& node = model->GetNodes().at(mesh.nodeIndex);


        // レイをワールド空間からローカル空間へ変換
        DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&node.worldTransform);
        DirectX::XMMATRIX InverseWorldTransform = DirectX::XMMatrixInverse(nullptr, WorldTransform);

        DirectX::XMVECTOR S = DirectX::XMVector3TransformCoord(WorldStart, InverseWorldTransform);
        DirectX::XMVECTOR E = DirectX::XMVector3TransformCoord(WorldEnd, InverseWorldTransform);
        DirectX::XMVECTOR SE = DirectX::XMVectorSubtract(E, S);
        DirectX::XMVECTOR V = DirectX::XMVector3Normalize(SE);
        DirectX::XMVECTOR Length = DirectX::XMVector3Length(SE);

        // レイの長さ
        float neart;
        DirectX::XMStoreFloat(&neart, Length);

        // 三角形（面）との交差判定
        const std::vector<ModelResource::Vertex>& vertices = mesh.vertices;
        const std::vector<UINT> indices = mesh.indices;

        int materialIndex = -1;
        DirectX::XMVECTOR HitPosition;
        DirectX::XMVECTOR HitNormal;
        //int count = mesh.indices.size();
        //int indexcount = count / 3;
        //feokgek


        for (UINT i = 0; i < mesh.indices.size(); i += 3)
        {

            UINT index = i;



            // 三角形の頂点を抽出
            const ModelResource::Vertex& a = vertices.at(indices.at(index + 0));
            const ModelResource::Vertex& b = vertices.at(indices.at(index + 1));
            const ModelResource::Vertex& c = vertices.at(indices.at(index + 2));

            DirectX::XMVECTOR A = DirectX::XMLoadFloat3(&a.position);
            DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&b.position);
            DirectX::XMVECTOR C = DirectX::XMLoadFloat3(&c.position);

            // 三角形の三辺ベクトルを算出
            DirectX::XMVECTOR AB = DirectX::XMVectorSubtract(B, A);
            DirectX::XMVECTOR BC = DirectX::XMVectorSubtract(C, B);
            DirectX::XMVECTOR CA = DirectX::XMVectorSubtract(A, C);

            // 三角形の法線ベクトルを算出
            DirectX::XMVECTOR N = DirectX::XMVector3Cross(AB, BC);

            // 内積の結果がプラスならば裏向き
            DirectX::XMVECTOR Dot = DirectX::XMVector3Dot(V, N);
            float d;
            DirectX::XMStoreFloat(&d, Dot);
            if (d >= 0.0f) continue;

            // レイと平面の交点を算出
            DirectX::XMVECTOR SA = DirectX::XMVectorSubtract(A, S);
            DirectX::XMVECTOR X = DirectX::XMVectorDivide(DirectX::XMVector3Dot(SA, N), Dot);
            float x;
            DirectX::XMStoreFloat(&x, X);
            if (x < .0f || x > neart) continue; // 交点までの距離が今までに計算した最近距離より   
            // 大きいときはスキップ
            DirectX::XMVECTOR P = DirectX::XMVectorAdd(S, DirectX::XMVectorScale(V, x));


            // 交点が三角形の内側にあるか判定
            // １つめ
            DirectX::XMVECTOR PA = DirectX::XMVectorSubtract(A, P);
            DirectX::XMVECTOR Cross1 = DirectX::XMVector3Cross(PA, AB);
            DirectX::XMVECTOR Dot1 = DirectX::XMVector3Dot(Cross1, N);
            float d1;
            DirectX::XMStoreFloat(&d1, Dot1);
            if (d1 < 0.0f) continue;
            // ２つめ
            DirectX::XMVECTOR PB = DirectX::XMVectorSubtract(B, P);
            DirectX::XMVECTOR Cross2 = DirectX::XMVector3Cross(PB, BC);
            DirectX::XMVECTOR Dot2 = DirectX::XMVector3Dot(Cross2, N);
            float d2;
            DirectX::XMStoreFloat(&d2, Dot2);
            if (d2 < 0.0f) continue;
            // ３つめ
            DirectX::XMVECTOR PC = DirectX::XMVectorSubtract(C, P);
            DirectX::XMVECTOR Cross3 = DirectX::XMVector3Cross(PC, CA);
            DirectX::XMVECTOR Dot3 = DirectX::XMVector3Dot(Cross3, N);
            float d3;
            DirectX::XMStoreFloat(&d3, Dot3);
            if (d3 < 0.0f) continue;

            // 最近距離を更新
            neart = x;

            // 交点と法線を更新
            HitPosition = P;
            HitNormal = N;
            materialIndex = mesh.materialIndex;

        }
        if (materialIndex >= 0)
        {
            // ローカル空間からワールド空間へ変換
            DirectX::XMVECTOR WorldPosition = DirectX::XMVector3TransformCoord(HitPosition, WorldTransform);
            DirectX::XMVECTOR WorldCrossVec = DirectX::XMVectorSubtract(WorldPosition, WorldStart);
            DirectX::XMVECTOR WorldCrossLength = DirectX::XMVector3Length(WorldCrossVec);
            float distance;
            DirectX::XMStoreFloat(&distance, WorldCrossLength);
            //seriousl

            // ヒット情報保存
            if (result.distance > distance)
            {
                DirectX::XMVECTOR WorldNormal = DirectX::XMVector3TransformNormal(HitNormal, WorldTransform);

                result.distance = distance;
                result.materialIndex = materialIndex;
                DirectX::XMStoreFloat3(&result.position, WorldPosition);
                DirectX::XMStoreFloat3(&result.normal, DirectX::XMVector3Normalize(WorldNormal));
                hit = true;
                //return true;
            }

        }


    }
    return hit;
}
