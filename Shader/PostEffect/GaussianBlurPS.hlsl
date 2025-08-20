#include "../FullScreenQuad/FullScreenQuad.hlsli"
#include "PostEffect.hlsli"
Texture2D colorMap : register(t0);
//Texture2D sceneMap : register(t1);
SamplerState linearSampler : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    //return float4(1, 1, 1, 1);
    float4 color = (float4) 0;
    color.a = 1;
    // 指定のカーネルサイズ分周囲から色を取得。CPU側で計算した重みを積和していく
    for (int i = 0; i < kernelSize * kernelSize; i++)
    {
        float2 offset = texcel * weights[i].xy;
        float weight = weights[i].z;
        color.rgb += colorMap.Sample(linearSampler, pin.texcoord + offset).rgb * weight;
    }
    return color;
}