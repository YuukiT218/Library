#pragma once

#include <DirectXMath.h>

struct DirectionalLight
{
	DirectX::XMFLOAT4	direction = { 0, -1, 0, 0 };
	DirectX::XMFLOAT4	color = { 1, 1, 1, 1 };
};

#define POINT_MAX (64)
struct PointLight
{
	int               index;
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 color;
};

class LightManager
{
public:
	static LightManager& Instance()
	{
		static LightManager instance;
		return instance;
	}

	// ディレクショナルライト設定
	void SetDirectionalLight(DirectionalLight& light) { directionalLight = light; }

	// ディレクショナルライト取得
	const DirectionalLight& GetDirectionalLight() const { return directionalLight; }

	// ポイントライト設定
	void SetPointLight(PointLight& light, int index) { pointLight[index] = light; }

	// ポイントライト取得
	const PointLight& GetPointLight(int index) const { return pointLight[index]; }

	// 空きを探してポイントライトを1つ確保する（空きが無ければ -1）
	int AllocatePointLight()
	{
		for (int i = 0; i < POINT_MAX; ++i)
		{
			if (pointLightUsed[i]) continue;

			pointLightUsed[i] = true;
			pointLight[i] = PointLight{};
			pointLight[i].index = i;
			return i;
		}
		return -1;
	}

	// 確保したポイントライトを解放する
	void FreePointLight(int index)
	{
		if (index < 0 || index >= POINT_MAX) return;

		pointLightUsed[index] = false;
		// 範囲を0に戻してシェーダー側で無視されるようにする
		pointLight[index] = PointLight{};
	}

	// 確保済みのポイントライトの位置・範囲・色を更新する
	void UpdatePointLight(int index, const DirectX::XMFLOAT3& position,
		float range, const DirectX::XMFLOAT4& color)
	{
		if (index < 0 || index >= POINT_MAX) return;

		pointLight[index].index = index;
		pointLight[index].position = { position.x, position.y, position.z, range };
		pointLight[index].color = color;
	}

private:
	DirectionalLight	directionalLight;

	// 未使用のライトは範囲0で無効になるよう、必ずゼロ初期化しておく
	PointLight pointLight[POINT_MAX]{};
	bool pointLightUsed[POINT_MAX]{};
};
