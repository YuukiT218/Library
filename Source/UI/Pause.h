#pragma once

#include <memory>
#include <d3d11.h>
#include <string>

#include <DirectXMath.h>
#include <unordered_map>

#include "Sprite/Sprite.h"

struct SpriteData
{
	std::unique_ptr<Sprite> sprite;            // スプライト本体
	DirectX::XMFLOAT3 position{};              // 左上位置(dx, dy, dz)
	DirectX::XMFLOAT2 size{};                  // 幅と高さ(dw, dh)
	DirectX::XMFLOAT2 texPos{};                // テクスチャ切り抜き位置(sx, sy)
	DirectX::XMFLOAT2 texSize{};               // テクスチャ切り抜きサイズ(sw, sh)
	float angle = 0.0f;                        // 回転角
	DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f }; // 色 (r, g, b, a)
};

enum class SelectScene
{
	Game,
	Title,
	Count
};

enum class PulseState {
	Waiting,
	Expanding,
	Shrinking
};

class Pause
{
private:
	Pause() {};
	~Pause() {};
public:
	// 唯一のインスタンス取得
	static Pause& Instance()
	{
		static Pause pause;
		return pause;
	}
public:
	//初期化関数
	void Initialize();

	// Spriteの追加
	void AddSprite(const std::string& name, SpriteData data);

	SpriteData CreateSpriteData(
		ID3D11Device* device,
		const char* filepath,
		const DirectX::XMFLOAT3& pos,
		const DirectX::XMFLOAT2& size,
		const DirectX::XMFLOAT2& texPos,
		const DirectX::XMFLOAT2& texSize,
		float angle,
		const DirectX::XMFLOAT4& color);

	//更新関数
	void Update(float elapsedTime);

	//描画関数
	void Render(float elapsedTime, ID3D11DeviceContext* dc);

	bool GetIsPause() { return isPause; };

	// デバッグ用GUI描画
	void DrawDebugGUI();
public:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> sprite_vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> sprite_input_layout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> sprite_pixel_shader;
private:
	std::unordered_map<std::string, SpriteData> sprite;
	std::unordered_map<std::string, SpriteData> previousSpriteState;

	std::unordered_map<std::string, std::string> spriteChild;


	bool linkedSize = false;
	bool isController = true;
	bool isPause = false;

	DirectX::XMFLOAT2 baseScale = { 0,0 };

	SelectScene scene = SelectScene::Game;
private:
	PulseState pulseState = PulseState::Waiting;
	float pulseTimer = 0.0f;
	float pulseInterval = 0.5f;      // 鼓動の待ち時間（秒）
	float expandDuration = 0.06f;     // 膨らむ時間（秒）
	float shrinkDuration = 0.06f;     // 縮む時間（秒）
	float pulseStrength = 8.0f;     // 拡大量（ピクセル）
	float expandTimer = 0.0f;
	float shrinkTimer = 0.0f;
	void UpdatePulse(float elapsedTime);
};