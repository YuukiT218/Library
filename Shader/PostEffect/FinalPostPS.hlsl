// BLOOM
#include "../FullScreenQuad/FullScreenQuad.hlsli"
#include "PostEffect.hlsli"
#include "PostShadingFunction.hlsli"

SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);
SamplerState AnisotropicSampler : register(s2);
SamplerState LinearBorderBlackSampler : register(s3);
SamplerState LinearBorderWhiteSampler : register(s4);

Texture2D colorMap : register(t0);
Texture2D luminanceMap : register(t1);


// コントラスト(明暗の差)
float3 Contrast(float3 color)
{
    //float _contrast = 1.3;
    color -= 0.5; //中央からどれだけ暗い明るい？
    color *= contrast; //中央からの明暗をcontrast倍
    color += 0.5;
    return color;
}

// 彩度
float3 Saturation(float3 color)
{
    //float _saturation = 0.5;
    float gray = 0.299 * color.r 
                + 0.587 * color.g
                + 0.114 * color.b;
    color -= gray; //グレーからの±
    color *= saturation;
    color += gray;
    return color;
}

//カラーフィルター
float3 ColorFilter(float3 color)
{
    //float3 _ColorFilter = { 1.05, 0.9, 0.9 };
    color *= colorFilter;
    return color;
}

float3 ChromaticAberration(float2 uv)
{
    //float _chromatic_aberration = 0.02;
    // 中心からの差を考慮したズレ
    float2 v = (0.5 - uv) * chromatic_aberration; //ずらす方向
    float r = colorMap.Sample(LinearSampler, uv).r;
    float g = colorMap.Sample(LinearSampler, uv + v).g;
    float b = colorMap.Sample(LinearSampler, uv + v * 2.0).b;
    
    return float3(r, g, b);
}


float3 reinhard_tone_mapping(float3 color)
{
    float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    float tone_mapped_luma = luma / (1. + luma);
    color *= tone_mapped_luma / luma;
    return color;
}

float4 hlsl_tone_map(float4 original, float4 blur, float2 tc)
{
    float fExposureLevel = 1.0f;
    float4 color = lerp(original, blur, 0.4f);
    tc -= 0.5f; // Put coords in -1 / 2 to 1 /2 range
    
    // Square of distance from origin (center of screen)
    float vignette = 1 - dot(tc, tc);
    // Multiply by vignette to the fourth
    color = color * vignette * vignette * vignette * vignette;
    color *= fExposureLevel; // Apply simple exposure level
    return pow(color, 0.55f); // Apply gamma and return
}

float3 filmic_tone_mapping(float3 color)
{
    color = max(float3(0.0, 0.0, 0.0), color - 0.004); // 黒レベル補正
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06); // 曲線適用
    return pow(color, 1.0 / 2.2); // ガンマ補正
}

float3 ACESFilmToneMapping(float3 color)
{
    return saturate((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14));
}

// セピア
float3 Sepia(float3 color, float amount)
{
    color.r = dot(color, float3(1.0 - 0.607 * amount, 0.769 * amount, 0.189 * amount));
    color.g = dot(color, float3(0.349 * amount, 1.0 - 0.314 * amount, 0.168 * amount));
    color.b = dot(color, float3(0.272 * amount, 0.534 * amount, 1.0 - 0.869 * amount));
    return color;
}

#define FXAA_REDUCE_MIN (1.0f / 128.0f)
#define FXAA_REDUCE_MUL (1.0f / 8.0f)
#define FXAA_SPAN_MAX 8.0f
#define FXAA_SUBPIX_SHIFT (1.0f / 4.0f)
float3 fxaa(Texture2D color_texture, SamplerState state, float2 texcoord, float2 pixel_size)
{
    //  周辺ピクセルを参照
    float2 texcoord2 = texcoord.xy - (pixel_size * (0.5 + FXAA_SUBPIX_SHIFT));
    float3 sampled_nw = color_texture.Sample(state, texcoord2.xy).xyz;
    float3 sampled_ne = color_texture.Sample(state, texcoord2.xy, float2(1, 0)).xyz;
    float3 sampled_sw = color_texture.Sample(state, texcoord2.xy, float2(0, 1)).xyz;
    float3 sampled_se = color_texture.Sample(state, texcoord2.xy, float2(1, 1)).xyz;
    float3 sampled_m = color_texture.Sample(state, texcoord.xy).xyz;

    //  それぞれの輝度を算出
    float luma_nw = convert_rgb_to_luminance(sampled_nw);
    float luma_ne = convert_rgb_to_luminance(sampled_ne);
    float luma_sw = convert_rgb_to_luminance(sampled_sw);
    float luma_se = convert_rgb_to_luminance(sampled_se);
    float luma_m = convert_rgb_to_luminance(sampled_m);

    //  輝度差の最小最大を取得
    float luma_min = min(luma_m, min(min(luma_nw, luma_ne), min(luma_sw, luma_se)));
    float luma_max = max(luma_m, max(max(luma_nw, luma_ne), max(luma_sw, luma_se)));

    //  輝度差から向きベクトルを算出
    float2 dir;
    dir.x = -((luma_nw + luma_ne) - (luma_sw + luma_se));
    dir.y = +((luma_nw + luma_sw) - (luma_ne + luma_se));

    //  複数ピクセルを横断するようにエッジを伸ばす
    float dir_reduce = max((luma_nw + luma_ne + luma_sw + luma_se) * (0.25f * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcp_dir_min = 1.0f / (min(abs(dir.x), abs(dir.y)) + dir_reduce);
    dir = min(float2(FXAA_SPAN_MAX, FXAA_SPAN_MAX), max(float2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcp_dir_min)) * pixel_size;

    //  向きベクトルを元に複数位置の色をブレンドして返す
    float3 rgb_a = (1.0f / 2.0f)
                 * (color_texture.Sample(state, texcoord.xy + dir * (1.0f / 3.0f - 0.5f)).xyz +
                    color_texture.Sample(state, texcoord.xy + dir * (2.0f / 3.0f - 0.5f)).xyz);
    float3 rgb_b = rgb_a * (1.0f / 2.0f)
                 + (1.0f / 4.0f) * (color_texture.Sample(state, texcoord.xy + dir * (0.0f / 3.0f - 0.5f)).xyz +
		                            color_texture.Sample(state, texcoord.xy + dir * (3.0f / 3.0f - 0.5f)).xyz);
    float luma_b = convert_rgb_to_luminance(rgb_b);
    return ((luma_b < luma_min) || (luma_b > luma_max)) ? rgb_a : rgb_b;
}

 //  ラジアルブラー
float calculate_radial_line(float angle)
{
    float2 line_seed = floor(float2(angle, angle) * ConcentLineDatas.noiseScale);
    float line_value = random(line_seed);
    return lerp(sin(line_value * ConcentLineDatas.patternSeed),
                sin(line_value * time * ConcentLineDatas.animationSpeed),
                ConcentLineDatas.useAnimation);
}

float3 RadialBlur(float2 texcoord)
{
    float2 scene_map_size;
    colorMap.GetDimensions(scene_map_size.x, scene_map_size.y);
 
    float4 color = colorMap.Sample(LinearSampler, texcoord);
    float4 result_color = color;
 
    float2 blur_vector = (radialDatas.center - texcoord);
    blur_vector *= (radialDatas.radius / scene_map_size.xy) / radialDatas.samplingCount;
    for (int index = 1; index < radialDatas.samplingCount; ++index)
    {
        result_color += colorMap.Sample(LinearSampler, texcoord + blur_vector * index);
    }
 
    float mask_radius = radialDatas.maskRadius / min(scene_map_size.x, scene_map_size.y);
    float mask_value = saturate(length(texcoord - radialDatas.center) / mask_radius);
    return lerp(color, result_color / radialDatas.samplingCount, mask_value);

}

//float3 radialBlur(float4 color, float2 texcoord)
//{
//    if (radialDatas.radius > 0.0f && radialDatas.samplingCount > 1)
//    {
//        float4 result_color = color;
        
//        float2 blur_vector = (radialDatas.center - texcoord);

//        //if (blur_adjust)
//        {
//            //  sin関数は-1～+1の値を算出するので、
//            //  ブラーベクトルから角度を求めて、
//            //  sin 関数を用いて振幅値を求める。
//            float pixel_angle = atan2(blur_vector.y, blur_vector.x);
//            float wave_length = lerp(50, 100, random(pixel_angle + oldTime));
//            float amplitude = lerp(20, 50, random(pixel_angle + time));
//            float value = sin(pixel_angle * wave_length + amplitude);
//            blur_vector *= abs(value);
//        }

//        float radius = radialDatas.radius; //  lerp(radialDatas.radius * 0.5f, radialDatas.radius, random(oldTime));
//        blur_vector *= (radius.xx / radialDatas.textureSize.xy) / radialDatas.samplingCount;
//        float rand_factor = random(texcoord) * 0.25f + 0.75f;
//        float total_alpha = 1.0f;
//        for (int index = 1; index < radialDatas.samplingCount; ++index)
//        {
//            float alpha = (float) (radialDatas.samplingCount - index) / radialDatas.samplingCount;
//            result_color += colorMap.Sample(LinearSampler, texcoord + blur_vector * index) * alpha;
//            total_alpha += alpha;
//        }

//        //  指定の範囲内は適応量を変える
//        float mask_value = 1.0f;
//        if (radialDatas.maskRadius > 0.0f)
//        {
//            float mask_radius = radialDatas.maskRadius / min(radialDatas.textureSize.x, radialDatas.textureSize.y);
//            mask_value = saturate(length(texcoord - radialDatas.center) / mask_radius);
//        }

//        return lerp(color, result_color / total_alpha, mask_value);
//    }

//    return color;
//}

float3 Converginglines(float3 color, float2 texcoord)
{
 //  集中線
    if (ConcentLineDatas.intensity > 0.0f)
    {
        float angle = calculate_uv_angle(texcoord);
        float line_value = calculate_radial_line(angle);
        float circle = calculate_center_circle(texcoord);
        float result_line = saturate(line_value * circle);
        float smooth_alpha = smoothstep(ConcentLineDatas.edge.x, ConcentLineDatas.edge.y, result_line);

        return lerp(color.rgb, ConcentLineDatas.color, smooth_alpha);
    }
   
    return color;
}

float3 Accelaration(float4 fragcolor, float2 texcoord)
{

    float2 scene_map_size;
    colorMap.GetDimensions(scene_map_size.x, scene_map_size.y);
    
    const float GammaFactor = 1.0 / 2.2;
    
    //  ガンマ補正を切る
    float4 color = float4(fragcolor.rgb, 1);
    float4 old_color = color;
    float weight = AccelWeight;
    //  画面の中心から外側に向かって灰色＋青を入れていく
    float length_factor = saturate(distance(texcoord, 0.5f) * 2.0f);

     //  ラジアルブラー
    float mask_value_rate = 0.0f;
    {
        float2 blur_center = 0.5f;
        float blur_radius = 50.0f;
        float blur_mask_radius = 500.0f;
        int blur_sampling_count = 15;

        float4 result_color = colorMap.Sample(LinearSampler, texcoord);
        result_color.rgb = float4(pow(result_color.rgb, 1.0f / GammaFactor), 1);

        float2 blur_vector = (blur_center - texcoord);
        blur_vector *= (blur_radius / scene_map_size.xy) / blur_sampling_count;
        for (int index = 1; index < blur_sampling_count; ++index)
        {
            result_color.rgb += pow(colorMap.Sample(LinearSampler, texcoord + blur_vector * index).rgb, 1.0f / GammaFactor);
        }

                //  指定の範囲内は適応量を変える
        float mask_radius = blur_mask_radius / min(scene_map_size.x, scene_map_size.y);
        float mask_value = saturate(length(texcoord - blur_center) / mask_radius);
        color.rgb = lerp(color.rgb, result_color.rgb / blur_sampling_count, mask_value);
        mask_value_rate = mask_value;
    }
            
    //  hsvに変換
    color.rgb = convert_rgb_to_hsv(color.rgb);

    //  全体の彩度を下げる
    color.g *= 0.05f;

    //  画面的に妙に明るいので外側程暗くする
    color.b *= 1.0f - 0.3f * length_factor * length_factor;

    //  rgbに変換
    color.rgb = convert_hsv_to_rgb(color.rgb);

    //  画面全体の青を強くする
    color.b *= 1.0f + 0.35f * length_factor * length_factor * length_factor;

    ////  メッシュ属性に応じて適用を変える
    //weight *= 1 - 0.75f * (data.mesh_attribute == mesh_attribute_player || data.mesh_attribute == mesh_attribute_enemy);
    //return color.rgb;
    return lerp(old_color, saturate(color), weight);
}

float3 GrayScall(float3 color)
{
     //グレースケール
    float3 graycolor = color;
    graycolor.r *= 0.333;
    graycolor.g *= 0.333;
    graycolor.b *= 0.333;

    float gray = graycolor.r + graycolor.g + graycolor.b;
    
    float weight = GrayWeight;
    return lerp(color, saturate(float3(gray, gray, gray)), weight);
}

float4 Flash(float3 color, float2 texcoord)
{
    float2 scene_map_size;
    colorMap.GetDimensions(scene_map_size.x, scene_map_size.y);
    
    float2 texel = 1.0 / scene_map_size;

    // 元の色取得
    float3 col = colorMap.Sample(LinearSampler, texcoord).rgb;

    // エッジ検出（簡易Sobel）
    float3 colL = colorMap.Sample(LinearSampler, texcoord + float2(-texel.x, 0)).rgb;
    float3 colR = colorMap.Sample(LinearSampler, texcoord + float2(texel.x, 0)).rgb;
    float3 colU = colorMap.Sample(LinearSampler, texcoord + float2(0, -texel.y)).rgb;
    float3 colD = colorMap.Sample(LinearSampler, texcoord + float2(0, texel.y)).rgb;

    float edge = length(colL - colR) + length(colU - colD);
    edge = saturate(edge * 5.0); // エッジの強さ調整

    // --- 中央から広がる/収束するマスク ---
    float2 center = float2(0.5, 0.5);
    float dist = distance(texcoord, center);
    float distMask = smoothstep(0.0, 0.707, dist); // 中心0 → 端1

    float localFlash = 0.0;

    localFlash = saturate((flashAmount - distMask) * 5.0); // 拡大

    // 白色への補間
    float3 whiteFlash = lerp(color.rgb, float3(1, 1, 1), localFlash);

    // エッジ分を黒く落とす（シルエット強調）
    float3 finalColor = whiteFlash - edge * localFlash;
    
    // // 白色への補間
    //float3 whiteFlash = lerp(color.rgb, GrayScall(color), localFlash);

    ////// エッジ分を黒く落とす（シルエット強調）
    //float3 finalColor = whiteFlash;

    float mask_radius = flashMask / min(scene_map_size.x, scene_map_size.y);
    float mask_value = saturate(length(texcoord - radialDatas.center) / mask_radius);
    
    return float4(lerp(color, saturate(finalColor), mask_value), 1.0);
}


float4 BlackFlash(float3 color, float2 texcoord)
{
    float2 texelSize;
    colorMap.GetDimensions(texelSize.x, texelSize.y);
    
    texelSize.x = 1 / texelSize.x;
    texelSize.y = 1 / texelSize.y;
    
     // --- エッジ検出 ---
    float3 colL = colorMap.Sample(LinearSampler, texcoord + float2(-texelSize.x, 0)).rgb;
    float3 colR = colorMap.Sample(LinearSampler, texcoord + float2(texelSize.x, 0)).rgb;
    float3 colU = colorMap.Sample(LinearSampler, texcoord + float2(0, -texelSize.y)).rgb;
    float3 colD = colorMap.Sample(LinearSampler, texcoord + float2(0, texelSize.y)).rgb;

    float edge = length(colL - colR) + length(colU - colD);
    edge = saturate(edge * 5.0);

    // --- フラッシュマスク：中心から広がる & 戻る ---
    float2 center = float2(0.5, 0.5);
    float Maskdist = distance(texcoord, center);
    float distMask = smoothstep(0.0, 0.707, Maskdist); // 0.0～1.0

    float localFlash = 0.0;

    if (flashAmount <= 0.8f)
    {
        localFlash = saturate((flashAmount - distMask) * 5.0); // フェーズ1：拡大
    }
    else
    {
        float t = (flashAmount - 0.8f) * 5.0; // 1→0
        localFlash = saturate((t - distMask) * 5.0); // フェーズ2：収束
    }

    // --- フラッシュとエッジ合成 ---
    float3 flash = lerp(color, float3(1, 1, 1), localFlash);
    float3 finalColor = flash - localFlash * edge;
    
    return float4(saturate(finalColor), 1.0);
}


float3 Vignette(float3 color, float2 texcoord)
{
    float2 scene_map_size;
    colorMap.GetDimensions(scene_map_size.x, scene_map_size.y);
    
     //  周辺減光処理 
    float2 d = abs(texcoord - vignetteDatas.vignetteCenter) * (vignetteDatas.vignetteIntensity);
    //  減光をスクリーンに合わすかどうか 
    d.x *= lerp(1.0f, scene_map_size.x / scene_map_size.y, vignetteDatas.vignetteRounded);
    //  隅の濃さ 
    d = pow(saturate(d), vignetteDatas.vignetteRoundness);
    half vignette_factor = pow(saturate(1.0f - dot(d, d)), vignetteDatas.vignetteSmoothness);
    color.rgb *= lerp(vignetteDatas.vignetteColor.rgb, (float3) 1.0f, vignette_factor);
    return color;
};

float4 main(VS_OUT pin) : SV_TARGET
{

    //float4 color = colorMap.Sample(LinearSampler, pin.texcoord);
    float4 color;
    //color.rgb = ChromaticAberration(pin.texcoord);
    color.rgb = fxaa(colorMap, LinearSampler, pin.texcoord, 1.0f / float2(1200, 720));
    //color.rgb = RadialBlur(pin.texcoord);
    //color.rgb = radialBlur(float4(color.rgb, 1.0), pin.texcoord);
    color.rgb = Converginglines(color.rgb, pin.texcoord);
    color.a = 1.0;
    float4 bloom;
    bloom.rgb = fxaa(luminanceMap, LinearSampler, pin.texcoord, 1.0f / float2(1200, 720));

    float3 fragment_color = color.rgb + bloom.rgb;
    
    float alpha = color.a;
    

    
    //fragment_color = radialColor;
    
    //return float4(fragment_color, alpha);
    
    //fragment_color = filmic_tone_mapping(fragment_color);
    ////fragment_color = reinhard_tone_mapping(fragment_color);
    //const float INV_GAMMA = 1.0 / 2.2;
    //fragment_color = pow(fragment_color, INV_GAMMA);

    fragment_color.rgb = Contrast(fragment_color.rgb);
    fragment_color.rgb = Saturation(fragment_color.rgb);
    // セピア
    //fragment_color.rgb = Sepia(fragment_color.rgb, 0.3);
    fragment_color.rgb = ColorFilter(fragment_color.rgb);
    
    fragment_color.rgb = Vignette(fragment_color, pin.texcoord);
    
    fragment_color.rgb = Flash(fragment_color, pin.texcoord);
    
    fragment_color.rgb = Accelaration(float4(fragment_color, alpha), pin.texcoord);
    
    fragment_color.rgb = GrayScall(fragment_color.rgb);
    
    //return float4(fragment_color, alpha);
	// Tone map
    //fragment_color += ACESFilmToneMapping(fragment_color);
    fragment_color = hlsl_tone_map(color, bloom, pin.texcoord);
	//fragment_color = reinhard_tone_mapping(fragment_color);
    
    return float4(fragment_color, alpha);

    
	//// Gamma correction
 //   const float INV_GAMMA = 1.0 / 2.2;
 //   fragment_color = pow(fragment_color, INV_GAMMA);
    
    return float4(fragment_color, alpha);

}
