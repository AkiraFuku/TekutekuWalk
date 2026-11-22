#include "GpuParticle.hlsli"

StructuredBuffer<Particle> gParticles : register(t0);
ConstantBuffer<PerView> gPerView : register(b0);

struct VertexShaderInput {
    float4 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

float4x4 MakeRotateXYZMatrix(float3 rot) {
    float cx = cos(rot.x), sx = sin(rot.x);
    float cy = cos(rot.y), sy = sin(rot.y);
    float cz = cos(rot.z), sz = sin(rot.z);

    float4x4 rx = float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f,   cx,   sx, 0.0f,
        0.0f,  -sx,   cx, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    float4x4 ry = float4x4(
          cy, 0.0f,  -sy, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
          sy, 0.0f,   cy, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    float4x4 rz = float4x4(
          cz,   sz, 0.0f, 0.0f,
         -sz,   cz, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    return mul(mul(rx, ry), rz);
}

VertexShaderOutput main(VertexShaderInput input, uint instanceId : SV_InstanceID) {
    VertexShaderOutput output;
    Particle particle = gParticles[instanceId];

    if (particle.lifeTime <= 0.0f) {
        output.position = float4(0.0f, 0.0f, 0.0f, 0.0f);
        output.texCoord = input.texCoord;
        output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return output;
    }

    float4x4 worldMatrix = float4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    float speed = length(particle.velocity);
    if (gPerView.useVelocityStretch != 0 && speed > 0.001f) {
        float3 velDir = particle.velocity / speed;
        float3 viewDir = normalize(gPerView.billboardMatrix[2].xyz);
        float3 right = cross(velDir, viewDir);
        if (length(right) < 0.001f) {
            right = cross(velDir, float3(1.0f, 0.0f, 0.0f));
            if (length(right) < 0.001f) {
                right = cross(velDir, float3(0.0f, 0.0f, 1.0f));
            }
        }
        right = normalize(right);
        float3 forward = normalize(cross(right, velDir));
        float stretch = 1.0f + speed * 0.45f;

        worldMatrix[0] = float4(right * particle.scale.x, 0.0f);
        worldMatrix[1] = float4(velDir * (particle.scale.y * stretch), 0.0f);
        worldMatrix[2] = float4(forward * particle.scale.z, 0.0f);
    } else if (gPerView.useBillboard != 0) {
        float4x4 rotMat = MakeRotateXYZMatrix(particle.rotate);
        float4x4 bbMat = mul(rotMat, gPerView.billboardMatrix);
        worldMatrix[0] = bbMat[0] * particle.scale.x;
        worldMatrix[1] = bbMat[1] * particle.scale.y;
        worldMatrix[2] = bbMat[2] * particle.scale.z;
    } else {
        float4x4 rotMat = MakeRotateXYZMatrix(particle.rotate);
        worldMatrix[0] = rotMat[0] * particle.scale.x;
        worldMatrix[1] = rotMat[1] * particle.scale.y;
        worldMatrix[2] = rotMat[2] * particle.scale.z;
    }

    worldMatrix[3] = float4(particle.translate, 1.0f);

    output.position = mul(input.position, mul(worldMatrix, gPerView.viewProjection));
    float2 uv = input.texCoord * particle.uvScale + particle.uvOffset;
    uv.x += input.texCoord.y * gPerView.uvTwist;
    if (gPerView.flipUvY != 0) {
        uv.y = 1.0f - uv.y;
    }
    output.texCoord = uv;
    output.color = particle.color;
    return output;
}
