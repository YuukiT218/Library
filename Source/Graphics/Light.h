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

	int AllocatePointLight();             // 空きを探して使えるインデックスを返す

private:
	DirectionalLight	directionalLight;
	PointLight pointLight[POINT_MAX];
};
