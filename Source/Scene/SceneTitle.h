#pragma once

#include "Scene.h"
#include "Sprite/Sprite.h"
#include "System/Audio/AudioSource.h"

// タイトルシーン
class SceneTitle : public Scene
{
public:
	SceneTitle() {}
	~SceneTitle() override {}

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

	struct EasingData
	{
		bool startFlag = false;
		float resetTime = 0.0f;

		int mode = 0;

		float time = 0.0f;
		float totalTime = 60.0f;
		float maxValue = 1000.0f;
		float minValue = 0.0f;
	};

	enum EasingMode
	{
		OutBounce,
		OutSine,
	};

public:
	D3D11_TEXTURE2D_DESC mask_texture2dDesc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mask_texture;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> sprite_vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> sprite_input_layout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> sprite_pixel_shader;

private:
	std::unique_ptr<Sprite> TitleBack;
	std::unique_ptr<Sprite> TitleName;
	std::unique_ptr<Sprite> AnyButton;

	std::unique_ptr<Sprite> Sword;

	std::unique_ptr<Sprite> EndPause;
	std::unique_ptr<Sprite> EndKey;
	std::unique_ptr<Sprite> EndCon;
	std::unique_ptr<Sprite> EndYes;
	std::unique_ptr<Sprite> EndNo;
	std::unique_ptr<Sprite> EndSele;

	AudioSource* BGM = nullptr;

	EasingData easingData;

	DirectX::XMFLOAT2 pos = { 0.0f, 360.0f };

	float amount = 0;

	float NameAlpha = 0.f;
	float AnyAlpha = 0.f;

	float alphaTime = 0.f;
	float alphaSpeed = 5.0f;

	float timer = 0;

	float keyAlpha = 0.0f;
	float conAlpha = 0.0f;

	float pauseAlpha = 0.0f;

	bool isPause = false;
	bool isYesSelected = false;

	DirectX::XMFLOAT3 sePos{ 0,0,0 };
	DirectX::XMFLOAT3 soPos{ 0,0,0 };
	DirectX::XMFLOAT3 soscale{ 882,1664,0 };
};