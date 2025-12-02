#include "Enemy.h"
#include "System/HitStop.h"
//#include "EnemyManager.h"
#include "Character/Player.h"
#include "Math/Collision.h"
#include "Camera/Camera.h"
#include <string>

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
        ImGui::DragFloat("Teleport Timer", &teleportTimer, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Teleport Duration", &teleportDuration, 0.1f, 0.1f, 2.0f);

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
    // テレポート更新
    UpdateTeleport(elapsedTime);

	// 派生クラス専用処理（デフォルトは何もしない）
	if (actionFlag)
		UpdateEnemySpecific(elapsedTime/* * HitStop::Instance().GetEnemyTimeScale()*/);

	// 共通の更新処理
	UpdateVelocity(elapsedTime/* * HitStop::Instance().GetEnemyTimeScale()*/);
	UpdateInvincibleTimer(elapsedTime);
    UpdateTransform();

	if (model)
	{
		model->UpdateAnimation(elapsedTime, this);
		model->UpdateTransform(transform);
		//model->UpdateShakeModel(transform, 0.25f);
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
void Enemy::StartTeleport(const DirectX::XMFLOAT3& targetPos, float duration)
{
    isTeleporting = true;
    teleportTimer = 0.0f;
    teleportDuration = duration;

    // 開始位置と目標位置を保存
    teleportStartPosition = position;
    teleportTargetPosition = targetPos;

    // 論理位置は現在位置から開始
    logicalPosition = position;

    // 見た目の位置は即座に目標位置へ（瞬間移動に見える）
    visualPosition = targetPos;

    // モデルの実際のpositionも目標位置へ
    position = targetPos;
}

// テレポート更新
void Enemy::UpdateTeleport(float elapsedTime)
{
    if (!isTeleporting) return;

    teleportTimer += elapsedTime;

    // 進行度を計算
    float t = teleportTimer / teleportDuration;
    t = std::clamp(t, 0.0f, 1.0f);
    float easedT = 1.0f - powf(1.0f - t, 3.0f);  // easeOutCubic

    // 論理位置を線形補完で移動（カメラが追従する位置）
    DirectX::XMVECTOR vStart = XMLoadFloat3(&teleportStartPosition);
    DirectX::XMVECTOR vTarget = XMLoadFloat3(&teleportTargetPosition);
    DirectX::XMVECTOR vLogical = DirectX::XMVectorLerp(vStart, vTarget, easedT);
    XMStoreFloat3(&logicalPosition, vLogical);

    // テレポート完了判定
    if (teleportTimer >= teleportDuration)
    {
        isTeleporting = false;
        teleportTimer = 0.0f;

        // 位置を同期
        logicalPosition = position;
        visualPosition = position;
    }
}

// 破棄
void Enemy::Destroy()
{
	Destroy();
}
