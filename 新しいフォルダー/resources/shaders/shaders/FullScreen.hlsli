struct VertexShaderOutput{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// --- ポストエフェクト フラグ定義 ---
static const uint POST_EFFECT_NONE = 0;
static const uint POST_EFFECT_GRAYSCALE = 1 << 0; // 0x01
static const uint POST_EFFECT_RANDOM = 1 << 1; // 0x02
static const uint POST_EFFECT_VIGNETTE = 1 << 2; // 0x04
static const uint POST_EFFECT_RADIAL_BLUR = 1 << 3; // 0x08
static const uint POST_EFFECT_DEPTH_OUTLINE = 1 << 4; // 0x10
static const uint POST_EFFECT_Luminance_OUTLINE = 1 << 5; // 0x10
static const uint POST_EFFECT_DISSOLVE = 1 << 6; // 0x20