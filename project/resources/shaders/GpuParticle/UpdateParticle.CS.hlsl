#include "GpuParticle.hlsli"

RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);
ConstantBuffer<PerFrame> gPerFrame : register(b0);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    uint particleIndex = DTid.x;
    if (particleIndex < kMaxParticles) {
        if (gParticles[particleIndex].lifeTime > 0.0f) {
            float dt = gPerFrame.deltaTime;
            gParticles[particleIndex].velocity.y -= gParticles[particleIndex].gravity * dt;
            gParticles[particleIndex].velocity *= saturate(1.0f - gParticles[particleIndex].drag * dt);
            gParticles[particleIndex].translate += gParticles[particleIndex].velocity * dt;
            gParticles[particleIndex].currentTime += dt;

            float progress = saturate(gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime);
            float easeOut = 1.0f - pow(1.0f - progress, 3.0f);
            gParticles[particleIndex].scale = lerp(gParticles[particleIndex].startScale, gParticles[particleIndex].endScale, easeOut);
            gParticles[particleIndex].color.rgb = lerp(gParticles[particleIndex].startColor.rgb, gParticles[particleIndex].endColor.rgb, progress);
            gParticles[particleIndex].color.a = lerp(gParticles[particleIndex].startColor.a, gParticles[particleIndex].endColor.a, progress * progress);

            if (gParticles[particleIndex].currentTime >= gParticles[particleIndex].lifeTime) {
                gParticles[particleIndex].color.a = 0.0f;
                gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
                gParticles[particleIndex].lifeTime = 0.0f;

                int freeListIndex;
                InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
                if ((freeListIndex + 1) < (int)kMaxParticles) {
                    gFreeList[freeListIndex + 1] = particleIndex;
                } else {
                    InterlockedAdd(gFreeListIndex[0], -1);
                }
            }
        }
    }
}
