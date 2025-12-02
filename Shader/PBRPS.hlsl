#include "PBR.hlsli"
#include "ShadingFunctions.hlsli"
#include "ShadowMap.hlsli"

//通常描画に必要なTexture
Texture2D albedoMap : register(t0);
Texture2D normalMap : register(t1);
Texture2DArray shadowMap : register(t2);

//PBRに必要なTexture
Texture2D emissiveMap : register(t3);
Texture2D metallicMap : register(t4);
Texture2D occlusionMap : register(t5);

//カスケードシャドウマップ用テクスチャ
Texture2DArray cascadeShadowMap[4] : register(t6);

//ActorPBRで(t15)を使用

//IBL用テクスチャ
TextureCube diffuseiem : register(t33);
TextureCube specularpmrem : register(t34);
Texture2D lut_ggx : register(t35);


// テクスチャから値を比較するサンプラ
SamplerState linearSampler : register(s0);
SamplerState anisotropic : register(s1);
SamplerState shadowSampler : register(s2);

float4 main(VS_OUT pin, bool isFrontFace : SV_IsFrontFace) : SV_TARGET
{

    //	ガンマ係数
    static const float GammaFactor = 2.2f;
    
	//	ベースカラーを取得
    float4 base_color = materialColor;
    {
        float4 sampled = albedoMap.Sample(anisotropic, pin.texcoord);
        sampled.rgb = pow(sampled.rgb, GammaFactor);
        base_color *= sampled;
    }

	//	自己発光色を取得
    float3 emissive_color = emissiveColor;
    {
        float3 emissive = emissiveMap.Sample(anisotropic, pin.texcoord).rgb;
        emissive.rgb = pow(emissive.rgb, GammaFactor);
        float Factor = emissiveFactor * isEmissive;
        emissive_color.rgb *= emissive.rgb * adjustColor.rgb * Factor;
    }

	//	法線/従法線/接線
    float3 N = normalize(pin.normal.xyz);
    float3 T = normalize(pin.tangent.xyz);
    float sigma = 1.0;
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T) * sigma);

	//	法線マッピング
    {
        float4 sampled = normalMap.Sample(linearSampler, pin.texcoord);
        float3 normal_factor = sampled.xyz;
        normal_factor = (normal_factor * 2.0f) - 1.0f;
        normal_factor = normalize(normal_factor * float3(1.0f, 1.0f, 1.0f));
        N = normalize((normal_factor.x * T) + (normal_factor.y * B) + (normal_factor.z * N));
    }

	//金属質粗さを取得
    float roughness = roughnessFactor;
    float metalness = metalicFactor;
    if (metalicindex > -1)
    {
        float4 sampled = metallicMap.Sample(linearSampler, pin.texcoord);
        roughness *= sampled.g;
        metalness *= sampled.b;
    }
	
#if 01
    roughness = saturate(roughness + 0.001);
    metalness = saturate(metalness + 0.001);
    roughness = saturate(roughness + adjustRoughness);
    metalness = saturate(metalness + adjustMetalness);
#endif
	
	//	光の遮蔽値を取得
    float occlusion_factor = 1.0f;
    {
        float4 sampled = occlusionMap.Sample(linearSampler, pin.texcoord);
        occlusion_factor *= sampled.r;
    }
    const float occlusion_strength = occlusionStrength;

	//	(非金属部分)
    float4 albedo = base_color;

	//	入射光のうち拡散反射になる割合
    float3 diffuse_reflectance = lerp(albedo.rgb, 0.0f, metalness);

	//	垂直反射時のフレネル反射率(非金属でも最低4%は鏡面反射する)
    float3 F0 = lerp(0.04f, albedo.rgb, metalness);

	//	視線ベクトル
    float3 V = normalize(pin.position.xyz - cameraPosition.xyz);

#if 01  //  本来はデバッグ用の機能なのでいらない
    int DebugShadowMapIndex = -1;
#endif  //  defined(_DEBUG)

    //	直接光のシェーディング
    float3 total_diffuse = 0, total_specular = 0;
	{
	    // 平行光源の処理
	    {
            float3 diffuse = (float3) 0, specular = (float3) 0;
            float3 LightColor = lightColor.rgb * lightColor.a;
            float3 L = normalize(lightDirection.xyz);
            DirectBDRF(diffuse_reflectance, F0, N, V, L,
					   LightColor, roughness,diffuse, specular);
//            if (cascadeFlags.y > 0)
//            {
            
//                //	平行光源用カスケードシャドウマップ
//                for (int index = 0; index < ShadowBufferSize; ++index)
//                {
//		        // ライトから見たNDC座標を算出
//                    float4 wvpPos = mul(float4(pin.position.xyz, 1.0f), CascadeLightViewProjection[index]);

//                // NDC座標からUV座標を算出する
//                    wvpPos /= wvpPos.w;
//                    wvpPos.y = -wvpPos.y;
//                    wvpPos.xy = 0.5f * wvpPos.xy + 0.5f;

//		        // シャドウマップのUV範囲内か、深度値が範囲内か判定する
//                    if (wvpPos.z >= 0 && wvpPos.z <= 1 && wvpPos.x >= 0 && wvpPos.x <= 1 && wvpPos.y >= 0 && wvpPos.y <= 1)
//                    {
//						float3 shadowAtten = ShadowMapFetchPCF(cascadeShadowMap[index], shadowSampler, index, wvpPos.xyz,
//                                       shadowColor, shadowAttenuation, CascadeShadowBias[index], 3.0f);
                        
//                        diffuse *= shadowAtten;
//                        specular *= shadowAtten;
                        
//#if 01  //  本来はデバッグ用の機能なのでいらない
//                        DebugShadowMapIndex = index;
//#endif  //  defined(_DEBUG)
//                        break;
//                    }
//                }
//            }
//            else
            {
                float3 shadowAtten = ShadowMapFetchPCF(shadowMap, shadowSampler, 1, pin.shadow,
                                       shadowColor, shadowAttenuation, shadowBias, 3.0f);
                
                diffuse *= shadowAtten;
                specular *= shadowAtten;
            }

            total_diffuse += diffuse;
            total_specular += specular;
        }

        // 点光源
        for (int i = 0; i < POINT_MAX; ++i)
        {
            float4 point_light = pointLight[i];
            float4 point_color = pointColor[i];

            float3 L = pin.position.xyz - point_light.xyz;
            float len = length(L);
            if (len >= point_light.w)
                continue;
            float attenuateLength = saturate(1.0f - len / point_light.w);
            float attenuation = attenuateLength * attenuateLength;
            L /= len;
            float3 diffuse = (float3) 0, specular = (float3) 0;
            DirectBDRF(diffuse_reflectance, F0, N, V, L,
                       point_color.rgb * 100, roughness,
                       diffuse, specular);
            total_diffuse += diffuse * attenuation;
            total_specular += specular * attenuation;
        }
    }
    
    //IBL処理
    total_diffuse += DiffuseIBL(N, V, roughness, diffuse_reflectance, F0,
              diffuseiem, linearSampler) * IBLDiffuseScale;
    total_specular += SpecularIBL(N, V, roughness, F0, lut_ggx,
                    specularpmrem, linearSampler) * IBLSpecularScale;
    
	//	遮蔽処理
    total_diffuse = lerp(total_diffuse, total_diffuse * occlusion_factor, occlusion_strength);
    total_specular = lerp(total_specular, total_specular * occlusion_factor, occlusion_strength);
   
    //	色生成
    float3 color = total_diffuse + total_specular + emissive_color;
    color = pow(color, 1.0f / GammaFactor);

    //  カスケード表示
    if (cascadeFlags.x > 0)
    {
        if (DebugShadowMapIndex >= 0)
        {
            float col = rcp((float) (DebugShadowMapIndex / 3 + 1));
            float r = DebugShadowMapIndex % 3 == 0;
            float g = DebugShadowMapIndex % 3 == 1;
            float b = DebugShadowMapIndex % 3 == 2;
            color.rgb = float3(r, g, b) * col;
        }
        else
        {
            color.rgb = 0;
        }
    }

	// リムライト（安定版：pow の利点を残しつつ 0 でオフに）
    V = normalize(cameraPosition.xyz - pin.position.xyz);
    float rimBase = 1.0f - saturate(dot(N, V));
    float rimRange = pow(rimBase, rimPower); // 立ち上がりカーブ
    float rimFactor = rimRange * rimIntensity; // 明るさ
    color.rgb += rimColor * rimFactor;

    return float4(color, base_color.a);
}