#include "../FullScreenQuad/FullScreenQuad.hlsli"
#include "PostEffect.hlsli"

#define MAX_CHROMATIC_SAMPLES 16 
SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);
SamplerState AnisotropicSampler : register(s2);
SamplerState LinearBorderBlackSampler : register(s3);
SamplerState LinearBorderWhiteSampler : register(s4);

Texture2D colorMap : register(t0);

float4 main(VS_OUT pin) : SV_TARGET
{
    //float2 scene_map_size;
    //colorMap.GetDimensions(scene_map_size.x, scene_map_size.y);
    //float2 scene_map_texel_size = (float2) 1.0f / scene_map_size.xy;
    //float2 coords = 2.0f * pin.texcoord - 1.0f;
    //float2 end = pin.texcoord - coords * dot(coords, coords) * chromatic_aberration;
 
    //float2 diff = end - pin.texcoord;
    //int samples = clamp(int(length(scene_map_texel_size * diff / 2.0f)), 3, MAX_CHROMATIC_SAMPLES);
    //float2 delta = diff / samples;
    //float2 pos = pin.texcoord;
    //float4 sum = (0.0f).xxxx, filterSum = (0.0f).xxxx;
    //for (int index = 0; index < MAX_CHROMATIC_SAMPLES; ++index)
    //{
    //    int t = (int) (index + 0.5f) / samples;
    //    float4 s = colorMap.Sample(LinearSampler, pos);
    //    float3 rgb = chromaticAberrationShift[t % 3];
    //    float4 filter = float4(rgb, 1.0f);
 
    //    sum += s * filter;
    //    filterSum += filter;
    //    pos += delta;
    //}
 
    //return sum / filterSum;

    //float _chromatic_aberration = 0.02;
    // 中心からの差を考慮したズレ
      // 中心からの方向ベクトル
    float2 offsetDir = 0.5 - pin.texcoord;
    
    // 距離に応じた非線形ズレ（歪み効果）
    float dist = length(offsetDir);
    float2 v = normalize(offsetDir) * chromatic_aberration * dist;

    // 各チャンネルをそれぞれずらしてサンプリング
    float r = colorMap.Sample(LinearSampler, pin.texcoord - v).r;
    float g = colorMap.Sample(LinearSampler, pin.texcoord).g;
    float b = colorMap.Sample(LinearSampler, pin.texcoord + v).b;

    float2 sceneMapSize;
    colorMap.GetDimensions(sceneMapSize.x, sceneMapSize.y);
    float4 color = colorMap.Sample(LinearSampler, pin.texcoord);
    
    float mask_radius = chromaticMask / min(sceneMapSize.x, sceneMapSize.y);
    float mask_value = saturate(length(pin.texcoord - radialDatas.center) / mask_radius);
    
    return float4(lerp(color, float4(r, g, b, 1.0), mask_value));
   

    //float2 uv = pin.texcoord;
    //float2 center = float2(0.5, 0.5);
    //float2 toCenter = uv - center;
    //float dist = length(toCenter); // 中心からの距離

    //// 色のずれ量（中心から離れるほど大きくなる）
    //float aberrationAmount = chromatic_aberration * pow(dist, 1.5); // powで非線形に調整

    //// RGBそれぞれずらす（微妙にずれ方向を変えても良い）
    //float2 offsetR = uv + toCenter * aberrationAmount;
    //float2 offsetG = uv;
    //float2 offsetB = uv - toCenter * aberrationAmount;

    //// サンプリング
    //float3 color;
    //color.r = colorMap.Sample(LinearSampler, offsetR).r;
    //color.g = colorMap.Sample(LinearSampler, offsetG).g;
    //color.b = colorMap.Sample(LinearSampler, offsetB).b;

    //return float4(color, 1.0);
}