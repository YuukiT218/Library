cbuffer CbPostEffect : register(b0)
{
    // 輝度の範囲を指定
    float luminanceExtractionLowerEdge;
    float luminanceExtractionHigherEdge;
    // ぼかし具合
    float gaussianSigma;
    // 光の溢れ具合
    float bloomIntensity;
    //トーンマッピング
    float exposure;
    //露出レベル
    float fExposureLevel;
    // 閾値
    float threshold;
    
    //時間
    float time; //現在の時間
    float oldTime; //前フレームの時間
    
    //今後も実装していく
    float3 postDummy;
};

#define KERNEL_MAX 25

cbuffer GAUSSIAN_FILTER : register(b1)
{
    float4 weights[KERNEL_MAX * KERNEL_MAX];
    float kernelSize;
    float2 texcel;
    float dummy;

}

//	ラジアルブラー情報
struct RadialBlurDatas
{
    float radius;
    int samplingCount;
    float2 center;

    float maskRadius;
    
    float2 textureSize;
    
    float dummy;
};

//	集中線処理
struct ConcentratedLineDatas
{
    float intensity; // 色調補正適応量
    float useAnimation; // アニメーション使用フラグ
    float animationSpeed; // アニメーション速度
    float patternSeed; // パターンシード値

    float noiseScale; // ノイズ拡大値
    float3 color; // 集中線色

    float2 edge; // エッジ
    float2 dummy;
};

struct VignetteDatas
{
    float4 vignetteColor;
    float2 vignetteCenter;
    float vignetteIntensity;
    float vignetteSmoothness;
 
    float vignetteRounded;
    float vignetteRoundness;
    float vignetteOpacity;
    float vignette_dummy;
};

//FinalPost
cbuffer CbFpost : register(b2)
{
    float contrast;
    float saturation;
    float chromatic_aberration;
    float flashAmount; // 0.0 ~ 1.0 で白さを調整（1.0で真っ白）
    
    float4 chromaticAberrationShift[3];

    float3 colorFilter;
    float chromaticMask;
    
    float flashMask;
    
    float AccelWeight;
    
    float GrayWeight;
    
    float pad; //4個区切り用
    
    RadialBlurDatas radialDatas;
    
    ConcentratedLineDatas ConcentLineDatas;
    
    VignetteDatas vignetteDatas;
}

