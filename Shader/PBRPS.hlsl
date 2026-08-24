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

//ディゾルブ用ノイズテクスチャ
Texture2D noiseTexture : register(t36); // ノイズテクスチャ

// テクスチャから値を比較するサンプラ
SamplerState linearSampler : register(s0);
SamplerState anisotropic : register(s1);
SamplerState shadowSampler : register(s2);

float4 main(VS_OUT pin, bool isFrontFace : SV_IsFrontFace) : SV_TARGET
{
    // 障害物の透過処理
    if (targetPosition.w > 0.5 && enableDither > 0.5)
    {
        float3 camPos = cameraPosition.xyz;
        float3 playerPos = targetPosition.xyz;
        float3 pixelPos = pin.position.xyz;

        // カメラ -> プレイヤー のベクトル
        float3 vecCamToPlayer = playerPos - camPos;
        float distCamToPlayer = length(vecCamToPlayer);
        
        // カメラ -> 現在のピクセル のベクトル
        float3 vecCamToPixel = pixelPos - camPos;
        float distCamToPixel = length(vecCamToPixel);

        // 「プレイヤーより手前」かつ「カメラに十分近い」場合のみ判定
        // (-0.5はプレイヤー自身の描画が消えないようにするためのオフセット)
        if (distCamToPixel < distCamToPlayer - 0.5f)
        {
            float3 rayDir = normalize(vecCamToPlayer);
            
            // ピクセルが視線（レイ）からどれくらい離れているか（円筒判定）
            float3 crossProd = cross(rayDir, vecCamToPixel);
            float distFromRay = length(crossProd);

            // 透過させる半径（0.5メートル以内なら透けさせる）
            float clipRadius = 1.25f;

            if (distFromRay < clipRadius)
            {
                // ノイズテクスチャを使ったディザリング
                // スクリーン座標ベースでサンプリングするとカメラが動いてもノイズがちらつきにくい
                float2 noiseUV = pin.vertex.xy / 100.0f; // スケーリングは適宜調整
                float noise = noiseTexture.Sample(linearSampler, noiseUV).r;

                // 中心に近いほど消える確率を上げる（透明度を上げる）
                // 外側(clipRadius)にいくほど不透明(1.0)になる
                float threshold = distFromRay / clipRadius;
                
                // ノイズ値が閾値より大きければピクセルを破棄
                // (noise: 0~1, threshold: 0~1)
                // 中心付近(threshold=0) -> noise > 0 はほぼ成立 -> ほぼ消える
                if (noise > threshold)
                {
                    discard;
                }
            }
        }
    }

    //	ガンマ係数
    static const float GammaFactor = 2.2f;
    
	//	ベースカラーを取得
    float4 base_color = materialColor;
    {
        float4 sampled = albedoMap.Sample(anisotropic, pin.texcoord);
        sampled.rgb = pow(sampled.rgb, GammaFactor);
        base_color *= sampled;
    }

    // テレポートエフェクト: Dissolve
    if (enableDissolve > 0.5 && teleportProgress > 0.0)
    {
        float2 noiseUV = pin.texcoord * 2.0 + float2(teleportTime * 0.1, teleportTime * 0.05);
        float noise = noiseTexture.Sample(linearSampler, noiseUV).r;
        float noise2 = noiseTexture.Sample(linearSampler, noiseUV * 2.13 + 0.37).r;
        noise = noise * 0.7 + noise2 * 0.3;
        
        float dissolveThreshold = teleportProgress;
        float dissolveEdge = dissolveThreshold - dissolveEdgeWidth;
        
        if (noise < dissolveThreshold)
        {
            if (noise > dissolveEdge)
            {
                float edgeFactor = (noise - dissolveEdge) / dissolveEdgeWidth;
                float3 edgeGlow = dissolveEdgeColor * (1.0 - edgeFactor);
                float pulse = sin(teleportTime * 10.0) * 0.5 + 0.5;
                edgeGlow *= (1.5 + pulse * 0.5);
                base_color.rgb += edgeGlow;
                base_color.a *= edgeFactor;
            }
            else
            {
                discard;
            }
        }
    }

	//	自己発光色を取得
    float3 emissive_color = emissiveColor.rgb;
    {
        //  エミッシブテクスチャを持たないマテリアルでは
        //  白(1,1,1)として扱う。こうしないとサンプル結果の0が掛かり、
        //  マテリアルに発光色があっても光らなくなる
        float3 emissive = (float3) 1.0f;
        if (hasEmissiveTexture > 0.5f)
        {
            emissive = emissiveMap.Sample(anisotropic, pin.texcoord).rgb;
            emissive = pow(emissive, GammaFactor);
        }

        emissive_color.rgb *= emissive * adjustColor.rgb * emissiveFactor * isEmissive;
    }

	//	法線/従法線/接線
    float3 N = normalize(pin.normal.xyz);
    //  影のバイアス計算には法線マップ適用前の面の向きを使う
    float3 geometricNormal = N;
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
    //  平行光源による影の減衰（間接光にも使う）
    float3 shadowAtten = (float3) 1.0f;

    float3 total_diffuse = 0, total_specular = 0;
	{
	    // 平行光源の処理
	    {
            float3 diffuse = (float3) 0, specular = (float3) 0;
            float3 LightColor = lightColor.rgb * lightColor.a;
            float3 L = normalize(lightDirection.xyz);
            DirectBDRF(diffuse_reflectance, F0, N, V, L,
					   LightColor, roughness,diffuse, specular);
            {
                //  ワールド座標から所属するカスケードを求めて影を引く
                //  面が光に対して寝ているほど自己遮蔽しやすいため
                //  シャドウマップを引く位置を法線方向へ逃がす
                //  まず素の位置で所属カスケードを決める
                float3 shadowCoord;
                //  cascadeFlags.y = 実際に使う段数。1なら従来の1枚のシャドウマップ
                int cascadeIndex = SelectShadowCascade(pin.position.xyz, (int) cascadeFlags.y,
                                       CascadeLightViewProjection, shadowCoord);

                //  どのカスケードにも入らない遠景には影を落とさない
                if (cascadeIndex >= 0)
                {
                    //  面が光に対して寝ているほど自己遮蔽しやすいので、
                    //  シャドウマップを引く位置を法線方向へ逃がす
                    //  ずらす量はそのカスケードのテクセル基準にする
                    //  (ワールド固定量だと段によってテクセル数百個分ずれ、
                    //   曲面の多いキャラクターで影の縁がとげ状になる)
                    float NdotL = saturate(dot(geometricNormal, L));
                    float slopeScale = min(sqrt(1.0f - NdotL * NdotL) / max(NdotL, 0.2f), 2.0f);
                    float texelWorldSize = GetCascadeTexelWorldSize(cascadeTexelWorldSize, cascadeIndex);

                    float3 offsetPosition = pin.position.xyz
                         + geometricNormal * texelWorldSize * shadowNormalOffset * (1.0f + slopeScale);

                    shadowCoord = ComputeShadowCoord(offsetPosition, CascadeLightViewProjection[cascadeIndex]);

                    shadowAtten = ShadowMapFetchPCF(shadowMap, shadowSampler, cascadeIndex, shadowCoord,
                                       shadowColor, shadowAttenuation, shadowBias, 3.0f);

                    DebugShadowMapIndex = cascadeIndex;
                }

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
    
    //IBL間接光
    float3 indirect_diffuse = DiffuseIBL(N, V, roughness, diffuse_reflectance, F0,
              diffuseiem, linearSampler) * IBLDiffuseScale;
    float3 indirect_specular = SpecularIBL(N, V, roughness, F0, lut_ggx,
                    specularpmrem, linearSampler) * IBLSpecularScale;

    //  影の中では間接光も落とす
    //  ここを落とさないと、影の中でも法線マップの陰影が満額残り、
    //  落ち影が起伏に埋もれて見えなくなる
    float3 indirectShadow = lerp((float3) 1.0f, shadowAtten, indirectShadowStrength);
    indirect_diffuse *= indirectShadow;
    indirect_specular *= indirectShadow;

    //  遮蔽は間接光にのみ適用する
    indirect_diffuse = lerp(indirect_diffuse, indirect_diffuse * occlusion_factor, occlusion_strength);
    indirect_specular = lerp(indirect_specular, indirect_specular * occlusion_factor, occlusion_strength);

    total_diffuse += indirect_diffuse;
    total_specular += indirect_specular;
   
    //	色生成
    float3 color = total_diffuse + total_specular + emissive_color;
    color = pow(color, 1.0f / GammaFactor);

    //  カスケード表示
    if (cascadeFlags.x > 0)
    {
        if (DebugShadowMapIndex >= 0)
        {
            color.rgb = GetCascadeDebugColor(DebugShadowMapIndex);
        }
        else
        {
            color.rgb = 0;
        }
    }

	// リムライト
    V = normalize(cameraPosition.xyz - pin.position.xyz);
    float rimBase = 1.0f - saturate(dot(N, V));
    float rimRange = pow(rimBase, rimPower); // 立ち上がりカーブ
    float rimFactor = rimRange * rimIntensity; // 明るさ
    color.rgb += rimColor.rgb * rimFactor;

    if (afterimageAlpha < 0.999f)
    {
        //  残像はライティングせず、素の色に色味を掛けて光らせる
        //  暗くして消すのではなくアルファだけで消すことで、
        //  グレーのまま残らずに透けながら消えていく
        color.rgb = base_color.rgb * afterimageColor.rgb * afterimageColor.a;
    }

    float4 finalColor = float4(color, base_color.a);

    if (afterimageAlpha < 1.0)
    {
        finalColor.a *= afterimageAlpha;
    }

    return finalColor;
}