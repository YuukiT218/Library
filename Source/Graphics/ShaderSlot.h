#pragma once

#include <d3d11.h>

// シェーダーのレジスタ番号
// HLSL側の register(bN) / register(tN) と必ず合わせること
namespace ShaderSlot
{
	// 定数バッファ (bN)
	constexpr UINT SKELETON_CONSTANT_BUFFER = 6;	// CbSkeleton（Skinning.hlsli）
	constexpr UINT SCENE_CONSTANT_BUFFER = 7;		// CbScene（Scene.hlsli / PBR.hlsli）
	constexpr UINT RIM_LIGHT_CONSTANT_BUFFER = 8;	// CbRimLight（PBR.hlsli）

	// テクスチャ (tN)
	constexpr UINT DIFFUSE_IEM_TEXTURE = 33;		// 拡散反射用の環境マップ
	constexpr UINT SPECULAR_PMREM_TEXTURE = 34;		// 鏡面反射用の環境マップ
	constexpr UINT LUT_GGX_TEXTURE = 35;			// GGXの事前計算テーブル
	constexpr UINT NOISE_TEXTURE = 36;				// ディゾルブ用ノイズ
}
