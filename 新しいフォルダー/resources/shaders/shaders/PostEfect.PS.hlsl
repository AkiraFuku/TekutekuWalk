// PostEffect.ps.hlsl
#include "FullScreen.hlsli"
#include "PostEfect/DepthOutline.hlsli"

// テクスチャ宣言
Texture2D<float4> gTexture : register(t0); // メインカラー
Texture2D<float> gTextureDepth : register(t1); // 深度
Texture2D<float4> gMaskTexture : register(t2); // ディゾルブ用マスク

// サンプラー宣言
SamplerState gSampler : register(s0); // リニア
SamplerState gSamplerPoint : register(s1); // ポイント

// 定数バッファ宣言
struct PostEffectMaterial
{
    float4x4 projectionInverse;
    float time;
    uint activeFlags;
    float2 padding;
};
ConstantBuffer<PostEffectMaterial> gMaterial : register(b0);

struct BlurParam
{
    float2 center;
    int radius;
    float blurWidth;
};
ConstantBuffer<BlurParam> gBlurParam : register(b1);

struct DissolveParam
{
    float threshold;
    float3 padding;
};
ConstantBuffer<DissolveParam> gDissolveParam : register(b2);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // ベースカラーのサンプリング
    float4 baseColor = gTexture.Sample(gSampler, input.texcoord);
    float3 finalColor = baseColor.rgb;

    // 1ピクセル幅の計算
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(rcp(width), rcp(height));

    // ----------------------------------------------------
    // フラグ判定によるエフェクト適用
    // ----------------------------------------------------
    
    // 深度アウトライン適用判定
    if ((gMaterial.activeFlags & POST_EFFECT_DEPTH_OUTLINE) != 0)
    {
        finalColor = ApplyDepthOutline(
            finalColor,
            input.texcoord,
            uvStepSize,
            gTextureDepth,
            gSamplerPoint,
            gMaterial.projectionInverse
        );
    }

    output.color = float4(finalColor, baseColor.a);
    return output;
}