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
	D3D11_TEXTURE2D_DESC maskTexture2dDesc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> maskTexture;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePixelShader;

private:
	std::unique_ptr<Sprite> titleBack;
	std::unique_ptr<Sprite> titleName;
	std::unique_ptr<Sprite> anyButton;

	std::unique_ptr<Sprite> endPause;
	std::unique_ptr<Sprite> endKey;
	std::unique_ptr<Sprite> endCon;
	std::unique_ptr<Sprite> endYes;
	std::unique_ptr<Sprite> endNo;
	std::unique_ptr<Sprite> endSelect;

	AudioSource* bgm = nullptr;

	EasingData easingData;

	DirectX::XMFLOAT2 pos = { 0.0f, 360.0f };

	float amount = 0;

	float nameAlpha = 0.f;
	float anyAlpha = 0.f;

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
	DirectX::XMFLOAT3 soScale{ 882,1664,0 };
};