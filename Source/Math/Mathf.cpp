#include <stdlib.h>
#include "Mathf.h"

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
DirectX::XMFLOAT4& Mathf::Color255ToNormalized(const DirectX::XMFLOAT4& color)
{
	return DirectX::XMFLOAT4
	{
		color.x / 255.0f,
		color.y / 255.0f,
		color.z / 255.0f,
		color.w / 255.0f
	};
}