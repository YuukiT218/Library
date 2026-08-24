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
     
    //  スキニング結果の w は「重みの合計」になる。
    //  ここで w=1 に潰すと、重みの合計が1でない頂点が
    //  ワールド原点方向へ引き寄せられ、影だけが伸びた形になる。
    //  通常描画(PBRVS)と同じく w を保持し、同次座標の除算に任せる。
    return mul(position, lightViewProjection);
}