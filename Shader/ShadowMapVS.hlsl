#include "Skinning.hlsli"

cbuffer CbScene : register(b7)
{
    row_major float4x4 lightViewProjection;
};

float4 main(
    float4 position : POSITION,
    float4 boneWeights  : BONE_WEIGHTS,
    uint4  boneIndices  :BONE_INDICES
    // 今回はピクセルシェーダーを使用せず、頂点変換のみ行う
    ) : SV_POSITION 
{
    // ここで計算されたZ値が深度テクスチャに保存される
    position = SkinningPosition(position, boneWeights, boneIndices);
     
    return mul(float4(position.xyz, 1.0), lightViewProjection);
}