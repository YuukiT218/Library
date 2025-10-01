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

class SilverDragonkin : public Enemy
{
public:
	static SilverDragonkin& Instance();

	SilverDragonkin(ID3D11Device* device, const char* filename, float scale);
	~SilverDragonkin()override;

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
	void TurnToTarget(float elapsedTime, float vx, float vz, float speed);

	// プレイヤー索敵
	bool SearchPlayer();

	// プレイヤーの方向を向ききったか判定
	bool IsTurnToTarget(float vx, float vz);

	// 咆哮使用済みフラグ
	bool RoarUsedFlag = false;

	// 咆哮を使用中かどうか
	bool IsRoarUsing = false;

	bool AngryBGMStart = false; // 怒りBGM開始フラグ

	bool isTurnAnimation = false;

	// ゲームクリアシーン遷移可能フラグ
	bool IsGameClear = false;

	// ターゲットポジション設定
	void SetTargetPosition(DirectX::XMFLOAT3 position) { targetPosition = position; }

	// ターゲットポジション取得
	DirectX::XMFLOAT3 GetTargetPosition() { return targetPosition; }

	// ポジション取得
	DirectX::XMFLOAT3 GetPosition() { return position; }

	DirectX::XMFLOAT3 GetAngle() { return angle; }

	// 攻撃範囲取得
	float GetAttackRange() { return attackRange; }

	// ステート取得
	RootState<SilverDragonkin>* GetState() const { return rootState.get(); }

	// デバッグエネミー情報表示
	void DrawDebugGUI();

	// 実行タイマー設定
	void SetRunTimer(float timer) { runTimer = timer; }

	// 実行タイマー取得
	float GetRunTimer() { return runTimer; }

	float GetBlendSeconds() { return blendSeconds; }

	float GetMoveSpeed() { return moveSpeed; }

	bool GetIsRoarUsing() const { return IsRoarUsing; }
	bool GetRoarUsed() const { return RoarUsedFlag; }

	std::shared_ptr<Model> GetEnemyModel() { return model; }

	EnemyType GetEnemyType() const override { return EnemyType::SilverDragonkin; }
	std::string GetName() { return "SilverDragonkin"; }
	/*std::unique_ptr<Effect> breathEffect = nullptr;
	std::unique_ptr<Effect> attackTelegraphEffect = nullptr;*/

	float nodeRadius[9] =
	{
		0.3f, 0.3f, 0.3f, 0.3f, 0.2f, 0.2f, 0.2f, 0.2f, 0.2f
	};

	float nodeRadiusAttack[29] =
	{
		// 前右脚
		1.1f, 1.15f, 0.85f,
		// 前左脚
		1.1f, 1.15f, 0.85f,
		// 後右脚
		1.0f, 1.0f, 1.0f, 1.0f,
		// 後左脚
		1.0f, 1.0f, 1.0f, 1.0f,
		// 胴体
		1.7f, 1.65f, 1.45f, 1.95f, 1.4f, 1.7f, 1.7f,
		// 尻尾
		2.0f, 1.8f, 1.3f, 1.3f, 1.3f, 1.3f, 1.3f, 1.2f,
	};

	float attackFrameMin[7] =
	{
		2.6f,
		2.7f,
		1.0f,
		1.75f,
		2.0f,
		1.35f,
		1.15f
	};

	float attackFrameMax[7] =
	{
		3.1f,
		3.4f,
		1.8f,
		3.0f,
		3.9f,
		1.9f,
		1.9f
	};

	float attackDamage[2] =
	{
		10.0f,
		15.0f
	};

	void SetHealth(int setHealth) { health = setHealth; }
	void SetMaxHealth(int setMaxHealth) { maxHealth = setMaxHealth; }

	std::vector<NodeHitSphere> rightArmNodeHitSpheres;
	std::vector<NodeHitSphere> tailNodeHitSpheres;
	std::vector<NodeHitSphere> tackleHitSpheres;

private:
	void ShowDragonLightEditor();

protected:
	// 死亡したときに呼ばれる
	void OnDead() override;
public:
	// アニメーション
	enum class EnemyAnimation
	{
		Attack01 = 0,
		Attack02,
		Breath,
		BreathFlying,
		BreathSweeping,
		Die,
		IdleDying,
		IdleFlying,
		IdleNormal,
		Landing,
		Roar,
		SpinAttack,
		TakeOff,
		a,
		b,
		c,
		d,
		e,
		WalkFWD,
		WorldTour,
		TurnLeft,
		TurnRight,
		Tackle,
		Sidestep,
		f,
		g,
		h,
		GetHit
	};


private:
	DirectX::XMFLOAT3	targetPosition = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3	territoryOrigin = { 0.0f,0.0f,0.0f };
	float				territoryRange = 255.0f;
	float				moveSpeed = 2.5f;
	float				turnSpeed = DirectX::XMConvertToRadians(360);
	float				searchRange = 20.0f;
	float				attackRange = 1.5f;
	float				runTimer = 0.0f;
	float 			    blendSeconds = 0.5f;
	bool 				isBattle = false;
	int 				step = 0;

	std::unique_ptr<BehaviorTree<SilverDragonkin>> aiTree = nullptr;
	std::unique_ptr<BehaviorData<SilverDragonkin>> behaviorData = nullptr;
	NodeBase<SilverDragonkin>* activeNode = nullptr;
	RandomState<SilverDragonkin>* randomState = nullptr;
	SequenceState<SilverDragonkin>* sequenceState = nullptr;
	std::unique_ptr<RootState<SilverDragonkin>> rootState = nullptr;
	std::unique_ptr<EnemySword> sword;

	//ポイントライト
	//std::vector<AnimatedLight> AnimPointLights;

	std::vector<std::string> NodeName;
};