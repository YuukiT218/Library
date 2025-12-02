//TrailRendererPS
#include "TrailRenderer.hlsli"

Texture2D Texture0 : register(t0);
Texture2D Dissolve : register(t1);

SamplerState Sampler0 : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = Texture0.Sample(Sampler0, pin.texcoord);
    //color.rgb *= pin.color.rgb;
    color.rgb *= 1.2;
    
    float2 uv = pin.texcoord;
    uv += Direction * Timer;
    float maskValue = Dissolve.Sample(Sampler0, uv).r;

    maskValue = smoothstep(pin.dissolve, pin.dissolve + 0.1, maskValue);
         // ディゾルブ効果で透明になる部分を青色に光らせる
    float edgeGlow = smoothstep(maskValue - 0.5, maskValue + 0.5, maskValue + 30); // エッジ部分の強調

    // エミッシブカラーを追加（青色の光）
    //float3 emissiveColor = float3(0.0f, 0.5f, 1.0f) * edgeGlow * 20.0f; // 青色の強度を設定
    //オブジェクトと同じ色
    float3 edgeColor = color.rgb * edgeGlow * 40.0f; // 強度を設定
    color.rgb += edgeColor * (1.0f - maskValue); // 透明になっていく部分だけに適用

    color *= maskValue;
    
    color *= pin.color;
    
   // color.a = 0;
    return color;
    //return Texture0.Sample(Sampler0, pin.texcoord) * pin.color;
}
