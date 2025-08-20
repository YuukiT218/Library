#include "PostEffect.hlsli"
#include "../FullScreenQuad/FullScreenQuad.hlsli"
#define POINT 0
#define LINEAR 1
#define ANISOTROPIC 2
#define LINEAR_BORDER_BLACK 3
#define LINEAR_BORDER_WHITE 4
SamplerState sampler_states[5] : register(s0);

Texture2D hdr_color_buffer_texture : register(t0);

float4 main(VS_OUT pin) : SV_TARGET
{
    return hdr_color_buffer_texture.Sample(sampler_states[LINEAR], pin.texcoord, 0.0);
}