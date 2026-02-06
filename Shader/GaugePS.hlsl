#include "Sprite.hlsli"

// レジスタ設定
Texture2D mainTexture : register(t0); // 緑色のバー画像
Texture2D maskTexture : register(t1); // グラデーションマスク画像
SamplerState mainSampler : register(s0);

// C++から送られてくる定数バッファ
cbuffer GaugeParam : register(b0)
{
    float hpRatio; // 現在のHP割合 (0.0 ～ 1.0)
    float3 padding;
};

float4 main(VS_OUT pin) : SV_TARGET
{
    // 1. メイン画像（緑バー）の色を取得
    float4 color = mainTexture.Sample(mainSampler, pin.texcoord) * pin.color;
    
    // 透明な部分は計算スキップ
    if (color.a <= 0.01f)
        discard;

    // 2. マスク画像の明るさを取得 (白=1.0, 黒=0.0)
    // 画像のR成分を使用します
    float maskValue = maskTexture.Sample(mainSampler, pin.texcoord).r;
    
    // 3. 描画判定 (Clip)
    // マスクの値が HP割合 より大きい場所（しっぽ側）は描画しない
    // これにより、HPが減るとグラデーションに沿って消えていきます
    clip(hpRatio - maskValue);

    return color;
}