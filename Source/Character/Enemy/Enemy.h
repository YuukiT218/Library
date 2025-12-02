#pragma once

#include "Character/Character.h"
#include "Model/Model.h"
#include "Graphics/Graphics.h"

class Enemy : public Character
{
public:
	Enemy() {}
	~Enemy() override {}

	// 更新処理
	void Update(float elapsedTime);

	// 行列更新処理
	void UpdateTransform();

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

	// テレポート用の変数
	bool isTeleporting = false;                          // テレポート中フラグ
	DirectX::XMFLOAT3 visualPosition;                    // 見た目の位置（モデル描画用）
	DirectX::XMFLOAT3 logicalPosition;                   // 論理的な位置（カメラ追従用）
	DirectX::XMFLOAT3 teleportStartPosition;             // テレポート開始位置
	DirectX::XMFLOAT3 teleportTargetPosition;            // テレポート目標位置
	float teleportProgress = 0.0f;                       // テレポート進行度 (0.0 ~ 1.0)
	float teleportDuration = 0.5f;                       // テレポートにかかる時間（秒）
	float teleportVisualDelay = 0.1f;                    // 見た目の遅延時間（秒）
	float teleportTimer = 0.0f;							 // 経過時間を直接カウント

private:
	bool isFocus = false;

protected:
	bool isDamage = false;
	bool isLightKnockbackDamage = false;
	bool isHeavyKnockbackDamage = false;
	bool isLaunchKnockbackDamage = false;
public:
	void SetFocus(bool focus) { isFocus = focus; }
	bool IsFocus() const { return isFocus; }
	void SetDamage(bool damage) { isDamage = damage; }
	bool IsDamage() const { return isDamage; }
	void SetLightKbDamage(bool damage) { isLightKnockbackDamage = damage; }
	bool IsLightKbDamage() const { return isLightKnockbackDamage; }
	void SetHeavyKbDamage(bool damage) { isHeavyKnockbackDamage = damage; }
	bool IsHeavyKbDamage() const { return isHeavyKnockbackDamage; }
	void SetLaunchKbDamage(bool damage) { isLaunchKnockbackDamage = damage; }
	bool IsLaunchKbDamage() const { return isLaunchKnockbackDamage; }
	bool IsAnyDamage() const { return isDamage || isLightKnockbackDamage || isHeavyKnockbackDamage || isLaunchKnockbackDamage; }
	bool actionFlag = true;
	bool isTeleport = false;
	std::string name = " ";

	// テレポート開始
	void StartTeleport(const DirectX::XMFLOAT3& targetPos, float duration = 0.5f);

	// テレポート更新
	void UpdateTeleport(float elapsedTime);

	// テレポート中かどうか
	bool IsTeleporting() const { return isTeleporting; }

	// カメラが追従すべき位置を取得（論理位置）
	DirectX::XMFLOAT3 GetCameraTrackingPosition() const { return logicalPosition; }

	// 見た目の位置を取得（モデル描画用）
	DirectX::XMFLOAT3 GetVisualPosition() const { return isTeleporting ? visualPosition : position; }

};