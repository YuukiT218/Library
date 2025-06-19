#pragma once

#include <vector>
#include "Stage.h"
//#include "Graphics/ShadowMap.h"

// ステージマネージャー
class StageManager
{
private:
	StageManager() {}
	~StageManager() {}

public:
	// 唯一のインスタンス取得
	static StageManager& Instance()
	{
		static StageManager instance;
		return instance;
	}

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const RenderContext& rc, ShaderId shaderId);

	//ステージモデルデバック確認
	void Debug(const RenderContext& rc);

	//ステージ内定数のデバッグ
	void DebugImGui();

	////影描画
	//void ShadowRender(const RenderContext& rc, ShadowMap* shadowMap);

	////カスケードシャドウマップ用モデル設定
	//void SetShadowModel(ShadowMap* shadowMap);

	// ステージ登録
	void Register(Stage* stage);

	// ステージ全削除
	void Clear();

	// レイキャスト
	bool RayCast(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& end, HitResult& hit);

	bool IsDivisionEnabled() const { return isDivision; }
	void SetDivisionEnabled(bool enabled) { isDivision = enabled; }

private:
	std::vector<Stage*> stages;

	bool isDivision = true;
	bool isDebugRender = false;
};