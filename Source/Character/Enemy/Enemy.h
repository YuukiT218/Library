#pragma once

#include "Character/Character.h"
#include "Model/Model.h"
#include "Graphics/Graphics.h"
//#include "System/AnimationConfigLoader.h"

enum class EnemyType
{
	Slime,
	Cactus,
	Mushroom,
	SilverDragonkin
};

class Enemy : public Character
{
public:

	Enemy() {}
	~Enemy() override {}

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	virtual void Render(const RenderContext& rc, ShaderId shaderId) = 0;

	// 破棄
	void Destroy();

	// ノードとプレイヤーの衝突処理
	void CollisionNodeVsPlayer(std::vector<NodeHitSphere> attackSpheres, int AttackDamage, float invicibleTime);

	// アニメーションの攻撃当たり判定を付ける
	void AttackAnimationCollision(std::vector<NodeHitSphere> attackSpheres, float animTimeMin, float animTimeMax, int AttackDamage, float invicibleTime = 0.5f);

	// ブレスエフェクトの当たり判定を付ける
	void BreathEffectCollision(float animTimeMin, float animTimeMax, DirectX::XMFLOAT3 startPosition, DirectX::XMFLOAT3 direction, float length, float sphereRadius, int sphereCount, int AttackDamage, float invicibleTime);

	// デバッグプリミティブ描画
	virtual void DrawDebugPrimitive();

	// デバッグエネミー情報表示
	virtual void DrawDebugGUI();
	virtual void DrawDebugChildGUI() {};

	// 攻撃フラグ取得
	bool GetAttackFlg() { return attackFlg; }
	// 攻撃フラグセット
	void SetAttackFlg(bool flg) { attackFlg = flg; };
	virtual void	SetId(int id) { this->id = id; }
	virtual int		GetId() { return id; }
	// 攻撃範囲取得
	virtual float GetAttackRange() { return attackRange; }

	virtual void SetTerritory(DirectX::XMFLOAT3 position, float range) {};

	virtual EnemyType GetEnemyType() const = 0; // 純粋仮想関数

	Model* GetModel() { return model.get(); }

	int GetDeathCount() { return deathCount; }

	// パリィやジャスト回避が成功している場合判定をモーション終了まで消す用のフラグ
	bool isPlayerInvincible = false;

	// 当たり判定構造体取得
	std::vector<NodeHitSphere> GetNodeHitSpheres() { return nodeHitSpheres; }

	std::vector<NodeHitSphere> nodeHitSpheres;

	// 攻撃判定取得
	std::vector<NodeHitSphere> GetAttackHitSpheres() { return attackHitSpheres; }

	std::vector<NodeHitSphere> attackNodeHitSpheres;

	virtual void EditUpdate(float elapsedTime);

protected:
	// 派生クラス専用の更新処理
	virtual void UpdateEnemySpecific(float elapsedTime) {}

protected:
	std::shared_ptr<Model> model = nullptr;
	int	state = 0;
	bool attackFlg = false;
	int id = 0;
	float searchRange = 0.0f;
	float attackRange = 0.0f;

	int deathCount = 0;

	std::vector<NodeHitSphere> attackHitSpheres;

private:
	bool isFocus = false;
public:
	void SetFocus(bool focus) { isFocus = focus; }
	bool IsFocus() const { return isFocus; }
	void SetDamage(bool damage) { isDamage = damage; }
	bool IsDamage() const { return isDamage; }
	bool actionFlag = true;
	bool isDamage = false;
	std::string name = " ";

	virtual std::string GetName() = 0;
};

