#include "HitStop.h"

// 更新処理
void HitStop::Update(float elapsedTime)
{
    if (hitStop)
    {
        // プレイヤーのヒットストップ時間のカウントダウン
        playerHitStopLastSeconds -= elapsedTime;
        if (playerHitStopLastSeconds <= 0.0f)
        {
            // リセット
            playerHitStopLastSeconds = 0.0f;
            playerTimeScale = 1.0f;
            playerHitStop = false;
        }

        // エネミーのヒットストップ時間のカウントダウン
        enemyHitStopLastSeconds -= elapsedTime;
        if (enemyHitStopLastSeconds <= 0.0f)
        {
            // リセット
            enemyHitStopLastSeconds = 0.0f;
            enemyTimeScale = 1.0f;
            enemyHitStop = false;
        }

        if (playerHitStopLastSeconds <= 0.0f && enemyHitStopLastSeconds <= 0.0f)
        {
            hitStop = false;
        }
    }
}

// ヒットストップ開始処理
void HitStop::HitStopStart(float playerStopTime, float playerStopSpeed, float enemyStopTime, float enemyStopSpeed)
{
    if (!hitStop)
    {
        // ヒットストップ開始
        hitStop = true;

        // プレイヤーヒットストップ時間設定
        playerHitStopLastSeconds = playerStopTime;
        // プレイヤーヒットストップ進行設定
        playerTimeScale = playerStopSpeed;
        playerHitStop = true;

        // エネミーヒットストップ時間設定
        enemyHitStopLastSeconds = enemyStopTime;
        // エネミーヒットストップ進行設定
        enemyTimeScale = enemyStopSpeed;
        enemyHitStop = true;
    }
}

//// ヒットストップ開始処理
//void HitStop::HitStopStart(float stopTime, float stopSpeed)
//{
//    if (!hitStop)
//    {
//        // ヒットストップ開始
//        hitStop = true;
//        // ヒットストップの時間を設定
//        hitStopLastSeconds = stopTime;
//
//        timeScale = stopSpeed;
//    }
//}
