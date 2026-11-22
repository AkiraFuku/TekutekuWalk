#include "GpuParticle.hlsli"

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
ConstantBuffer<EmitterSphere> gEmitter : register(b0);
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    if (gEmitter.emit != 0) {
        RandomGenerator generator;
        generator.seed = (float3(DTid) + gPerFrame.time + 1.0f) * (gPerFrame.time + 1.0f);

        for (uint countIndex = 0; countIndex < gEmitter.count; ++countIndex) {
            int freeListIndex;
            InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);

            if (0 <= freeListIndex && freeListIndex < (int)kMaxParticles) {
                uint particleIndex = gFreeList[freeListIndex];
                gParticles[particleIndex].scale = float3(0.3f, 0.3f, 0.3f);
                gParticles[particleIndex].translate = gEmitter.translate + (generator.Generate3d() * 2.0f - 1.0f) * gEmitter.radius;
                gParticles[particleIndex].velocity = (generator.Generate3d() * 2.0f - 1.0f) * 0.05f;
                gParticles[particleIndex].lifeTime = 1.0f + generator.Generate1d();
                gParticles[particleIndex].currentTime = 0.0f;
                gParticles[particleIndex].color = float4(generator.Generate3d(), 1.0f);
            } else {
                InterlockedAdd(gFreeListIndex[0], 1);
                break;
            }
        }
    }
}
