#pragma once

class HitStop
{
public:
	HitStop() {}
	~HitStop() {}

	// 唯一のインスタンス取得
	static HitStop& Instance()
	{
		static HitStop instance;
		return instance;
	}

	// 更新処理
	void Update(float elapsedTime);

	// ヒットストップ開始処理
	//void HitStopStart(float stopTime, float stopSpeed);
	void HitStopStart(float playerStopTime, float playerStopSpeed, float enemyStopTime, float enemyStopSpeed);

	// プレイヤータイムスケール取得
	float GetPlayerTimeScale() { return playerTimeScale; }
	// エネミータイムスケール取得
	float GetEnemyTimeScale() { return enemyTimeScale; }

	// プレイヤーヒットストップ状態取得
	bool GetPlayerHitStop() { return playerHitStop; }
	// エネミーヒットストップ状態取得
	bool GetEnemyHitStop() { return enemyHitStop; }

private:
	// ヒットストップに必要な変数
	bool hitStop = false;
	float playerHitStopLastSeconds = 0.0f;
	float enemyHitStopLastSeconds = 0.0f;
	float playerTimeScale = 1.0f;
	float enemyTimeScale = 1.0f;
	bool playerHitStop = false;
	bool enemyHitStop = false;
};
