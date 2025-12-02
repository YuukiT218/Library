#pragma once

#include <memory>
#include <d3d11.h>
#include <string>

#include <DirectXMath.h>
#include <unordered_map>

#include "Sprite/Sprite.h"
#include "Character/Enemy/Enemy.h"

class BattleUI
{
private:
	BattleUI() {};
	~BattleUI() {};
public:
	// 唯一のインスタンス取得
	static BattleUI& Instance()
	{
		static BattleUI BattleUI;
		return BattleUI;
	}

	//初期化関数
	void Initialize(float maxhealth, int maxAg);

	//Sprite読み込み
	void InitializeSprites();

	//UIのパラメータ初期化
	void UpdateUIData(float elapsedTime);

	//更新関数
	void Update(float elapsedTime, float currenthealth, float currentag);

	//描画関数
	void Render(float elapsedTime);

	// デバッグ用GUI描画
	void DrawDebugGUI();
public:
	//D3D11_TEXTURE2D_DESC mask_texture2dDesc;
	//Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mask_texture;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> sprite_vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> sprite_input_layout;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> sprite_pixel_shader;

private:
	struct  HPUIData
	{
		DirectX::XMFLOAT3 position{};
		DirectX::XMFLOAT2 scale{};
		DirectX::XMFLOAT3 angle{};

		float amount = 0;
	};

	struct SpriteUIData
	{
		DirectX::XMFLOAT3 position{};
		float width = 100;
		float height = 100;
		DirectX::XMFLOAT4 size{ 0.0f, 0.0f, 100.0f, 100.0f };
		float angle = 0;
		DirectX::XMFLOAT4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
	};

public:

	//最大HPをセット
	void SetMaxHealth(float playerMaxHealth) { maxHealth = playerMaxHealth; }

	//最大AGをセット
	void SetMaxAg(float playerMaxAg) { maxAg = playerMaxAg; }

	// ワールド座標からスクリーン座標へ変換する関数
	DirectX::XMFLOAT2 ConvertWorldToScreen(
		const DirectX::XMFLOAT3& worldPosition,
		const DirectX::XMMATRIX& view,
		const DirectX::XMMATRIX& projection,
		float screenWidth,
		float screenHeight);

	// UIの位置を更新する関数
	void UpdateUIPosition(
		const std::string& key,
		const DirectX::XMFLOAT3& worldPosition,
		std::unordered_map<std::string, HPUIData>& uiMap,
		const DirectX::XMMATRIX& view,
		const DirectX::XMMATRIX& projection,
		float screenWidth,
		float screenHeight);

	void SetRockOnEnemy(Enemy* enemy) { lockonEnemy = enemy; }

	// スプライトの初期設定関数
	void SpriteSetUp(std::string name, DirectX::XMFLOAT3 position, float width, float height, DirectX::XMFLOAT4 size, float angle, DirectX::XMFLOAT4 color);

private:
	float maxHealth = 0;
	float currentHealth = 0;
	int maxAg = 0;
	float currentAg = 0;

	//オーバーリミットの現在量。プロトでは使わない
	float currentOverLimit = 1.0f;

	std::unique_ptr<Sprite> lockonpoint;

	float lockonSinOffset = 0;
	float lockonSinSpeed = 4;
	float lockonSinTimer = 0;
	float lockonAngle = 0;

	std::unique_ptr<Sprite> set1;
	std::unique_ptr<Sprite> set2;
	Enemy* lockonEnemy = nullptr;

	std::unordered_map<std::string, HPUIData> uiMap;
	std::unordered_map<std::string, SpriteUIData> spriteUI;

	float offsetPositionX = 100;
	float offsetPositionY = 30;

	float offsetAgPositionX = -105;
	float offsetAgPositionY = 35;

	float currentRedHealth = 1;

	DirectX::XMFLOAT2 scale{ 40,40 };
};