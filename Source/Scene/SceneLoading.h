#pragma once

#include "Sprite/Sprite.h"
#include "Scene.h"
#include <unordered_map>
#include <thread>
#include <string>

enum class MovePhase
{
	MoveIn,
	Wait,
	MoveOut,
	Done,
	FinishedAll
};


struct SpriteDataLoad
{
	std::unique_ptr<Sprite> sprite;            // スプライト本体
	DirectX::XMFLOAT3 position{};              // 左上位置(dx, dy, dz)
	DirectX::XMFLOAT2 size{};                  // 幅と高さ(dw, dh)
	DirectX::XMFLOAT2 texPos{};                // テクスチャ切り抜き位置(sx, sy)
	DirectX::XMFLOAT2 texSize{};               // テクスチャ切り抜きサイズ(sw, sh)
	float angle = 0.0f;                        // 回転角
	DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f }; // 色 (r, g, b, a)
};

// ローディングシーン
class SceneLoading : public Scene
{
public:
	SceneLoading(Scene* nextScene) : nextScene(nextScene) {}
	SceneLoading() {}
	~SceneLoading() /*override*/ {}

	// 初期化
	void Initialize() override;

	// 終了化
	void Finalize() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render(float elapsedTime) override;

private:
	// ローディングスレッド
	static void LoadingThread(SceneLoading* scene);

private:
	Scene* nextScene = nullptr;
	std::thread* thread = nullptr;
public:
	// Spriteの追加
	void AddSprite(const std::string& name, SpriteDataLoad data);

	SpriteDataLoad CreateSpriteData(
		ID3D11Device* device,
		const char* filepath,
		const DirectX::XMFLOAT3& pos,
		const DirectX::XMFLOAT2& size,
		const DirectX::XMFLOAT2& texPos,
		const DirectX::XMFLOAT2& texSize,
		float angle,
		const DirectX::XMFLOAT4& color);

	// デバッグ用GUI描画
	void DrawDebugGUI();

public:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> sprite_vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> sprite_input_layout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> sprite_pixel_shader;
private:
	std::unordered_map<std::string, SpriteDataLoad> sprite;
	std::unordered_map<std::string, SpriteDataLoad> previousSpriteState;

	std::unordered_map<std::string, std::string> spriteChild;

	bool linkedSize = false;
	bool isController = true;


	MovePhase hintPhase = MovePhase::MoveIn;
	MovePhase hint1Phase = MovePhase::Done; // Hint が終わったら MoveIn へ

	float hintTimer = 0.0f;
	float hint1Timer = 0.0f;

	DirectX::XMFLOAT3 hintStartPos{ 1249.0f,552.0f,0 };
	DirectX::XMFLOAT3 hintMidPos{ 0.0f,552.0f,0 };
	DirectX::XMFLOAT3 hintEndPos{ -560.0f,552.0f,0 };

	DirectX::XMFLOAT3 hint1StartPos{ 1249.0f,552.0f,0 };
	DirectX::XMFLOAT3 hint1MidPos{ 0.0f,552.0f,0 };
	DirectX::XMFLOAT3 hint1EndPos{ -560.0f,552.0f,0 };

	const float moveDuration = 1.0f;
	const float waitDuration = 3.0f;

	void MoveHintText(float elapsedTime);
};