#pragma once
#include "Graphics/Graphics.h"
#include "Model/Model.h"
#include "Character/Enemy/Enemy.h"
#include "Graphics/Light.h"
#include "Character/Weapon/EnemySword.h"

template <typename ActorType>
class BehaviorTree;
template <typename ActorType>
class BehaviorData;
template <typename ActorType>
class NodeBase;
template <typename ActorType>
class StateBase;
template <typename ActorType>
class RootState;
template <typename ActorType>
class RandomState;
template <typename ActorType>
class SequenceState;

class EnemyBoss : public Enemy
{
public:
	static EnemyBoss& Instance();

	EnemyBoss(ID3D11Device* device, const char* filename, float scale);
	~EnemyBoss()override;

	// 更新処理
	void UpdateEnemySpecific(float elapsedTime)override;

	// エディター用更新処理
	void EditUpdate(float elapsedTime) override;

	// 描画処理
	void Render(const RenderContext& rc, ShaderId shaderId)override;

	//影描画処理
	void ShadowRender(const RenderContext& rc, ShadowMap* shadowMap);

	// デバッグプリミティブ描画
	void DrawDebugPrimitive() override;

	// 縄張り設定
	void SetTerritory(const DirectX::XMFLOAT3& origin, float range);

	// ターゲット位置をランダム設定
	void SetRandomTargetPosition();

	// 移動設定
	void SetMovement(DirectX::XMFLOAT3& Vec, float speedRate);

	// 目標地点へ移動
	void MoveToTarget(float elapsedTime, float speedRate);

	// 旋回
	void TurnToTarget(float elapsedTime, float speed);

	// プレイヤー索敵
	bool SearchPlayer();

	// プレイヤーの方向を向ききったか判定
	bool IsTurnToTarget(float vx, float vz);

	// ゲームクリアシーン遷移可能フラグ
	bool IsGameClear = false;

	// ターゲットポジション設定
	void SetTargetPosition(DirectX::XMFLOAT3 position) { targetPosition = position; }

	// ターゲットポジション取得
	DirectX::XMFLOAT3 GetTargetPosition() { return targetPosition; }

	// ポジション取得
	DirectX::XMFLOAT3 GetPosition() { return position; }

	// 角度取得
	DirectX::XMFLOAT3 GetAngle() { return angle; }

	// 攻撃範囲取得
	float GetAttackRange() { return attackRange; }

	// ステート取得
	RootState<EnemyBoss>* GetState() const { return rootState.get(); }

	// デバッグエネミー情報表示
	void DrawDebugGUI();

	// 実行タイマー設定
	void SetRunTimer(float timer) { runTimer = timer; }

	// 実行タイマー取得
	float GetRunTimer() { return runTimer; }

	void AddRevengeValue(int num) { revengeValue += num; }

	float GetRevengeValue() { return revengeValue; }

	float GetBlendSeconds() { return blendSeconds; }

	float GetMoveSpeed() { return moveSpeed; }

	void SetVerticalVelocity(float velocity) { this->velocity.y = velocity; }

	void SetWarpPosition();

	void SetKnockBackPosition();

	DirectX::SimpleMath::Vector3 WarpPosition[3];
	DirectX::SimpleMath::Vector3 KnockBackPosition[4];

	EnemySword* GetSword() { return sword.get(); }
	Character* GetCharacter() { return this; }

	std::string GetName() override { return "EnemyBoss"; }

	// ノード当たり判定半径配列
	float nodeRadius[9] =
	{
		0.4f, 0.3f, 0.3f, 0.3f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f
	};

protected:
	// 死亡したときに呼ばれる
	void OnDead() override;

private:
	DirectX::XMFLOAT3	targetPosition = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3	territoryOrigin = { 0.0f,0.0f,0.0f };
	float				territoryRange = 255.0f;
	float				moveSpeed = 2.5f;
	float				turnSpeed = DirectX::XMConvertToRadians(360);
	float				searchRange = 20.0f;
	float				attackRange = 2.0f;
	float				runTimer = 0.0f;
	float 			    blendSeconds = 0.1f;
	bool 				isBattle = false;
	int 				step = 0;
	int 				revengeValue = 0;

	BehaviorTree<EnemyBoss>* aiTree = nullptr;
	std::unique_ptr<BehaviorData<EnemyBoss>> behaviorData = nullptr;
	NodeBase<EnemyBoss>* activeNode = nullptr;
	RandomState<EnemyBoss>* randomState = nullptr;
	SequenceState<EnemyBoss>* sequenceState = nullptr;
	std::unique_ptr<RootState<EnemyBoss>> rootState = nullptr;
	std::unique_ptr<EnemySword> sword;

	//ポイントライト
	//std::vector<AnimatedLight> AnimPointLights;

	std::vector<std::string> NodeName;
};