#include "Enemy.h"
#include "System/HitStop.h"
#include "Character/Player.h"
#include "Math/Collision.h"
#include "Graphics/Graphics.h"
#include "Camera/Camera.h"
#include <string>
#include <Math/Mathf.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

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
    fadeInDuration = 0.1f;  // 出現も同じ時間

    teleportStartPosition = position;
    teleportTargetPosition = targetPos;

    // 見た目の位置は現在位置のまま
    visualPosition = position;

    if (hasAfterimage)
    {
        afterimage.ClearNodes();
    }

    hasAfterimage = true;
    afterimage.position = position;
    afterimage.angle = angle;
    afterimage.transform = transform;
    afterimage.alpha = 1.0f;
    afterimage.lifetime = afterimageDuration;
    afterimage.darkness = afterimageDarkness;

    // 現在のボーン姿勢をコピー
    afterimage.nodes = model->GetNodes();
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
            
            if (teleportPhaseTimer >= moveDuration)
            {
                // 移動完了、出現フェーズへ
                teleportPhase = TeleportPhase::FadeIn;
                teleportPhaseTimer = 0.0f;
                position = teleportTargetPosition;
                visualPosition = teleportTargetPosition;
            }
        }
        break;
        
    case TeleportPhase::FadeIn:
        // 出現演出中
        if (teleportPhaseTimer >= fadeInDuration)
        {
            // 出現完了、テレポート終了
            teleportPhase = TeleportPhase::None;
            teleportPhaseTimer = 0.0f;
        }
        break;
    }
}

void Enemy::UpdateAfterimage(float elapsedTime)
{
    if (!hasAfterimage) return;

    afterimage.lifetime -= elapsedTime;

    // 透明度を時間経過で減衰
    afterimage.alpha = afterimage.lifetime / afterimageDuration;

    // 寿命が尽きたら削除
    if (afterimage.lifetime <= 0.0f)
    {
        hasAfterimage = false;
        afterimage.alpha = 0.0f;
        afterimage.ClearNodes();
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
    for (int i = 0; i < 8; ++i)
    {
        float angle = DirectX::XMConvertToRadians(i * 45.0f);
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

// 破棄
void Enemy::Destroy()
{
	Destroy();
}
