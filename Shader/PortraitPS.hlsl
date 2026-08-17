#include "Sprite.hlsli"

// レジスタ設定
Texture2D mainTexture : register(t0); // キャラクターの立ち絵
SamplerState mainSampler : register(s0);

// C++から送られてくる定数バッファ
cbuffer PortraitParam : register(b0)
{
    float2 circleCenter;    // HPGaugeのグレー円の中心(スクリーン座標)
    float circleRadius;     // グレー円の半径(スクリーン座標)
    float edgeSoftness;     // 縁のぼかし幅(ピクセル)

    float2 headCenter;      // 頭の突き抜けを許可する楕円の中心(スクリーン座標)
    float2 headRadius;      // 頭の突き抜けを許可する楕円の半径(スクリーン座標)
};

float4 main(VS_OUT pin) : SV_TARGET
{
    // 立ち絵の色を取得
    float4 color = mainTexture.Sample(mainSampler, pin.texcoord) * pin.color;

    // 透明な部分は計算スキップ
    if (color.a <= 0.004f)
        discard;

    // ピクセルのスクリーン座標
    float2 screenPos = pin.position.xy;
    float softness = max(edgeSoftness, 0.0001f);

    // グレー円による切り抜き(体の下部分はここで切り取られる)
    float distCircle = length(screenPos - circleCenter);
    float maskCircle = saturate((circleRadius - distCircle) / softness);

    // 頭が上に突き抜けるための楕円(円との合成領域になる)
    float2 headVec = (screenPos - headCenter) / max(headRadius, 0.0001f);
    float distHead = length(headVec);
    // 楕円の距離を大まかにピクセル単位へ戻してぼかし幅を合わせる
    float headScale = min(headRadius.x, headRadius.y);
    float maskHead = saturate((1.0f - distHead) * headScale / softness);

    // 二つの領域の和が最終的な切り抜き範囲
    float mask = max(maskCircle, maskHead);
    if (mask <= 0.001f)
        discard;

    color.a *= mask;

    return color;
}
