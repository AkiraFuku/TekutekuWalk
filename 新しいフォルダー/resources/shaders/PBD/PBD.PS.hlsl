#include "PBD.hlsli"

struct Material {
    float4 color;
    int enableLighting;
    float3 padding;
};

struct Camera {
    float3 worldPosition;
    float farClip;
    float3 cameraForward;
    float padding;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<Camera>   gCamera   : register(b1);

Texture2D<float4> gTexture : register(t0);
SamplerState      gSampler : register(s0);

struct PixelShaderOutput {
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    // テクスチャサンプリング
    float4 textureColor = gTexture.Sample(gSampler, input.texCoord);
    
    // ベースカラー計算
    float4 finalColor = gMaterial.color * textureColor;
    
    // アルファテスト
    if (finalColor.a < 0.01f) {
        discard;
    }
    
    if (gMaterial.enableLighting != 0) {
        // 法線の正規化
        float3 normal = normalize(input.normal);
        
        // 視線方向
        float3 viewDir = normalize(gCamera.worldPosition - input.worldPosition);
        
        // 布の裏面でもライティングが成立するように、法線を視線側に向ける（両面ライティング）
        if (dot(normal, viewDir) < 0.0f) {
            normal = -normal;
        }
        
        // 簡易ディレクショナルライト（斜め上からの平行光）
        float3 lightDir = normalize(float3(0.5f, 1.0f, -0.5f));
        float NdotL = max(dot(normal, lightDir), 0.0f);
        
        // ハーフランバートライティング + 環境光
        float halfLambert = pow(NdotL * 0.5f + 0.5f, 2.0f);
        float3 ambient = float3(0.35f, 0.35f, 0.35f);
        float3 diffuse = float3(1.0f, 1.0f, 1.0f) * halfLambert;
        
        finalColor.rgb *= (diffuse + ambient);
    }
    
    output.color = finalColor;
    return output;
}
