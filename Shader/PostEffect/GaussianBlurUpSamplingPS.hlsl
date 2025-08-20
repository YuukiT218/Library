//GaussianBlurUpsamplingPS.hlsl
#include "../FullScreenQuad/FullScreenQuad.hlsli"
#include "PostEffect.hlsli"
#define POINT 0
#define LINEAR 1
#define ANISOTROPIC 2
#define LINEAR_BORDER_BLACK 3
#define LINEAR_BORDER_WHITE 4
SamplerState sampler_states[5] : register(s0);

static const uint downsampled_count = 6;
Texture2D downsampled_textures[downsampled_count] : register(t0);

//ダウンサンプリングされた複数のテクスチャを合成して輝度を強調
float4 main(VS_OUT pin) : SV_TARGET
{
    float3 sampled_color = 0;
    [unroll]
    for (uint downsampled_index = 0; downsampled_index < downsampled_count; ++downsampled_index)
    {
        sampled_color += downsampled_textures[downsampled_index].Sample(sampler_states[LINEAR], pin.texcoord).xyz;
    }
    return float4(sampled_color * bloomIntensity, 1);
}