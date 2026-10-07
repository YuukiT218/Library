#pragma once

#include "Graphics/Graphics.h"
#include "Model/Model.h"
#include "Character/Character.h"
#include <memory>
#include <functional>
#include "Sprite/Sprite.h"
#include "Character/Weapon/Sword.h"
#include "Character/Enemy/Enemy.h"

// 前方宣言
class PlayerState;

// プレイヤーのステートID
enum class PlayerStateId
{
	Idle,
	Walk,
	Run,
	Jump,
	Fall,
	Dodge,
	Combo1,
	Combo2,
	Combo3,
	Combo4,
	Combo5,
	Heavy1,
	GuardIdle,
	GuardHit,
	GuardCounter,
	Damage,
	Dead,

	EnumCount
};

// プレイヤー
class Player : public Character
{
public:
	Player(ID3D11Device* device, const char* filename, float scale = 1.0f);
	~Player() override;

	// インスタンス取得
	static Player& Instance();

	// 更新処理
	void Update(float elapsedTime);

	// エディタ用更新処理
	void EditUpdate(float elapsedTime);

	// ImGui がマウス／キーボードを掴んでいても操作を受け付けるようにする。
	// エディタの Game View にフォーカスがあるときに立てる。
	void SetInputForced(bool forced) { inputForced = forced; }
	bool IsInputForced() const { return inputForced; }

	// 描画処理
	void Render(const RenderContext& rc, ShaderId shaderId);

	//影描画処理
	void ShadowRender(const RenderContext& rc, ShadowMap* shadowMap);

	// デバッグプリミティブ描画
	void DrawDebugPrimitive();

	// デバッグ用GUI描画
	void DrawDebugGUI();

	// 当たり判定構造体取得
	std::vector<NodeHitSphere> GetNodeHitSpheres() { return nodeHitSpheres; }

	// 回転速度取得
	float GetTurnSpeed() const { return turnSpeed; }

	// ロックオンしている敵を取得
	Enemy* GetLockOnEnemy() { return lockOnEnemy; }

	// ロックオン
	void SetLockOnCamera(Enemy* enemy) { lockOnEnemy = enemy; }

	// プレイヤーの回避状態取得
	bool IsRolling() { return isRolling; }

	// プレイヤーの回避状態設定
	void SetRolling(bool rollingFlag) { isRolling = rollingFlag; }

	// プレイヤーのガード状態取得
	bool IsGuard() { return isGuard; }

	// プレイヤーのガード状態設定
	void SetGuard(bool guardFlag) { isGuard = guardFlag; }

	// ガードカウンター可能か状態取得
	bool IsStandbyCounter() { return isStandbyCounter; }

	// ガードカウンター可能か状態設定
	void SetStandbyCounter(bool flag) { isStandbyCounter = flag; }

	// プレイヤーのパリィ状態取得
	bool IsParry() { return isParry; }

	// プレイヤーのパリィ状態設定
	void SetParry(bool flag) { isParry = flag; }

	// モデル取得
	std::shared_ptr<Model> GetPlayerModel() { return model; }

	// ステート切り替え
	void ChangeState(PlayerStateId stateId);

	// 現在のステートID取得 (追加)
	PlayerStateId GetCurrentStateId() const { return currentStateId; }

	// 移動処理
	void PlayerMove(float elapsedTime, float moveRate = 1.0f, float turnRate = 1.0f);

	// 入力方向に回転する処理
	void PlayerTurn(float elapsedTime, float turnRate);

	// ジャンプ処理
	void PlayerJump(float speed);

	// ロックオン時敵の方を向く処理（完了判定付き）
	bool LockOnTurnToEnemy(float elapsedTime);

	// 旋回完了フラグのリセット
	void ResetTurnCompleted() { isTurnCompleted = false; }

	// 旋回が完了しているか
	bool IsTurnCompleted() const { return isTurnCompleted; }

	// 旋回完了の閾値を設定
	void SetTurnCompletedThreshold(float degrees)
	{
		turnCompletedThreshold = DirectX::XMConvertToRadians(degrees);
	}

	// 移動設定
	void SetMovement(DirectX::XMFLOAT3& vec, float moveRate);

	// 摩擦力設定
	void SetFriction(float friction) { this->friction = friction; }

	// 水平移動力設定
	void SetHorizonVelocity(float velocity) { this->velocity.x = this->velocity.z = velocity; }

	// 垂直移動力設定
	void SetVerticalVelocity(float velocity) { this->velocity.y = velocity; }

	Sword* GetSword() { return sword.get(); }
	std::string GetName() override { return "Player"; }

	// ステック入力値から移動ベクトルを取得
	DirectX::XMFLOAT3 GetMoveVec() const;

	// タイトル遷移要求コールバック
	std::function<void()> onExitToTitle;

	enum class DeathMenuOption
	{
		Continue,
		Exit
	};
	DeathMenuOption currentSelection = DeathMenuOption::Continue;
	int GetCurrentSelection() { return static_cast<int>(currentSelection); }

	// 敵をノックバック・テレポートさせる位置（プレイヤー前方）
	DirectX::SimpleMath::Vector3 knockbackPosition;
	float knockbackDistance = 2.0f;
	DirectX::SimpleMath::Vector3 lightKnockbackPosition;
	float lightKnockbackDistance = 4.3f;
	DirectX::SimpleMath::Vector3 heavyKnockbackPosition;
	float heavyKnockbackDistance = 7.0f;
	DirectX::SimpleMath::Vector3 launchKnockbackPosition;
	float launchKnockbackRise = 3.0f;

	// ノックバック強度
	float normalKnockbackPower = -1.0f;
	float lightKnockbackPower = -4.0f;
	float heavyKnockbackPower = -7.0f;
	float launchKnockbackHeight = 5.0f;
	float knockdownKnockbackPower = -1.0f;

	// ダメージタイプ判定用のフラグゲッター・セッター
	bool IsDamage() const { return isDamage; }
	void SetDamage(bool flag) { isDamage = flag; }

	bool IsLightDamage() const { return isLightDamage; }
	void SetLightDamage(bool flag) { isLightDamage = flag; }

	bool IsHeavyDamage() const { return isHeavyDamage; }
	void SetHeavyDamage(bool flag) { isHeavyDamage = flag; }

	bool IsLaunchDamage() const { return isLaunchDamage; }
	void SetLaunchDamage(bool flag) { isLaunchDamage = flag; }

	bool IsKnockdownDamage() const { return isKnockdownDamage; }
	void SetKnockdownDamage(bool flag) { isKnockdownDamage = flag; }

	// 任意のダメージを受けているか
	bool IsAnyDamage() const
	{
		return isDamage || isLightDamage || isHeavyDamage || isLaunchDamage || isKnockdownDamage;
	}

	// 攻撃を受けた方向を記録
	void SetDamageDirection(const DirectX::XMFLOAT3& attackerPos);

	// ダメージ方向取得
	DirectX::XMFLOAT3 GetDamageDirection() const { return damageDirection; }

	// ノックバック位置を計算（攻撃を受けた方向から後方へ）
	DirectX::XMFLOAT3 CalculateKnockbackPosition(float power);

	std::unique_ptr<Effect> guardEffect = nullptr;
	Effekseer::Handle effectHandle;

protected:
	// 着地した時に呼ばれる
	void OnLanding() override;

	// ダメージを受けた時に呼ばれる
	void OnDamaged() override;

	// 死亡した時に呼ばれる
	void OnDead() override;

private:
	std::unique_ptr<Sword> sword;

	// 全身の当たり判定（ノード名と半径）
	std::vector<NodeHitSphere> nodeHitSpheres =
	{
		{"Character1_LeftLeg", 0.15f},
		{"Character1_RightLeg", 0.15f},
		{"Character1_Spine", 0.2f},
		{"Character1_Head", 0.15f},
		{"Character1_LeftForeArm", 0.1f},
		{"Character1_RightForeArm", 0.1f},
		{"Character1_Spine2", 0.2f},
		{"Character1_LeftShoulder", 0.1f},
		{"Character1_RightShoulder", 0.1f},
		{"Character1_LeftFoot", 0.1f},
		{"Character1_RightFoot", 0.1f},
	};

private:
	// 移動入力処理
	// 入力された時にtrueを返すようにする。
	float InputMove(float elapsedTime);

	// 前方向の移動値
	void ForwardMove(float speed, float elapsedTime);

	// 後ろ方向の移動値
	void BackMove(float speed, float elapsedTime);

	// ジャンプ入力処理
	bool InputJump();

	// プレイヤーとエネミーの衝突処理
	void CollisionPlayerVsEnemies();

	// ノックバック位置設定
	void SetKnockbackPosition();

	// ステート取得
	PlayerState& GetState(PlayerStateId stateId)
	{
		return *states[static_cast<int>(stateId)];
	}

private:
	float moveSpeed = 7.0f;
	float turnSpeed = DirectX::XMConvertToRadians(1080.0f);
	float jumpSpeed = 20.0f;
	int jumpCount = 0;
	int jumpLimit = 2;
	// エディタから操作を通したいときに立つ
	bool inputForced = false;

	bool isRolling = false;
	bool isGuard = false;
	bool isStandbyCounter = false;
	bool isParry = false;
	bool isDamage = false;
	bool isLightDamage = false;
	bool isHeavyDamage = false;
	bool isLaunchDamage = false;
	bool isKnockdownDamage = false;

	// 攻撃を受けた方向
	DirectX::XMFLOAT3 damageDirection = { 0.0f, 0.0f, 0.0f };

	// ロックオン時の旋回完了フラグ
	bool isTurnCompleted = false;
	float turnCompletedThreshold = DirectX::XMConvertToRadians(3.0f);  // 3度以内なら完了とみなす

	Enemy* lockOnEnemy = nullptr;

	// アニメーション変数
	PlayerStateId currentStateId = PlayerStateId::EnumCount;
	PlayerStateId lastStateId = PlayerStateId::EnumCount;
	std::unique_ptr<PlayerState> states[static_cast<int>(PlayerStateId::EnumCount)];
};

