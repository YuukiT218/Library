#pragma once

#include <string>
#include <thread>
#include <unordered_map>

#include "Scene.h"
#include "Sprite/Sprite.h"

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
	std::unique_ptr<Sprite> sprite;
	DirectX::XMFLOAT3 position{};
	DirectX::XMFLOAT2 size{};
	DirectX::XMFLOAT2 texPos{};
	DirectX::XMFLOAT2 texSize{};
	float angle = 0.0f;
	DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
};

// ローディングシーン
class SceneLoading : public Scene
{
public:
	explicit SceneLoading(Scene* nextScene)
		: nextScene(nextScene)
	{
	}

	SceneLoading() = default;

	~SceneLoading() override
	{
		// SceneManager の保留中にアプリ終了しても、スレッドと遷移先を回収する。
		Finalize();
		delete nextScene;
		nextScene = nullptr;
	}

	void Initialize() override;
	void Finalize() override;
	void Update(float elapsedTime) override;
	void Render(float elapsedTime) override;

private:
	static void LoadingThread(SceneLoading* scene);

private:
	Scene* nextScene = nullptr;
	std::thread* thread = nullptr;

public:
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

	void DrawDebugGUI();

public:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> spriteVertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> spriteInputLayout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> spritePixelShader;

private:
	std::unordered_map<std::string, SpriteDataLoad> sprite;
	std::unordered_map<std::string, SpriteDataLoad> previousSpriteState;
	std::unordered_map<std::string, std::string> spriteChild;

	bool linkedSize = false;
	bool isController = true;

	MovePhase hintPhase = MovePhase::MoveIn;
	MovePhase hint1Phase = MovePhase::Done;

	float hintTimer = 0.0f;
	float hint1Timer = 0.0f;

	DirectX::XMFLOAT3 hintStartPos{ 1249.0f, 552.0f, 0.0f };
	DirectX::XMFLOAT3 hintMidPos{ 0.0f, 552.0f, 0.0f };
	DirectX::XMFLOAT3 hintEndPos{ -560.0f, 552.0f, 0.0f };

	DirectX::XMFLOAT3 hint1StartPos{ 1249.0f, 552.0f, 0.0f };
	DirectX::XMFLOAT3 hint1MidPos{ 0.0f, 552.0f, 0.0f };
	DirectX::XMFLOAT3 hint1EndPos{ -560.0f, 552.0f, 0.0f };

	// ヒント文が画面に出入りする時間と、表示し続ける時間
	static constexpr float HINT_MOVE_DURATION = 1.0f;
	static constexpr float HINT_WAIT_DURATION = 3.0f;

	void MoveHintText(float elapsedTime);
};