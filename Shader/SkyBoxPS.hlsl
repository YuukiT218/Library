#include "SkyBox.hlsli"

TextureCube specularPmrem : register(t0);
SamplerState samplerState : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
	// 視線ベクトル
    float3 E = normalize(pin.worldPosition.xyz - cameraPosition.xyz);
	
	// mipmap使うと崩れるのでmipmapなしになるようにしておく
    return specularPmrem.SampleLevel(samplerState, E, 0) * pin.color;

}