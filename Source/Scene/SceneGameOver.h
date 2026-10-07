#pragma once

#include "Scene.h"
#include "Sprite/Sprite.h"
#include <memory>

// ゲームオーバーシーン
class SceneGameOver : public Scene
{
public:
	SceneGameOver() {}
	~SceneGameOver() override {}

	// 初期化
	void Initialize() override;

	// 終了化
	void Finalize() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render(float elapsedTime) override;

	// デバッグ用GUI描画
	void DrawDebugGUI();

private:
	std::unique_ptr<Sprite> backSprite;       // 背景
	std::unique_ptr<Sprite> retrySprite;      // リトライ（未選択）
	std::unique_ptr<Sprite> retrySelectedSprite;     // リトライ（選択中）
	std::unique_ptr<Sprite> backTitleSprite;  // タイトルへ（未選択）
	std::unique_ptr<Sprite> backTitleSelectedSprite; // タイトルへ（選択中）

	// trueならリトライ選択中、falseならタイトルへ戻る選択中
	bool isRetrySelected = true;

	// 誤操作防止用のタイマーなどが必要であれば使用
	float timer = 0.0f;
};