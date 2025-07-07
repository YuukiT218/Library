#include "Enemy.h"
//#include "System/HitStop.h"
//#include "EnemyManager.h"
#include "Character/Player.h"
#include "Math/Collision.h"
#include "Camera/Camera.h"
#include <string>

// ノードとプレイヤーの衝突処理
void Enemy::CollisionNodeVsPlayer(std::vector<NodeHitSphere> attackSpheres, int AttackDamage, float invicibleTime)
{
	//if (HitStop::Instance().GetPlayerHitStop() || HitStop::Instance().GetEnemyHitStop()) return;

	// ノードの位置と当たり判定を行う
	for (auto& enemyAttackSpheres : attackSpheres)
	{
		this->attackHitSpheres = attackSpheres;

		Model::Node* node = model->FindNode(enemyAttackSpheres.nodeName);
		if (node != nullptr)
		{
			// ノードのワールド座標
			DirectX::XMFLOAT3 nodePosition(
				node->worldTransform._41,
				node->worldTransform._42,
				node->worldTransform._43
			);

			//// 当たり判定表示
			//Graphics::Instance().GetDebugRenderer()->DrawSphere(
			//	nodePosition, enemyAttackSpheres.radius, DirectX::XMFLOAT4(1, 0, 0, 1)
			//);

			Player& player = Player::Instance();

			//if (player.GetLeftShield()->GetIsParry()) return;

			std::vector<NodeHitSphere> playerNode = Player::Instance().GetNodeHitSpheres();
			for (auto& playerHitSphere : playerNode)
			{
				std::shared_ptr<Model> playerModel = Player::Instance().GetPlayerModel();
				Model::Node* playerNode = playerModel->FindNode(playerHitSphere.nodeName);

				// ノード位置取得
				DirectX::XMFLOAT3 playerNodePosition;
				playerNodePosition = { playerNode->worldTransform._41, playerNode->worldTransform._42, playerNode->worldTransform._43 };

				DirectX::XMFLOAT3 outPosition;
				DirectX::XMFLOAT3 HitPosition;

				if (Collision::IntersectSphereVsSphere(
					nodePosition,
					enemyAttackSpheres.radius,
					playerNodePosition,
					playerHitSphere.radius,
					outPosition,
					HitPosition
				))
				{
					// プレイヤーが回避中なら処理を抜ける
					bool isPlayerRolling = Player::Instance().GetPlayerIsRolling();
					if (isPlayerRolling)
					{
						//HitStop::Instance().HitStopStart(1.0f, 0.1f, 3.0f, 0.1f);
						//Camera::Instance().SetAcceleration(3.0f);
						isPlayerInvincible = true;
					}
					else if (player.GetPlayerIsGuard())
					{
						player.ChangeState(PlayerStateId::GuardHit);
					}
					// ダメージを与える
					else if (player.ApplyDamage(AttackDamage, invicibleTime, true, HitPosition))
					{
						//// 敵を吹っ飛ばすベクトルを算出
						//DirectX::XMFLOAT3 vec;
						//vec.x = outPosition.x - nodePosition.x;
						//vec.z = outPosition.z - nodePosition.z;
						//float length = sqrtf(vec.x * vec.x + vec.z * vec.z);
						//vec.x /= length;
						//vec.z /= length;

						//// XZ平面に吹っ飛ばす力をかける
						//float power = 15.0f;
						//vec.x *= power;
						//vec.z *= power;
						//// Y方向にも力をかける
						//vec.y = 5.0f;

						//// 吹っ飛ばす
						//player.AddImpulse(vec);

						//Camera::Instance().SetCameraShakeSwitch(true, 0.5f, 0.1f);
					}
				}
			}
		}
	}
}

// アニメーションの攻撃当たり判定を付ける
void Enemy::AttackAnimationCollision(std::vector<NodeHitSphere> attackSpheres, float animTimeMin, float animTimeMax, int AttackDamage, float invicibleTime)
{
	// 任意のアニメーション再生区間でのみ衝突処理をする
	float animationTime = model->GetCurrentAnimationSeconds();
	if (animationTime >= animTimeMin && animationTime <= animTimeMax)
	{
		// 攻撃当たり判定球とプレイヤーの衝突判定
		CollisionNodeVsPlayer(attackSpheres, AttackDamage, invicibleTime);
		attackFlg = true;
	}
	else
	{
		attackFlg = false;
	}
}

// ブレスエフェクトの当たり判定を付ける
void Enemy::BreathEffectCollision(float animTimeMin, float animTimeMax, DirectX::XMFLOAT3 startPosition, DirectX::XMFLOAT3 direction, float length, float sphereRadius, int sphereCount, int AttackDamage, float invicibleTime)
{
	// 現在のアニメーション時間を取得
	float animationTime = model->GetCurrentAnimationSeconds();

	// アニメーション時間が指定範囲内でない場合は処理をスキップ
	if (animationTime < animTimeMin || animationTime > animTimeMax)
	{
		return;
	}

	// ブレスの方向を正規化
	DirectX::XMVECTOR dirVector = DirectX::XMLoadFloat3(&direction);
	dirVector = DirectX::XMVector3Normalize(dirVector);

	// 各球の間隔を計算
	float interval = length / static_cast<float>(sphereCount);

	// 各球の位置を計算し、衝突判定を行う
	for (int i = 0; i < sphereCount; ++i)
	{
		// 球の中心位置を計算
		DirectX::XMVECTOR spherePosition = DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&startPosition), DirectX::XMVectorScale(dirVector, interval * i));
		DirectX::XMFLOAT3 spherePositionFloat3;
		DirectX::XMStoreFloat3(&spherePositionFloat3, spherePosition);

		// デバッグ用に球を描画
		//Graphics::Instance().GetDebugRenderer()->DrawSphere(spherePositionFloat3, sphereRadius, DirectX::XMFLOAT4(1, 0, 0, 1));

		// プレイヤーとの衝突判定
		Player& player = Player::Instance();
		std::vector<NodeHitSphere> playerNode = player.GetNodeHitSpheres();
		for (auto& playerHitSphere : playerNode)
		{
			std::shared_ptr<Model> playerModel = player.GetPlayerModel();
			Model::Node* playerNode = playerModel->FindNode(playerHitSphere.nodeName);

			// プレイヤーのノード位置を取得
			DirectX::XMFLOAT3 playerNodePosition = {
				playerNode->worldTransform._41,
				playerNode->worldTransform._42,
				playerNode->worldTransform._43
			};

			DirectX::XMFLOAT3 outPosition;
			DirectX::XMFLOAT3 hitPosition;

			// 衝突判定
			if (Collision::IntersectSphereVsSphere(
				spherePositionFloat3,
				sphereRadius,
				playerNodePosition,
				playerHitSphere.radius,
				outPosition,
				hitPosition
			))
			{
				// プレイヤーが回避中なら処理を抜ける
				bool isPlayerRolling = Player::Instance().GetPlayerIsRolling();
				if (isPlayerRolling)
				{
					//HitStop::Instance().HitStopStart(1.0f, 0.1f, 3.0f, 0.1f);
					//Camera::Instance().SetAcceleration(3.0f);
					isPlayerInvincible = true;
				}
				else if (player.GetPlayerIsGuard())
				{
					player.ChangeState(PlayerStateId::GuardHit);
				}
				// プレイヤーにダメージを与える
				else if (player.ApplyDamage(AttackDamage, invicibleTime, true, hitPosition))
				{
					//Camera::Instance().SetCameraShakeSwitch(true);
				}
			}
		}
	}
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
	/*std::string label = "action##" + std::to_string(reinterpret_cast<uintptr_t>(this));
	ImGui::Checkbox(label.c_str(), &actionFlag);

	DrawDebugChildGUI();*/
}

void Enemy::EditUpdate(float elapsedTime)
{
	// 共通の更新処理
	UpdateVelocity(elapsedTime);
	UpdateInvincibleTimer(elapsedTime);
	UpdateTransform();

	if (model)
	{
		model->UpdateAnimation(elapsedTime);
		model->UpdateTransform(transform);
		//model->UpdateShakeModel(transform, 0.25f);
	}
}

void Enemy::Update(float elapsedTime)
{
	// 派生クラス専用処理（デフォルトは何もしない）
	if (actionFlag)
		UpdateEnemySpecific(elapsedTime/* * HitStop::Instance().GetEnemyTimeScale()*/);

	// 共通の更新処理
	UpdateVelocity(elapsedTime/* * HitStop::Instance().GetEnemyTimeScale()*/);
	UpdateInvincibleTimer(elapsedTime);
	UpdateTransform();

	if (model)
	{
		model->UpdateAnimation(elapsedTime/* * HitStop::Instance().GetEnemyTimeScale()*/);
		model->UpdateTransform(transform);
		//model->UpdateShakeModel(transform, 0.25f);
	}
}

// 破棄
void Enemy::Destroy()
{
	//EnemyManager::Instance().Remove(this);
	Destroy();
}
