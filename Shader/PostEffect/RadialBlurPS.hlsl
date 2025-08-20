#include "../FullScreenQuad/FullScreenQuad.hlsli"
#include "PostEffect.hlsli"

SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);
SamplerState AnisotropicSampler : register(s2);
SamplerState LinearBorderBlackSampler : register(s3);
SamplerState LinearBorderWhiteSampler : register(s4);

Texture2D colorMap : register(t0);


float4 main(VS_OUT pin) : SV_TARGET
{
    float2 scene_map_size;
    colorMap.GetDimensions(scene_map_size.x, scene_map_size.y);
 
    float4 color = colorMap.Sample(LinearSampler, pin.texcoord);
    float4 result_color = color;
 
    float2 blur_vector = (radialDatas.center - pin.texcoord);
    blur_vector *= (radialDatas.radius / scene_map_size.xy) / radialDatas.samplingCount;
    for (int index = 1; index < radialDatas.samplingCount; ++index)
    {
        result_color += colorMap.Sample(LinearSampler, pin.texcoord + blur_vector * index);
    }
 
    float mask_radius = radialDatas.maskRadius / min(scene_map_size.x, scene_map_size.y);
    float mask_value = saturate(length(pin.texcoord - radialDatas.center) / mask_radius);
    return lerp(color, result_color / radialDatas.samplingCount, mask_value);
}