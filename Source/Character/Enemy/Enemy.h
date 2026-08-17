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

	// ノードとプレイヤーの衝突処理
	void CollisionNodeVsPlayer(std::vector<NodeHitSphere> attackSpheres, int attackDamage, float invincibleTime);

	// アニメーションの攻撃当たり判定を付ける
	void AttackAnimationCollision(std::vector<NodeHitSphere> attackSpheres, float animTimeMin, float animTimeMax, int attackDamage, float invincibleTime = 0.5f);

	// ブレスエフェクトの当たり判定を付ける
	void BreathEffectCollision(float animTimeMin, float animTimeMax, DirectX::XMFLOAT3 startPosition, DirectX::XMFLOAT3 direction, float length, float sphereRadius, int sphereCount, int attackDamage, float invincibleTime);

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

	// テレポート中とスーパーアーマー中は、致命傷を受けても体力1で耐える
	// （テレポートは演出が途中で止まらないように、
	//   スーパーアーマーは動作を最後までやり切らせるため）
	bool ShouldSurviveLethalDamage() const override
	{
		return IsTeleporting() || IsSuperArmor();
	}

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
	void ResetDamage() { isDamage = isLightKnockbackDamage = isHeavyKnockbackDamage = isLaunchKnockbackDamage = false; }
	bool IsAnyDamage() const { return isDamage || isLightKnockbackDamage || isHeavyKnockbackDamage || isLaunchKnockbackDamage; }
	bool actionFlag = true;
	bool isTeleport = false;
	std::string name = " ";

	// カメラが追従すべき位置を取得（論理位置）
	DirectX::XMFLOAT3 GetCameraTrackingPosition() const { return teleportTargetPosition; }

	// 見た目の位置を取得（モデル描画用）
	DirectX::XMFLOAT3 GetVisualPosition() const { return isTeleporting ? visualPosition : position; }

	float GetTotalGameTime() const { return totalGameTime; }

protected:
	// テレポート用の変数
	bool isTeleporting = false;                          // テレポート中フラグ
	DirectX::XMFLOAT3 visualPosition;                    // 見た目の位置（モデル描画用）
	DirectX::XMFLOAT3 logicalPosition;                   // 論理的な位置（カメラ追従用）
	DirectX::XMFLOAT3 teleportStartPosition;             // テレポート開始位置
	DirectX::XMFLOAT3 teleportTargetPosition;            // テレポート目標位置
	float totalGameTime = 0.0f;
	float teleportPhaseTimer = 0.0f;
	float fadeOutDuration = 0.5f;   // 消失にかける時間
	float moveDuration = 0.1f;      // 移動にかける時間
	float fadeInDuration = 0.5f;    // 出現にかける時間

	// テレポート演出のフェーズ管理
	enum class TeleportPhase
	{
		None,              // テレポートしていない
		FadeOut,           // 消失演出
		Moving,            // 位置移動（見えない状態）
		FadeIn,            // 出現演出
	};
	TeleportPhase teleportPhase = TeleportPhase::None;

	virtual void OnTeleportPhaseChanged(TeleportPhase newPhase) {}

	// 残像用の変数
	struct Afterimage
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 angle;
		DirectX::XMFLOAT4X4 transform;
		float alpha;               // 透明度（1.0→0.0に減衰）
		float lifetime;            // 残り時間
		std::vector<Model::Node> nodes;  // ボーン情報をコピー
		float darkness;

		~Afterimage()
		{
			ClearNodes();
		}

		void ClearNodes()
		{
			if (!nodes.empty())
			{
				nodes.clear();
				nodes.shrink_to_fit();
			}
		}
	};

	std::vector<Afterimage> afterimages;         // 残像
	float afterimageDuration = 0.7f;			 // 残像1つあたりの持続時間
	float afterimageDarkness = 0.8f;

	// 残像の色味(rgb)と明るさ(a)
	// 1を超える明るさにするとブルームが乗って残像が光って見える
	DirectX::XMFLOAT4 afterimageColor = { 0.65f, 0.35f, 1.0f, 2.0f };

	// 1フレーム分の区間に並べるGPUパーティクルの数
	// 多いほど軌跡の線が密になる
	int teleportTrailEmitCount = 80;

	// 前回パーティクルを撒いた位置（区間の起点）
	DirectX::XMFLOAT3 teleportTrailPreviousPosition = { 0.0f, 0.0f, 0.0f };

public:
	// テレポート開始
	void StartTeleport(const DirectX::XMFLOAT3& targetPos, float fadeOutTime = 0.5f);

	// テレポート更新
	void UpdateTeleport(float elapsedTime);

	// テレポート中かどうか
	bool IsTeleporting() const { return teleportPhase != TeleportPhase::None; }

	// 現在のテレポート進行度（0.0〜1.0）
	float GetTeleportProgress() const;

	// 現在のテレポートフェーズ取得
	TeleportPhase GetTeleportPhase() const { return teleportPhase; }

	// 残像更新
	void UpdateAfterimage(float elapsedTime);

	// 現在の姿勢から残像を1つ生成する
	void SpawnAfterimage();

	// テレポートの軌跡をGPUパーティクルで撒く（区間の起点と終点を指定する）
	void EmitTeleportTrail(const DirectX::XMFLOAT3& from, const DirectX::XMFLOAT3& to);

	// 残像の色を取得
	const DirectX::XMFLOAT4& GetAfterimageColor() const { return afterimageColor; }

	// 残像発生フラグチェック
	bool HasAfterimage() const { return !afterimages.empty(); }

	// 残像情報取得
	const std::vector<Afterimage>& GetAfterimages() const { return afterimages; }

	// 指定した座標が画面内（スクリーン内）に入っているか判定
	bool IsPositionVisible(const DirectX::XMFLOAT3& worldPos);

	// プレイヤーの周囲から画面内のテレポート先を計算して返す
	// distance: プレイヤーからの距離
	DirectX::XMFLOAT3 CalculateVisibleTeleportPos(float distance, bool bakeY = false);
};