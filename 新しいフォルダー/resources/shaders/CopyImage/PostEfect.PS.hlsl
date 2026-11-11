// PostEffect.ps.hlsl
#include "FullScreen.hlsli"
#include "../PostEfect/Grayscale.hlsli"
#include "../PostEfect/DepthOutline.hlsli"
#include "../PostEfect/LuminanceOutline.hlsli"
#include "../PostEfect/RadialBlur.hlsli"
#include "../PostEfect/Random.hlsli"
#include "../PostEfect/Dissolve.hlsli"
#include "../PostEfect/Vignette.hlsli"     
#include "../PostEfect/BoxFilter.hlsli"    
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
    
    // 1ピクセル幅の計算
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(rcp(width), rcp(height));

    // ----------------------------------------------------
    // 1. ディゾルブ判定（画面から破棄する判定を最優先）
    // ----------------------------------------------------
    float3 finalColor = gTexture.Sample(gSampler, input.texcoord).rgb;

    if ((gMaterial.activeFlags & POST_EFFECT_DISSOLVE) != 0)
    {
        finalColor = ApplyDissolve(
            finalColor,
            input.texcoord,
            gMaskTexture,
            gSampler,
            gDissolveParam.threshold
        );
    }
    
    // ----------------------------------------------------
    // 2. ブラー・フィルタ系
    // ----------------------------------------------------
    if ((gMaterial.activeFlags & POST_EFFECT_BOX_FILTER) != 0)
    {
        finalColor = ApplyBoxFilter(gTexture, gSampler, input.texcoord, uvStepSize);
    }

    if ((gMaterial.activeFlags & POST_EFFECT_RADIAL_BLUR) != 0)
    {
        finalColor = ApplyRadialBlur(
            gTexture,
            gSampler,
            input.texcoord,
            gBlurParam.center,
            gBlurParam.radius,
            gBlurParam.blurWidth
        );
    }

    // ----------------------------------------------------
    // 3. カラー・トーン・画面減光調整系
    // ----------------------------------------------------
    if ((gMaterial.activeFlags & POST_EFFECT_RANDOM) != 0)
    {
        finalColor = ApplyRandomNoise(finalColor, input.texcoord, gMaterial.time);
    }

    if ((gMaterial.activeFlags & POST_EFFECT_GRAYSCALE) != 0)
    {
        finalColor = ApplyGrayscale(finalColor);
    }

    if ((gMaterial.activeFlags & POST_EFFECT_VIGNETTE) != 0)
    {
        finalColor = ApplyVignette(finalColor, input.texcoord);
    }
    // ----------------------------------------------------
    // 4. アウトライン系（輪郭線を一番上に重畳）
    // ----------------------------------------------------
    if ((gMaterial.activeFlags & POST_EFFECT_LUMINANCE_OUTLINE) != 0)
    {
        finalColor = ApplyLuminanceOutline(
            finalColor,
            gTexture,
            gSampler,
            input.texcoord,
            uvStepSize
        );
    }

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
    

    float baseAlpha = gTexture.Sample(gSampler, input.texcoord).a;
    output.color = float4(finalColor, baseAlpha);
    return output;
}