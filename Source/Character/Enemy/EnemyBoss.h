#pragma once
#include "Graphics/Graphics.h"
#include "Model/Model.h"
#include "Character/Enemy/Enemy.h"
#include "Character/Weapon/EnemySword.h"
#include "Character/Enemy/BehaviorTree/BehaviorTreeAsset.h"

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
	~EnemyBoss() override;

	// 更新処理
	void UpdateEnemySpecific(float elapsedTime)override;

	// エディター用更新処理
	void EditUpdate(float elapsedTime) override;

	// 描画処理
	void Render(const RenderContext& rc, ShaderId shaderId)override;

	//影描画処理
	void ShadowRender(const RenderContext& rc, ShadowMap* shadowMap);

	// テレポートの粒子分解に武器も巻き込む
	std::vector<std::shared_ptr<Model>> GetAfterimageAttachments() const override;

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
	bool isGameClear = false;

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

	// 反撃値加算
	void AddRevengeValue(int num) { revengeValue += num; }

	// 反撃値減算
	void SubRevengeValue() { revengeValue -= 1; }

	// 反撃値取得
	int GetRevengeValue() const { return revengeValue; }

	// 反撃許容値設定
	void SetRevengeTolerance(int num) { revengeTolerance = num; }

	// 反撃値リセット
	void ResetRevengeValue() { revengeValue = 0; }

	// 反撃状態設定
	void SetRevengeState(bool state) { isRevenge = state; }

	// 反撃状態取得
	bool GetRevengeState() { return isRevenge; }

	void SetSpecialReady(bool flag) { specialReady = flag; }
	bool GetSpecialReady() { return specialReady; }

	void SetSearchRange(float range) { searchRange = range; }

	void SetPlayedEffect(bool flag) { playedEffect = flag; }
	bool GetPlayedEffect() { return playedEffect; }

	float GetBlendSeconds() { return blendSeconds; }

	float GetMoveSpeed() { return moveSpeed; }

	void SetVerticalVelocity(float velocity) { this->velocity.y = velocity; }

	void SetWarpPosition();

	void SetKnockBackPosition();

	DirectX::SimpleMath::Vector3 WarpPosition[3];

	EnemySword* GetSword() { return sword.get(); }
	Character* GetCharacter() { return this; }

	std::string GetName() override { return "EnemyBoss"; }

	//----------------------------------------------------------------
	// ビヘイビアツリー（エディタから触る部分）
	//----------------------------------------------------------------

	// JSON からツリーを読み込んで組み立てる。
	// ファイルが無い・壊れている場合は既定の構成にフォールバックする。
	void LoadBehaviorTree();

	// エディタで編集したアセットでツリーを作り直す。
	// 失敗したら false を返し、それまでのツリーは壊れたままにならない。
	bool ApplyBehaviorAsset(const BehaviorTreeAsset& asset, std::vector<std::string>& problems);

	// 現在のツリー定義
	const BehaviorTreeAsset& GetBehaviorAsset() const { return behaviorAsset; }

	// 組み立て時に出た問題（エディタに表示する）
	const std::vector<std::string>& GetBehaviorProblems() const { return behaviorProblems; }

	// 今実行しているノードの元 ID。実行していなければ -1
	int GetActiveNodeSourceId() const;

	// 実行中の行動を中断してツリーの推論からやり直す
	void ResetBehaviorState();

	// ノード当たり判定半径配列
	float nodeRadius[9] =
	{
		0.4f, 0.3f, 0.3f, 0.3f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f
	};

	std::unique_ptr<Effect> lightBall = nullptr;
	std::unique_ptr<Effect> attackSign = nullptr;
	std::unique_ptr<Effect> teleportEffect = nullptr;
	std::unique_ptr<Effect> magicCircle = nullptr;
	Effekseer::Handle handle = -1;
	bool playedFadeEffect = false;

protected:
	// 死亡したときに呼ばれる
	void OnDead() override;

	void OnTeleportPhaseChanged(TeleportPhase newPhase) override;
private:
	DirectX::XMFLOAT3	targetPosition = { 0.0f,0.0f,0.0f };
	DirectX::XMFLOAT3	territoryOrigin = { 0.0f,0.0f,0.0f };
	float				territoryRange = 255.0f;
	float				moveSpeed = 2.5f;
	float				turnSpeed = DirectX::XMConvertToRadians(360);
	float				searchRange = 25.0f;
	float				attackRange = 5.0f;
	float				runTimer = 0.0f;
	float 			    blendSeconds = 0.1f;
	float 				teleportOffset = 17.5f;
	bool 				isBattle = false;
	bool				isRevenge = false;
	bool				playedEffect = false;
	bool				specialReady = true;
	int 				step = 0;
	int 				revengeValue = 0;		// 反撃値
	int					revengeTolerance = 30;	// 反撃許容値

	BehaviorTreeAsset	behaviorAsset;
	std::vector<std::string> behaviorProblems;

	BehaviorTree<EnemyBoss>* aiTree = nullptr;
	std::unique_ptr<BehaviorData<EnemyBoss>> behaviorData = nullptr;
	NodeBase<EnemyBoss>* activeNode = nullptr;
	RandomState<EnemyBoss>* randomState = nullptr;
	SequenceState<EnemyBoss>* sequenceState = nullptr;
	std::unique_ptr<RootState<EnemyBoss>> rootState = nullptr;
	std::unique_ptr<EnemySword> sword;
	std::vector<std::string> NodeName;
};