#include "Enemy.h"
#include "System/HitStop.h"
#include "Character/Player/Player.h"
#include "Math/Collision.h"
#include "Graphics/Graphics.h"
#include "Camera/Camera.h"
#include <string>
#include <Math/Mathf.h>
#include "Effect/EffectManager.h"
#include "Debug/DebugToggles.h"
#include <algorithm>


#include <stdlib.h>

namespace
{
    // テレポートの出現演出にかける時間
    constexpr float TELEPORT_FADE_IN_SECONDS = 0.1f;

    // スキニングで1頂点に影響するボーンの最大数
    constexpr int MAX_BONE_INFLUENCES = 4;

    // 法線をゼロベクトルとみなす長さ
    constexpr float NORMAL_MIN_LENGTH = 0.0001f;

    // 分解粒子の初速に加えるばらつき
    constexpr float DISINTEGRATE_JITTER_XZ = 0.3f;
    constexpr float DISINTEGRATE_JITTER_Y_MIN = -0.2f;
    constexpr float DISINTEGRATE_JITTER_Y_MAX = 0.4f;

    // 分解粒子の大きさ・寿命に掛けるばらつきの範囲
    constexpr float DISINTEGRATE_VARIATION_MIN = 0.7f;
    constexpr float DISINTEGRATE_VARIATION_MAX = 1.6f;

    // 画面内判定でNDCの端から取る余白（この値の内側を画面内とする）
    constexpr float SCREEN_VISIBLE_NDC_LIMIT = 0.7f;

    // テレポート先候補を何方向に作るか
    constexpr int TELEPORT_CANDIDATE_COUNT = 36;
}

// デバッグプリミティブ描画
void Enemy::DrawDebugPrimitive()
{
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();

	// 衝突判定用デバッグ球を描画
	shapeRenderer->DrawSphere(position, radius, DirectX::XMFLOAT4(0, 0, 0, 1));
}

// デバッグImGui描画
void Enemy::DrawDebugGUI()
{
    if (ImGui::TreeNode("Teleport System"))
    {
        ImGui::Checkbox("Is Teleporting", &isTeleporting);

        ImGui::Separator();
        ImGui::Text("Visual Position: (%.2f, %.2f, %.2f)",
            visualPosition.x, visualPosition.y, visualPosition.z);
        ImGui::Text("Logical Position: (%.2f, %.2f, %.2f)",
            logicalPosition.x, logicalPosition.y, logicalPosition.z);
        ImGui::Text("Actual Position: (%.2f, %.2f, %.2f)",
            position.x, position.y, position.z);

        float distance = 0.0f;
        if (isTeleporting)
        {
            DirectX::XMVECTOR vVisual = DirectX::XMLoadFloat3(&visualPosition);
            DirectX::XMVECTOR vLogical = DirectX::XMLoadFloat3(&logicalPosition);
            distance = DirectX::XMVectorGetX(
                DirectX::XMVector3Length(DirectX::XMVectorSubtract(vVisual, vLogical))
            );
        }
        ImGui::Text("Position Difference: %.2f", distance);

        ImGui::TreePop();
    }
}

void Enemy::EditUpdate(float elapsedTime)
{
	// 共通の更新処理
	UpdateVelocity(elapsedTime);
	UpdateInvincibleTimer(elapsedTime);
	UpdateTransform();

	if (model)
	{
		model->UpdateAnimation(elapsedTime, this);
		model->UpdateTransform(transform);
	}
}

void Enemy::Update(float elapsedTime)
{
	totalGameTime += elapsedTime;

    // テレポート更新
    UpdateTeleport(elapsedTime);

    // 残像更新
    UpdateAfterimage(elapsedTime);

	// 派生クラス専用処理（デフォルトは何もしない）
	UpdateEnemySpecific(elapsedTime * HitStop::Instance().GetEnemyTimeScale());

	// 共通の更新処理
	UpdateVelocity(elapsedTime * HitStop::Instance().GetEnemyTimeScale());
	UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

	if (model)
	{
		model->UpdateAnimation(elapsedTime, this);
		model->UpdateTransform(transform);
	}
}

void Enemy::UpdateTransform()
{
    // テレポート中は見た目の位置を使用
    DirectX::XMFLOAT3 renderPosition = isTeleporting ? visualPosition : position;

    // モデルのトランスフォームを更新（見た目の位置で）
    Character::UpdateTransform(scale, angle, renderPosition, &transform);
}

// テレポート開始
void Enemy::StartTeleport(const DirectX::XMFLOAT3& targetPos, float fadeOutTime)
{
    // 比較用の「演出なし」モード
    // 消失・移動・出現のフェーズを踏まず、その場で目的地へ飛ぶ。
    // 呼び出し側はどれも IsTeleporting() が false に戻るのを待って次へ進むので、
    // ここで即座に終わらせても行動そのものは止まらない。
    if (DebugToggles::Instance().IsTeleportInstant())
    {
        teleportPhase = TeleportPhase::None;
        teleportPhaseTimer = 0.0f;

        teleportStartPosition = targetPos;
        teleportTargetPosition = targetPos;
        position = targetPos;
        logicalPosition = targetPos;
        visualPosition = targetPos;
        teleportTrailPreviousPosition = targetPos;
        return;
    }

    teleportPhase = TeleportPhase::FadeOut;
    teleportPhaseTimer = 0.0f;
    fadeOutDuration = fadeOutTime;
    fadeInDuration = TELEPORT_FADE_IN_SECONDS;

    teleportStartPosition = position;
    teleportTargetPosition = targetPos;

    // 見た目の位置は現在位置のまま
    visualPosition = position;

    // 軌跡を撒く区間の起点を初期化する
    teleportTrailPreviousPosition = position;

    // テレポート開始地点に残像を1つだけ残す
    if (DebugToggles::Instance().IsTeleportParticleEnabled())
    {
        SpawnAfterimage();
    }
}

// 現在の姿勢から残像を1つ生成する
// 残像といっても薄い分身を残すのではなく、
// この瞬間の姿のまま置き去りにした体を、その場で粒子に分解して消すための情報を作る
void Enemy::SpawnAfterimage()
{
    if (!model) return;

    Afterimage& newAfterimage = afterimages.emplace_back();
    newAfterimage.position = position;
    newAfterimage.angle = angle;
    newAfterimage.transform = transform;
    newAfterimage.alpha = 1.0f;
    newAfterimage.lifetime = afterimageDuration;
    newAfterimage.darkness = afterimageDarkness;
    newAfterimage.dissolve = 0.0f;

    // 現在のボーン姿勢をコピー
    // ここで固めてしまうことで、崩れている間はアニメーションが進まなくなる
    newAfterimage.parts.push_back({ model, model->GetNodes() });

    // 武器も一緒に崩さないと、本体だけ分解して武器が瞬間的に消えたように見える
    for (const std::shared_ptr<Model>& attachment : GetAfterimageAttachments())
    {
        if (attachment == nullptr) continue;
        newAfterimage.parts.push_back({ attachment, attachment->GetNodes() });
    }

    // 粒子の発生点を作る
    CollectDisintegrationPoints(newAfterimage);
}

// モデル表面から粒子の発生点を拾う
// 頂点をCPU側でスキニングして、崩れ始めた瞬間の見た目どおりの位置に粒子を置けるようにする
void Enemy::CollectDisintegrationPoints(Afterimage& afterimage)
{
    afterimage.points.clear();
    afterimage.emitCursor = 0;

    // 何頂点に1つ拾うかを決める
    // モデルの頂点数が変わっても、粒の数がだいたい揃うようにする
    size_t totalVertexCount = 0;
    for (const AfterimagePart& part : afterimage.parts)
    {
        const ModelResource* resource = part.model ? part.model->GetResource() : nullptr;
        if (resource == nullptr) continue;

        for (const ModelResource::Mesh& mesh : resource->GetMeshes())
        {
            totalVertexCount += mesh.vertices.size();
        }
    }
    if (totalVertexCount == 0) return;

    const size_t targetCount = static_cast<size_t>((std::max)(disintegrateSampleCount, 1));
    const size_t stride = (std::max)(totalVertexCount / targetCount, static_cast<size_t>(1));

    afterimage.points.reserve(totalVertexCount / stride + 1);

    for (const AfterimagePart& part : afterimage.parts)
    {
        const ModelResource* resource = part.model ? part.model->GetResource() : nullptr;
        if (resource == nullptr) continue;

        for (const ModelResource::Mesh& mesh : resource->GetMeshes())
        {
            if (mesh.nodeIndex >= static_cast<int>(part.nodes.size())) continue;

            // ボーン行列を先に作っておく（描画時のスキニングと同じ組み立て方）
            std::vector<DirectX::XMMATRIX> boneTransforms;
            boneTransforms.reserve(mesh.bones.size());
            for (const ModelResource::Bone& bone : mesh.bones)
            {
                if (bone.nodeIndex >= static_cast<int>(part.nodes.size())) break;

                DirectX::XMMATRIX offsetTransform = DirectX::XMLoadFloat4x4(&bone.offsetTransform);
                DirectX::XMMATRIX worldTransform = DirectX::XMLoadFloat4x4(&part.nodes[bone.nodeIndex].worldTransform);
                boneTransforms.emplace_back(offsetTransform * worldTransform);
            }

            // ボーンを持たないメッシュはノードのワールド行列だけで位置が決まる
            const DirectX::XMMATRIX rigidTransform = DirectX::XMLoadFloat4x4(&part.nodes[mesh.nodeIndex].worldTransform);

            for (size_t i = 0; i < mesh.vertices.size(); i += stride)
            {
                const ModelResource::Vertex& vertex = mesh.vertices[i];

                const DirectX::XMVECTOR localPosition = DirectX::XMLoadFloat3(&vertex.position);
                const DirectX::XMVECTOR localNormal = DirectX::XMLoadFloat3(&vertex.normal);

                DirectX::XMVECTOR worldPosition = DirectX::XMVectorZero();
                DirectX::XMVECTOR worldNormal = DirectX::XMVectorZero();
                float weightSum = 0.0f;

                if (!boneTransforms.empty())
                {
                    const float weights[MAX_BONE_INFLUENCES] =
                    {
                        vertex.boneWeight.x, vertex.boneWeight.y,
                        vertex.boneWeight.z, vertex.boneWeight.w
                    };
                    const uint32_t boneIndices[MAX_BONE_INFLUENCES] =
                    {
                        vertex.boneIndex.x, vertex.boneIndex.y,
                        vertex.boneIndex.z, vertex.boneIndex.w
                    };

                    for (int b = 0; b < MAX_BONE_INFLUENCES; ++b)
                    {
                        if (weights[b] <= 0.0f) continue;
                        if (boneIndices[b] >= boneTransforms.size()) continue;

                        const DirectX::XMMATRIX& boneTransform = boneTransforms[boneIndices[b]];
                        worldPosition = DirectX::XMVectorAdd(worldPosition,
                            DirectX::XMVectorScale(DirectX::XMVector3Transform(localPosition, boneTransform), weights[b]));
                        worldNormal = DirectX::XMVectorAdd(worldNormal,
                            DirectX::XMVectorScale(DirectX::XMVector3TransformNormal(localNormal, boneTransform), weights[b]));
                        weightSum += weights[b];
                    }
                }

                // ウェイトが無い頂点はノードの行列で動かす（原点に潰れるのを防ぐ）
                if (weightSum <= 0.0f)
                {
                    worldPosition = DirectX::XMVector3Transform(localPosition, rigidTransform);
                    worldNormal = DirectX::XMVector3TransformNormal(localNormal, rigidTransform);
                }

                DisintegrationPoint& point = afterimage.points.emplace_back();
                DirectX::XMStoreFloat3(&point.position, worldPosition);

                // 法線が潰れている頂点は真上に飛ばす（正規化でNaNになるのを避ける）
                const float normalLength = DirectX::XMVectorGetX(DirectX::XMVector3Length(worldNormal));
                if (normalLength > NORMAL_MIN_LENGTH)
                {
                    DirectX::XMStoreFloat3(&point.normal, DirectX::XMVector3Normalize(worldNormal));
                }
                else
                {
                    point.normal = { 0.0f, 1.0f, 0.0f };
                }

                // シェーダー側のディゾルブはノイズで削っていくので、
                // こちらも点ごとにばらけたしきい値を持たせて、削れる歩調を合わせる
                point.threshold = Mathf::RandomRange(0.0f, 1.0f);
            }
        }
    }

    // 進行度と比べながら先頭から消費できるように、しきい値の昇順に並べておく
    std::sort(afterimage.points.begin(), afterimage.points.end(),
        [](const DisintegrationPoint& lhs, const DisintegrationPoint& rhs)
        {
            return lhs.threshold < rhs.threshold;
        });
}

// 削れた分だけ点を粒子に変える
void Enemy::EmitDisintegrationParticles(Afterimage& afterimage)
{
    if (afterimage.points.empty()) return;

    // 軌跡の粒子と同じ色にして、一連の演出としてつながって見えるようにする
    const DirectX::XMFLOAT4 color =
    {
        afterimageColor.x * afterimageColor.w,
        afterimageColor.y * afterimageColor.w,
        afterimageColor.z * afterimageColor.w,
        1.0f
    };

    while (afterimage.emitCursor < afterimage.points.size() &&
        afterimage.points[afterimage.emitCursor].threshold <= afterimage.dissolve)
    {
        const DisintegrationPoint& point = afterimage.points[afterimage.emitCursor];
        ++afterimage.emitCursor;

        // 表面から剥がれるように法線方向へ弾いてから、ゆっくり昇らせる
        const DirectX::XMFLOAT3 velocity =
        {
            point.normal.x * disintegrateBurstSpeed + Mathf::RandomRange(-DISINTEGRATE_JITTER_XZ, DISINTEGRATE_JITTER_XZ),
            point.normal.y * disintegrateBurstSpeed + disintegrateRiseSpeed + Mathf::RandomRange(DISINTEGRATE_JITTER_Y_MIN, DISINTEGRATE_JITTER_Y_MAX),
            point.normal.z * disintegrateBurstSpeed + Mathf::RandomRange(-DISINTEGRATE_JITTER_XZ, DISINTEGRATE_JITTER_XZ)
        };

        const float size = disintegrateParticleSize * Mathf::RandomRange(DISINTEGRATE_VARIATION_MIN, DISINTEGRATE_VARIATION_MAX);
        const float lifeTime = disintegrateParticleLife * Mathf::RandomRange(DISINTEGRATE_VARIATION_MIN, DISINTEGRATE_VARIATION_MAX);

        // Float は空気抵抗で急減速しながらふわりと昇るので、
        // 崩れた破片が空気に溶けていくように見える
        EffectManager::Instance().EmitGpuParticle(point.position, velocity, color, size, lifeTime, GpuParticleBehavior::Float);
    }
}

// テレポート更新
void Enemy::UpdateTeleport(float elapsedTime)
{
    if (teleportPhase == TeleportPhase::None) return;
    
    teleportPhaseTimer += elapsedTime;
    
    switch (teleportPhase)
    {
    case TeleportPhase::FadeOut:
        // 消失演出中
        if (teleportPhaseTimer >= fadeOutDuration)
        {
            // 消失完了、移動フェーズへ
            teleportPhase = TeleportPhase::Moving;
            OnTeleportPhaseChanged(teleportPhase);
            teleportPhaseTimer = 0.0f;
        }
        break;
        
    case TeleportPhase::Moving:
        // 位置移動中（見えない状態）
        {
            float t = teleportPhaseTimer / moveDuration;
            t = std::clamp(t, 0.0f, 1.0f);
            
            // イージング（easeInOutCubic）
            float easedT = t < 0.5f 
                ? 4.0f * t * t * t 
                : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
            
            // 論理位置を移動
            logicalPosition.x = Mathf::Lerp(teleportStartPosition.x, teleportTargetPosition.x, easedT);
            logicalPosition.y = Mathf::Lerp(teleportStartPosition.y, teleportTargetPosition.y, easedT);
            logicalPosition.z = Mathf::Lerp(teleportStartPosition.z, teleportTargetPosition.z, easedT);
            
            // 実際の位置も更新
            position = logicalPosition;

            // 通った軌跡をGPUパーティクルで可視化する
            // 残像は開始地点の1つだけなので、経路の表現はこちらが担当する
            // 前回位置から現在位置までを結ぶように撒くと、間が空かず直線になる
            EmitTeleportTrail(teleportTrailPreviousPosition, position);
            teleportTrailPreviousPosition = position;
            
            if (teleportPhaseTimer >= moveDuration)
            {
                // 移動完了、出現フェーズへ
                position = teleportTargetPosition;
                visualPosition = teleportTargetPosition;
                teleportPhase = TeleportPhase::FadeIn;
                OnTeleportPhaseChanged(teleportPhase);
                teleportPhaseTimer = 0.0f;
            }
        }
        break;
        
    case TeleportPhase::FadeIn:
        // 出現演出中
        if (teleportPhaseTimer >= fadeInDuration)
        {
            // 出現完了、テレポート終了
            teleportPhase = TeleportPhase::None;
            OnTeleportPhaseChanged(teleportPhase);
            teleportPhaseTimer = 0.0f;
        }
        break;
    }
}

void Enemy::UpdateAfterimage(float elapsedTime)
{
    // リスト内のすべての残像を更新・寿命が尽きたものを削除
    if (afterimages.empty()) return;

    for (auto it = afterimages.begin(); it != afterimages.end(); )
    {
        it->lifetime -= elapsedTime;

        // 残り時間を1.0→0.0に正規化する
        // 持続時間で割ることで、持続時間を変えても必ず1.0から0.0へ落ちる
        const float remain = (afterimageDuration > 0.0f)
            ? std::clamp(it->lifetime / afterimageDuration, 0.0f, 1.0f)
            : 0.0f;

        // アルファは落とさない（1.0未満にするとシェーダー側で残像色に塗り潰され、
        // モデルの見た目のまま崩したいのに、のっぺりした分身になってしまう）
        // 姿が消えるのはディゾルブで削り切ったときだけにする

        // 削れ具合を進める
        // カーブを1未満にして、序盤に一気に崩れてから残りがゆっくり散るようにする
        it->dissolve = powf(1.0f - remain, disintegrateCurve);

        // 削れた分を粒子にする
        EmitDisintegrationParticles(*it);

        // 寿命が尽きたら削除
        if (it->lifetime <= 0.0f)
        {
            it = afterimages.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// テレポートの軌跡をGPUパーティクルで撒く
// 1フレームで進んだ区間(from→to)の上に等間隔で並べることで、
// 同じ位置に固まらず1本の直線としてつながって見えるようにする
void Enemy::EmitTeleportTrail(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to)
{
    // 比較用に演出を切っているときは軌跡を撒かない（移動そのものは同じ時間で行う）
    if (!DebugToggles::Instance().IsTeleportParticleEnabled()) return;

    // 線として見せたいので、散らす量は最小限にする
    constexpr float TRAIL_CENTER_HEIGHT = 1.0f;  // 体の中心あたりの高さ
    constexpr float TRAIL_SPREAD = 0.07f;        // 線の太さ
    constexpr float TRAIL_DRIFT_SPEED = 0.25f;   // わずかに漂わせる速度
    constexpr float TRAIL_MIN_SIZE = 0.05f;
    constexpr float TRAIL_SIZE_RANGE = 0.06f;
    constexpr float TRAIL_MIN_LIFETIME = 0.30f;
    constexpr float TRAIL_LIFETIME_RANGE = 0.30f;

    // 残像と同じ色にして、軌跡と残像が同じ演出に見えるようにする
    const DirectX::XMFLOAT4 color =
    {
        afterimageColor.x * afterimageColor.w,
        afterimageColor.y * afterimageColor.w,
        afterimageColor.z * afterimageColor.w,
        1.0f
    };

    const int emitCount = (std::max)(teleportTrailEmitCount, 1);

    for (int i = 0; i < emitCount; ++i)
    {
        // 区間を等分した位置を求める
        float t = (emitCount > 1) ? static_cast<float>(i) / (emitCount - 1) : 0.0f;

        DirectX::XMFLOAT3 particlePosition =
        {
            Mathf::Lerp(from.x, to.x, t) + Mathf::RandomRange(-1.0f, 1.0f) * TRAIL_SPREAD,
            Mathf::Lerp(from.y, to.y, t) + TRAIL_CENTER_HEIGHT + Mathf::RandomRange(-1.0f, 1.0f) * TRAIL_SPREAD,
            Mathf::Lerp(from.z, to.z, t) + Mathf::RandomRange(-1.0f, 1.0f) * TRAIL_SPREAD
        };

        // 線の形を保ちたいので、ほとんど動かさずその場で消えるようにする
        DirectX::XMFLOAT3 velocity =
        {
            Mathf::RandomRange(-1.0f, 1.0f) * TRAIL_DRIFT_SPEED,
            Mathf::RandomRange(-0.2f, 1.0f) * TRAIL_DRIFT_SPEED,
            Mathf::RandomRange(-1.0f, 1.0f) * TRAIL_DRIFT_SPEED
        };

        float size = TRAIL_MIN_SIZE + Mathf::RandomRange(0.0f, 1.0f) * TRAIL_SIZE_RANGE;
        float lifeTime = TRAIL_MIN_LIFETIME + Mathf::RandomRange(0.0f, 1.0f) * TRAIL_LIFETIME_RANGE;

        EffectManager::Instance().EmitGpuParticle(particlePosition, velocity, color, size, lifeTime, GpuParticleBehavior::Drift);
    }
}

bool Enemy::IsPositionVisible(const DirectX::XMFLOAT3& worldPos)
{
    DirectX::XMMATRIX view = XMLoadFloat4x4(&Camera::Instance().GetView());
    DirectX::XMMATRIX projection = XMLoadFloat4x4(&Camera::Instance().GetProjection());

    // ワールド座標をNDC（正規化デバイス座標: -1.0～1.0）に変換
    DirectX::XMVECTOR posVec = DirectX::XMLoadFloat3(&worldPos);
    DirectX::XMVECTOR screenPos = DirectX::XMVector3TransformCoord(posVec, view * projection);

    DirectX::XMFLOAT3 ndc;
    DirectX::XMStoreFloat3(&ndc, screenPos);

    // 画面の端すぎると見切れるため、マージンを持たせる
    return (ndc.x >= -SCREEN_VISIBLE_NDC_LIMIT && ndc.x <= SCREEN_VISIBLE_NDC_LIMIT &&
        ndc.y >= -SCREEN_VISIBLE_NDC_LIMIT && ndc.y <= SCREEN_VISIBLE_NDC_LIMIT &&
        ndc.z >= 0.0f && ndc.z <= 1.0f);
}

DirectX::XMFLOAT3 Enemy::CalculateVisibleTeleportPos(float distance, bool bakeY)
{
    DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
    std::vector<DirectX::XMFLOAT3> availablePositions;

    // 1. 候補地点 (availablePositions) をプレイヤーの周囲に等間隔で生成
    for (int i = 0; i < TELEPORT_CANDIDATE_COUNT; ++i)
    {
        float angle = DirectX::XM_2PI * static_cast<float>(i) / TELEPORT_CANDIDATE_COUNT;
        DirectX::XMFLOAT3 pos = playerPos;
        pos.x += cosf(angle) * distance;
        pos.z += sinf(angle) * distance;

        if (bakeY)
            pos.y = GetPosition().y;

        // 地形制限（壁突き抜け防止）
        KeepAreaLimit(pos);
        availablePositions.push_back(pos);
    }

    // 比較用に画面内への限定を切っているときは、候補からそのままランダムに選ぶ
    // （画面外へ飛んでボスを見失う状態を再現する）
    if (!DebugToggles::Instance().IsTeleportDestinationOnScreenOnly())
    {
        return availablePositions[rand() % availablePositions.size()];
    }

    // 2. 画面内に入っている地点をフィルタリング
    std::vector<DirectX::XMFLOAT3> visiblePositions;
    for (const auto& pos : availablePositions)
    {
        if (IsPositionVisible(pos))
        {
            visiblePositions.push_back(pos);
        }
    }

    // 3. 最終選出
    if (!visiblePositions.empty())
    {
        // 画面内の候補があればランダムに選ぶ
        return visiblePositions[rand() % visiblePositions.size()];
    }

    // 画面内に候補がない場合は、強制的に 0 番目（プレイヤーの正面方向など）を返す
    return availablePositions[0];
}

float Enemy::GetTeleportProgress() const
{
    switch (teleportPhase)
    {
    case TeleportPhase::FadeOut:
        // 0.0 → 1.0 に進行
        return std::clamp(teleportPhaseTimer / fadeOutDuration, 0.0f, 1.0f);

    case TeleportPhase::Moving:
        // 完全に消えた状態（1.0のまま）
        return 1.0f;

    case TeleportPhase::FadeIn:
        // 1.0 → 0.0 に戻る
        return 1.0f - std::clamp(teleportPhaseTimer / fadeInDuration, 0.0f, 1.0f);

    default:
        return 0.0f;
    }
}
