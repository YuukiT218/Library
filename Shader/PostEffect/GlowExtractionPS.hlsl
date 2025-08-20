// BLOOM
#include "PostEffect.hlsli"
#include "../FullScreenQuad/FullScreenQuad.hlsli"

SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);
SamplerState AnisotropicSampler : register(s2);
SamplerState LinearBorderBlackSampler : register(s3);
SamplerState LinearBorderWhiteSampler : register(s4);

Texture2D hdr_color_buffer_texture : register(t0);
float4 main(VS_OUT pin) : SV_TARGET
{
    float4 sampled_color = hdr_color_buffer_texture.Sample(PointSampler, pin.texcoord);
#if 0
	return float4(step(threshold, dot(sampled_color.rgb, float3(0.299, 0.587, 0.114))) * sampled_color.rgb * effect_data.bloom_intensity, sampled_color.a);
#else	
    return float4(step(threshold, max(sampled_color.r, max(sampled_color.g, sampled_color.b))) * sampled_color.rgb, sampled_color.a);
#endif	
}
