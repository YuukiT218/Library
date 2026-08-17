#include "Enemy.h"
#include "System/HitStop.h"
#include "Character/Player/Player.h"
#include "Math/Collision.h"
#include "Graphics/Graphics.h"
#include "Camera/Camera.h"
#include <string>
#include <Math/Mathf.h>
#include "Effect/EffectManager.h"
#include <algorithm>


#include <stdlib.h>



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
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
    DirectX::XMMATRIX X = DirectX::XMMatrixRotationX(angle.x);
    DirectX::XMMATRIX Y = DirectX::XMMatrixRotationY(angle.y);
    DirectX::XMMATRIX Z = DirectX::XMMatrixRotationZ(angle.z);
    DirectX::XMMATRIX R = Y * X * Z;
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(renderPosition.x, renderPosition.y, renderPosition.z);
    DirectX::XMMATRIX W = S * R * T;
    DirectX::XMStoreFloat4x4(&transform, W);
}

// テレポート開始
void Enemy::StartTeleport(const DirectX::XMFLOAT3& targetPos, float fadeOutTime)
{
    teleportPhase = TeleportPhase::FadeOut;
    teleportPhaseTimer = 0.0f;
    fadeOutDuration = fadeOutTime;
    fadeInDuration = 0.1f;

    teleportStartPosition = position;
    teleportTargetPosition = targetPos;

    // 見た目の位置は現在位置のまま
    visualPosition = position;

    // 軌跡を撒く区間の起点を初期化する
    teleportTrailPreviousPosition = position;

    // テレポート開始地点に残像を1つだけ残す
    SpawnAfterimage();
}

// 現在の姿勢から残像を1つ生成する
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

    // 現在のボーン姿勢をコピー
    newAfterimage.nodes = model->GetNodes();
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

        // 透明度を時間経過で減衰させる
        // 持続時間で割ることで、持続時間を変えても必ず1.0から0.0へ落ちる
        it->alpha = (afterimageDuration > 0.0f)
            ? std::clamp(it->lifetime / afterimageDuration, 0.0f, 1.0f)
            : 0.0f;

        // 寿命が尽きたら削除
        if (it->lifetime <= 0.0f)
        {
            // デストラクタでClearNodesが呼ばれる
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

        EffectManager::Instance().EmitGpuParticle(particlePosition, velocity, color, size, lifeTime, 2);
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

    // 画面の端すぎると見切れるため、0.8f 程度のマージンを持たせる
    const float margin = 0.7f;
    return (ndc.x >= -margin && ndc.x <= margin &&
        ndc.y >= -margin && ndc.y <= margin &&
        ndc.z >= 0.0f && ndc.z <= 1.0f);
}

DirectX::XMFLOAT3 Enemy::CalculateVisibleTeleportPos(float distance, bool bakeY)
{
    DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
    std::vector<DirectX::XMFLOAT3> availablePositions;

    // 1. 候補地点 (availablePositions) をプレイヤーの周囲 8 方向に生成
    for (int i = 0; i < 36; ++i)
    {
        float angle = DirectX::XMConvertToRadians(i * 10.0f);
        DirectX::XMFLOAT3 pos = playerPos;
        pos.x += cosf(angle) * distance;
        pos.z += sinf(angle) * distance;

        if (bakeY)
            pos.y = GetPosition().y;

        // 地形制限（壁突き抜け防止）
        KeepAreaLimit(pos);
        availablePositions.push_back(pos);
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
