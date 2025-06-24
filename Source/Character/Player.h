#pragma once

#include "Graphics/Graphics.h"
#include "Model/Model.h"
#include "Character.h"
#include <memory>
#include <functional>
#include "Sprite/Sprite.h"
#include "Character/Weapon/Sword.h"
//#include "Character/Enemy/Enemy.h"

// 前方宣言
class PlayerState;

// プレイヤーのステートID
enum class PlayerStateId
{
	Idle,
	Walk,
	Run,
	Dodge,
	DodgeAttack,
	Combo1,
	Combo2,
	Combo3,
	Combo4,
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

	// 描画処理
	void Render(const RenderContext& rc, ShaderId shaderId);

	////影描画処理
	//void ShadowRender(const RenderContext& rc, ShadowMap* shadowMap);

	////影用モデル設定
	//void SetShadowMap(ShadowMap* shadowMap);

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

	//// ロックオンしている敵を取得
	//Enemy* GetLockOnEnemy() { return LockOnEnemy; }

	//// ロックオン
	//void SetLockOnCamera(Enemy* enemy) { LockOnEnemy = enemy; }

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

	// ロックオン時敵の方を向く処理
	void LockOnTurnToEnemy(float elapsedTime);

	// 移動設定
	void SetMovement(DirectX::XMFLOAT3& Vec, float moveRate);

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
		0.23f,
		0.23f,
		0.43f,
		0.34f,
		0.31f,
		0.31f,
	};

	// ダークナイト
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

	// 攻撃位置へ移動
	//bool AttackMoveToTarget(float elapsedTime);

	// 攻撃振り向き処理
	//void AttackRotation(float elapsedTime);

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

	//デバッグ用
	bool isCollisionRender = false;//当たり判定描画フラグ

	//Enemy* LockOnEnemy = nullptr;

	// アニメーション変数
	PlayerStateId currentStateID = PlayerStateId::EnumCount;
	PlayerStateId lastStateID = PlayerStateId::EnumCount;
	std::unique_ptr<PlayerState> states[static_cast<int>(PlayerStateId::EnumCount)];
};

// ステート基底
class PlayerState
{
public:
	PlayerState(Player* player);
	virtual ~PlayerState() = default;

	// 開始処理
	virtual void Enter() {}

	// 終了処理
	virtual void Exit() {}

	// 更新処理
	virtual void Update(float elapsedTime) {}

	// デバッグ用GUI描画
	virtual void DrawDebugGUI() {}

protected:
	// ステート変更
	void ChangeState(PlayerStateId stateId);

	// 回避入力
	bool InputDodge() const;

	//// コンボ入力
	//bool InputCombo() const;

	// 歩き移動入力
	bool InputWalkMove() const;

	// 走り移動入力
	bool InputRunMove() const;

	// ガード入力
	bool InputGuard() const;

	// パリィ入力
	bool InputGuardParry() const;

	// ロックオンしている場合はストレイフ
	void LockOnStrafe(int rightIndex, int leftIndex, int frontIndex, int backIndex);

	enum class InputComboType
	{
		None,
		Light,
		Heavy,
	};

	InputComboType InputCombo();

protected:
	Player* player = nullptr;
	std::unordered_map<InputComboType, PlayerStateId> inputToNextState;
};

// 待機ステート
class PlayerIdleState : public PlayerState
{
public:
	PlayerIdleState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int idleAnimationIndex = -1;
	float idleAnimationSpeed = 1.0f;
};

// 歩きステート
class PlayerWalkState : public PlayerState
{
public:
	PlayerWalkState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int walkFrontAnimationIndex = -1;
	int walkBackAnimationIndex = -1;
	int walkRightAnimationIndex = -1;
	int walkLeftAnimationIndex = -1;
	float walkAnimationSpeed = 1.0f;
	float walkAnimationMoveRate = 0.45f;
};

// 走りステート
class PlayerRunState : public PlayerState
{
public:
	PlayerRunState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int runAnimationIndex = -1;
	int runFrontAnimationIndex = -1;
	int runBackAnimationIndex = -1;
	int runRightAnimationIndex = -1;
	int runLeftAnimationIndex = -1;
	float runAnimationSpeed = 1.0f;
	float runAnimationMoveRate = 0.9f;
};

// 回避ステート
class PlayerDodgeState : public PlayerState
{
public:
	PlayerDodgeState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 終了処理
	void Exit() override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int dodgeBackAnimationIndex = -1;
	float dodgeBackAnimationSpeed = 1.0f;
	float dodgeBackAnimationTime = 0.7f;
	int dodgeBackMovePow = 2;
	int rollingFrontAnimationIndex = -1;
	int rollingBackAnimationIndex = -1;
	int rollingRightAnimationIndex = -1;
	int rollingLeftAnimationIndex = -1;
	int rollingFrontRightAnimationIndex = -1;
	int rollingFrontLeftAnimationIndex = -1;
	int rollingBackRightAnimationIndex = -1;
	int rollingBackLeftAnimationIndex = -1;
	float rollingFrontAnimationSpeed = 1.0f;
	float rollingFrontAnimationTime = 0.9f;
	int rollingFrontMovePow = 3;
	float timer = 0.0f;
	bool isDodgeBack = false;
	bool nextShiftReady = false;
};

// 回避攻撃ステート
class PlayerDodgeAttackState : public PlayerState
{
public:
	PlayerDodgeAttackState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

private:
	int dodgeAttackAnimationIndex = -1;
	float timer = 0.0f;
};

// コンボステート
class PlayerComboState : public PlayerState
{
public:
	PlayerComboState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 終了処理
	void Exit() override;

	// 更新処理
	void Update(float elapsedTime) override;

protected:
	PlayerStateId nextStateId = PlayerStateId::EnumCount;
	InputComboType nextInput;
	int comboAnimationIndex = -1;
	float comboAttackSpeed = 1.0f;
	float comboPoseSpeed = 1.0f;
	float poseFrame = 0.0f;
	float endFrame = 0.0f;
	float nextShiftFrame = 0.0f;
	bool nextShiftReady = false;

	// 敵との距離の移動値と回転値
	float moveRate = 1.0f;
	float turnRate = 1.0f;

	float forwardPower = 0.0f;
	float forwardFrame = 0.0f;
	bool forwarded = false;

	// 攻撃判定必要変数
	float attackCollisionStartFrame = 0.0f;
	float attackCollisionEndFrame = 1.0f;
	int	  attackDamage = 1.0f;
	float invisibleTime = 0.5f;

	// コントローラーの振動変数
	float attackLeftVibrate = 1.0f;
	float attackRightVibrate = 1.0f;

	// 攻撃時ヒットストップ変数
	float attackHitStopTime = 1.0f;
	float attackHitStopSpeed = 0.1f;
};

// コンボス1ステート
class PlayerCombo1State : public PlayerComboState
{
public:
	PlayerCombo1State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// コンボス2ステート
class PlayerCombo2State : public PlayerComboState
{
public:
	PlayerCombo2State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// コンボス3ステート
class PlayerCombo3State : public PlayerComboState
{
public:
	PlayerCombo3State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// コンボス4ステート
class PlayerCombo4State : public PlayerComboState
{
public:
	PlayerCombo4State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// 強攻撃1ステート
class PlayerHeavyAttack1State : public PlayerComboState
{
public:
	PlayerHeavyAttack1State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// 強攻撃2ステート
class PlayerHeavyAttack2State : public PlayerComboState
{
public:
	PlayerHeavyAttack2State(Player* player);

	// デバッグ用GUI描画
	void DrawDebugGUI() override;
};

// ガード待機ステート
class PlayerGuardIdle : public PlayerState
{
public:
	PlayerGuardIdle(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 終了処理
	void Exit() override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int guardIdleAnimationIndex = -1;
	float guardIdleAnimationSpeed = 1.0f;
};

// ガード歩きステート
class PlayerGuardWalk : public PlayerState
{
public:
	PlayerGuardWalk(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 終了処理
	void Exit() override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int guardFrontWalkAnimationIndex = -1;
	int guardBackWalkAnimationIndex = -1;
	int guardRightWalkAnimationIndex = -1;
	int guardLeftWalkAnimationIndex = -1;
	float guardWalkAnimationSpeed = 1.0f;
	float guardWalkAnimationMoveRate = 0.6f;

	int currentGuardWalkAnimationIndex = -1;
};

// ガードヒットステート
class PlayerGuardHit : public PlayerState
{
public:
	PlayerGuardHit(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 終了処理
	void Exit() override;

private:
	int guardHitAnimationIndex = -1;
	float timer = 0.0f;
};

// ガードパリィステート
class PlayerGuardParry : public PlayerState
{
public:
	PlayerGuardParry(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI() override;

private:
	int guardParryAnimationIndex = -1;
	float guardParryAnimationSpeed = 1.0f;
	float timer = 0.0f;

	// 攻撃判定必要変数
	float parryCollisionStartFrame = 0.03f;
	float parryCollisionEndFrame = 0.09f;
};

// ダメージステート
class PlayerDamageState : public PlayerState
{
public:
	PlayerDamageState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

private:
	int damageAnimationIndex = -1;
};

// 死亡ステート
class PlayerDeadState : public PlayerState
{
public:
	PlayerDeadState(Player* player);

protected:
	// 開始処理
	void Enter() override;

	// 更新処理
	void Update(float elapsedTime) override;

private:
	int deadAnimationIndex = -1;
	bool notificated = false;
};
