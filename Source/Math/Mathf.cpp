#include <stdlib.h>
#include "Mathf.h"

namespace
{
	// 8bitカラーの最大値
	constexpr float COLOR_8BIT_MAX = 255.0f;
}

float Mathf::Lerp(float a, float b, float t)
{
	return a * (1.0f - t) + (b * t);
}

// 指定範囲のランダム値を計算する
float Mathf::RandomRange(float min, float max)
{
	// 0.0～1.0の間までのランダム値
	float value = static_cast<float>(rand()) / RAND_MAX;

	// min～maxまでのランダム値に変換
	return min + (max - min) * value;
}

//0~255のカラーの値を0~1に変換する処理
DirectX::XMFLOAT4 Mathf::Color255ToNormalized(const DirectX::XMFLOAT4& color)
{
	return DirectX::XMFLOAT4
	{
		color.x / COLOR_8BIT_MAX,
		color.y / COLOR_8BIT_MAX,
		color.z / COLOR_8BIT_MAX,
		color.w / COLOR_8BIT_MAX
	};
}