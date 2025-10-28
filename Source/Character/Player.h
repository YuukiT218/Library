#pragma once

#include "Graphics/Graphics.h"
#include "Model/Model.h"
#include "Character.h"
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
	DodgeAttack,
	Combo1,
	Combo2,
	Combo3,
	Combo4,
	Combo5,
	Heavy1,
	Heavy2,
	GuardIdle,
	GuardWalk,
	GuardHit,
	GuardParry,
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

	//最大体力取得
	float GetMaxHealth() const { return maxHealth; }

	//体力取得
	float GetHealth() const { return health; }

	// ロックオンしている敵を取得
	Enemy* GetLockOnEnemy() { return LockOnEnemy; }

	// ロックオン
	void SetLockOnCamera(Enemy* enemy) { LockOnEnemy = enemy; }

	// プレイヤーの回避状態取得
	bool GetPlayerIsRolling() { return isRolling; }

	// プレイヤーの回避状態設定
	void SetPlayerRolling(bool rollingFlag) { isRolling = rollingFlag; }

	// プレイヤーのガード状態取得
	bool GetPlayerIsGuard() { return isGuard; }

	// プレイヤーのガード状態設定
	void SetPlayerGuard(bool guardFlag) { isGuard = guardFlag; }

	// プレイヤーのパリィ状態取得
	bool GetPlayerIsParry() { return isParry; }

	// プレイヤーのパリィ状態設定
	void SetPlayerParry(bool flag) { isParry = flag; }

	// モデル取得
	std::shared_ptr<Model> GetPlayerModel() { return model; }

	// ステート切り替え
	void ChangeState(PlayerStateId stateId);

	// 移動処理
	void PlayerMove(float elapsedTime, float moveRate = 1.0f, float turnRate = 1.0f);

	// ジャンプ処理
	void PlayerJump(float speed);

	// ロックオン時敵の方を向く処理
	void LockOnTurnToEnemy(float elapsedTime);

	// 移動設定
	void SetMovement(DirectX::XMFLOAT3& Vec, float moveRate);

	// 摩擦力設定
	void SetFriction(float friction) { this->friction = friction; }

	// 水平移動力設定
	void SetHorizonVelocity(float velocity) { this->velocity.x = this->velocity.z = velocity; }

	// 垂直移動力設定
	void SetVerticalVelocity(float velocity) { this->velocity.y = velocity; }

	Model* GetModel() { return model.get(); }
	Sword* GetSword() { return sword.get(); }

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

	DirectX::SimpleMath::Vector3 knockbackPosition;
	float knockBackPower = 2.0f;
	DirectX::SimpleMath::Vector3 lightKnockbackPosition;
	float lightKnockBackPower = 4.3f;
	DirectX::SimpleMath::Vector3 heavyKnockbackPosition;
	float heavyKnockBackPower = 7.0f;
	DirectX::SimpleMath::Vector3 launchKnockbackPosition;
	float launchKnockBackPower = 3.0f;

protected:
	// 着地した時に呼ばれる
	void OnLanding() override;

	// ダメージを受けた時に呼ばれる
	void OnDamaged() override;

	// 死亡した時に呼ばれる
	void OnDead() override;

private:
	std::unique_ptr<Sword> sword;

	float nodeRadius[6] =
	{
		0.15f,
		0.15f,
		0.2f,
		0.15f,
		0.1f,
		0.1f,
	};

	std::vector<NodeHitSphere> nodeHitSpheres =
	{
		{"Character1_LeftLeg", nodeRadius[0]},
		{"Character1_RightLeg", nodeRadius[1]},
		{"Character1_Spine", nodeRadius[2]},
		{"Character1_Head", nodeRadius[3]},
		{"Character1_LeftForeArm", nodeRadius[4]},
		{"Character1_RightForeArm", nodeRadius[5]},
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

	// 回避入力処理
	//bool InputRolling();

	// プレイヤーとエネミーの衝突処理
	void CollisionPlayerVsEnemies();

	//// ノードとエネミーの衝突処理
	//void CollisionNodeVsEnemies(Object* object, float nodeRadius, int AttackDamage, float invicibleTime = 0.5f);

	//// アニメーションの攻撃当たり判定を付ける
	//void AttackAnimationCollision(Object* object, float animTimeMin, float animTimeMax, int AttackDamage, float invicibleTime = 0.5f);

	// 攻撃入力処理
	bool InputAttack();

	// 体力ゲージ表示
	void DisplayHealthBar();

	// ヒットストップ処理
	void HitStop(float elapsedTime);

	// ノックバック位置設定
	void SetKnockbackPosition();

	// ステート取得
	PlayerState& GetState(PlayerStateId stateId)
	{
		return *states[static_cast<int>(stateId)];
	}

private:
	//std::shared_ptr<Model> model = nullptr;
	float moveSpeed = 7.0f;
	float turnSpeed = DirectX::XMConvertToRadians(1080);
	float jumpSpeed = 20.0f;
	int jumpCount = 0;
	int jumpLimit = 2;
	float leftHandRadius = 0.4f;
	bool attackCollisionFlag = false;
	bool leadingInputFlag = false;	// 先行入力フラグ
	bool isRolling = false;
	bool isGuard = false;
	bool isParry = false;
	/*std::unique_ptr<Effect> effect = nullptr;
	Effekseer::Handle effectHandle;
	std::unique_ptr<Sprite> healthBar = nullptr;*/

	Enemy* LockOnEnemy = nullptr;

	// アニメーション変数
	PlayerStateId currentStateID = PlayerStateId::EnumCount;
	PlayerStateId lastStateID = PlayerStateId::EnumCount;
	std::unique_ptr<PlayerState> states[static_cast<int>(PlayerStateId::EnumCount)];
};

