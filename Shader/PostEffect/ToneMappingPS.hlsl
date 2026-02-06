#include "../FullScreenQuad/FullScreenQuad.hlsli"
#include "PostEffect.hlsli"
Texture2D baseSampler : register(t0);
Texture2D blurSampler : register(t1);
SamplerState linearSampler : register(s0);
//hlsl_tone_map
float4 main(VS_OUT pin) : SV_TARGET
{
    //float fExposureLevel = 32.0f;
    
    // 元の画像とブラー画像を取得
    float4 original = baseSampler.Sample(linearSampler, pin.texcoord);
    float4 blur = blurSampler.Sample(linearSampler, pin.texcoord);

    // ブラー画像との補間 (0.4で重み付け)
    float4 color = lerp(original, blur, 0.4f);
    
    float tc = pin.texcoord;

    // ビネット効果の適用
    tc -= 0.5f;
    float vignette = 1 - dot(pin.texcoord, tc);
    color *= vignette * vignette * vignette * vignette;

    // 露出レベルを適用
    color *= fExposureLevel;
    
	// トーンマップ
    color.rgb = color.rgb / (color.rgb + 1.0);

    // ガンマ補正を適用
    //return pow(color, 0.55f);
    return pow(color, 1.0f / 2.2f);
}
