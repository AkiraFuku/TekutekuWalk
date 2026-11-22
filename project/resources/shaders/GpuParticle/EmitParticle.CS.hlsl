#include "GpuParticle.hlsli"

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
ConstantBuffer<EmitterConfig> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    if (gEmitter.count == 0) {
        return;
    }

    RandomGenerator generator;
    generator.seed = (float3(DTid) + gPerFrame.time + 1.0f) * (gPerFrame.time + 1.37f)
        + gEmitter.translate * 0.13f
        + float3(gEmitter.randomSeed, gEmitter.randomSeed * 1.73f, gEmitter.randomSeed * 2.41f);

    for (uint countIndex = 0; countIndex < gEmitter.count; ++countIndex) {
        int freeListIndex;
        InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);

        if (0 <= freeListIndex && freeListIndex < (int)kMaxParticles) {
            uint particleIndex = gFreeList[freeListIndex];

            float3 rPos = generator.Generate3d();
            float3 rVel = generator.Generate3d();
            float3 rRot = generator.Generate3d();
            float3 rAng = generator.Generate3d();
            float3 rStartScale = generator.Generate3d();
            float3 rEndScale = generator.Generate3d();
            float rLife = generator.Generate1d();
            float rStartColor = generator.Generate1d();
            float rEndColor = generator.Generate1d();

            float3 startScale = lerp(gEmitter.startScaleMin, gEmitter.startScaleMax, rStartScale);
            float3 endScale = lerp(gEmitter.endScaleMin, gEmitter.endScaleMax, rEndScale);
            float4 startColor = lerp(gEmitter.startColorMin, gEmitter.startColorMax, rStartColor);
            float4 endColor = lerp(gEmitter.endColorMin, gEmitter.endColorMax, rEndColor);

            gParticles[particleIndex].translate = gEmitter.translate + lerp(gEmitter.emitRangeMin, gEmitter.emitRangeMax, rPos);
            gParticles[particleIndex].velocity = lerp(gEmitter.velocityMin, gEmitter.velocityMax, rVel);
            gParticles[particleIndex].rotate = lerp(gEmitter.rotateMin, gEmitter.rotateMax, rRot);
            gParticles[particleIndex].angularVelocity = lerp(gEmitter.angularVelocityMin, gEmitter.angularVelocityMax, rAng);
            gParticles[particleIndex].startScale = startScale;
            gParticles[particleIndex].endScale = endScale;
            gParticles[particleIndex].scale = startScale;
            gParticles[particleIndex].startColor = startColor;
            gParticles[particleIndex].endColor = endColor;
            gParticles[particleIndex].color = startColor;
            gParticles[particleIndex].lifeTime = max(lerp(gEmitter.lifeTimeMin, gEmitter.lifeTimeMax, rLife), 0.01f);
            gParticles[particleIndex].currentTime = 0.0f;
            gParticles[particleIndex].drag = gEmitter.drag;
            gParticles[particleIndex].gravity = gEmitter.gravity;
            gParticles[particleIndex].uvOffset = gEmitter.uvOffset;
            gParticles[particleIndex].uvScale = gEmitter.uvScale;
            gParticles[particleIndex].uvScrollSpeed = gEmitter.uvScrollSpeed;
        } else {
            InterlockedAdd(gFreeListIndex[0], 1);
            break;
        }
    }
}
