static const uint kMaxParticles = 1024;

struct Particle {
    float3 translate;
    float lifeTime;
    float3 scale;
    float currentTime;
    float3 velocity;
    float drag;
    float3 rotate;
    float gravity;
    float3 startScale;
    float pad0;
    float3 endScale;
    float pad1;
    float4 color;
    float4 startColor;
    float4 endColor;
};

struct PerView {
    float4x4 viewProjection;
    float4x4 billboardMatrix;
    uint useBillboard;
    uint useVelocityStretch;
    float2 padding;
};

struct PerFrame {
    float time;
    float deltaTime;
};

struct EmitterConfig {
    float3 translate;
    uint count;
    float3 emitRangeMin;
    float lifeTimeMin;
    float3 emitRangeMax;
    float lifeTimeMax;
    float3 startScaleMin;
    float drag;
    float3 startScaleMax;
    float gravity;
    float3 endScaleMin;
    float pad0;
    float3 endScaleMax;
    float pad1;
    float3 rotateMin;
    float pad2;
    float3 rotateMax;
    float pad3;
    float3 velocityMin;
    float pad4;
    float3 velocityMax;
    float pad5;
    float4 startColorMin;
    float4 startColorMax;
    float4 endColorMin;
    float4 endColorMax;
};

struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR0;
};

float rand3dTo1d(float3 value, float3 dotDir = float3(12.9898f, 78.233f, 37.719f)) {
    float3 smallValue = sin(value);
    float random = dot(smallValue, dotDir);
    random = frac(sin(random) * 143758.5453f);
    return random;
}

float3 rand3dTo3d(float3 value) {
    return float3(
        rand3dTo1d(value, float3(12.989f, 78.233f, 37.719f)),
        rand3dTo1d(value, float3(39.346f, 11.135f, 83.155f)),
        rand3dTo1d(value, float3(73.156f, 52.235f, 9.151f))
    );
}

class RandomGenerator {
    float3 seed;
    float3 Generate3d() {
        seed = rand3dTo3d(seed);
        return seed;
    }
    float Generate1d() {
        float result = rand3dTo1d(seed);
        seed.x = result;
        return result;
    }
};
