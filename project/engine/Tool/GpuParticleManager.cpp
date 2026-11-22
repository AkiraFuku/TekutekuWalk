#include "GpuParticleManager.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include <numbers>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace {
size_t Align256(size_t size) {
    return (size + 255) & ~size_t(255);
}

Vector3 RotateY(const Vector3& v, float yaw) {
    float c = std::cos(yaw);
    float s = std::sin(yaw);
    return {
        v.x * c + v.z * s,
        v.y,
        -v.x * s + v.z * c
    };
}
} // namespace

std::unique_ptr<GpuParticleManager> GpuParticleManager::instance_ = nullptr;

GpuParticleManager* GpuParticleManager::GetInstance() {
    if (instance_ == nullptr) {
        struct Helper : public GpuParticleManager {
            Helper() : GpuParticleManager() {}
        };
        instance_ = std::make_unique<Helper>();
    }
    return instance_.get();
}

void GpuParticleManager::Initialize() {
    RegisterPsos();
    CreateSharedResources();
    CreatePrimitiveBuffers();
    totalTime_ = 0.0f;
}

void GpuParticleManager::Finalize() {
    for (auto& [name, group] : groups_) {
        if (!group) continue;
        if (group->perViewResource) {
            group->perViewResource->Unmap(0, nullptr);
            group->perViewData = nullptr;
            group->perViewResource.Reset();
        }
        for (uint32_t i = 0; i < kMaxEmitRequestsPerFrame; ++i) {
            if (group->emitRequestResources[i]) {
                group->emitRequestResources[i]->Unmap(0, nullptr);
                group->emitRequestData[i] = nullptr;
                group->emitRequestResources[i].Reset();
            }
        }
        group->particleResource.Reset();
        group->freeListIndexResource.Reset();
        group->freeListResource.Reset();
    }
    groups_.clear();
    groupOrder_.clear();
    effectSets_.clear();

    if (perFrameResource_) {
        perFrameResource_->Unmap(0, nullptr);
        perFrameData_ = nullptr;
        perFrameResource_.Reset();
    }
    primitiveResources_.clear();

    instance_.reset();
}

void GpuParticleManager::RegisterPsos() {
    auto psoManager = PSOManager::GetInstance();
    auto device = DXCommon::GetInstance()->GetDevice().Get();

    // 1. InitializeParticle.CS
    {
        PsoConfig config{};
        config.shaderPaths.push_back({ ShaderType::CS, L"resources/shaders/GpuParticle/InitializeParticle.CS.hlsl", "main", L"cs_6_0" });
        config.rootSignatureGenerator = [device]() {
            return RootSignatureBuilder()
                .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, D3D12_SHADER_VISIBILITY_ALL) // u0: gParticles
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, D3D12_SHADER_VISIBILITY_ALL) // u1: gFreeListIndex
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2, D3D12_SHADER_VISIBILITY_ALL) // u2: gFreeList
                .Build(device);
        };
        psoManager->RegisterPsoGenerator("GpuParticleInitializeCS", config);
    }

    // 2. EmitParticle.CS
    {
        PsoConfig config{};
        config.shaderPaths.push_back({ ShaderType::CS, L"resources/shaders/GpuParticle/EmitParticle.CS.hlsl", "main", L"cs_6_0" });
        config.rootSignatureGenerator = [device]() {
            return RootSignatureBuilder()
                .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, D3D12_SHADER_VISIBILITY_ALL) // u0: gParticles
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, D3D12_SHADER_VISIBILITY_ALL) // u1: gFreeListIndex
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2, D3D12_SHADER_VISIBILITY_ALL) // u2: gFreeList
                .AddCBV(0, D3D12_SHADER_VISIBILITY_ALL)                                               // b0: gEmitter
                .AddCBV(1, D3D12_SHADER_VISIBILITY_ALL)                                               // b1: gPerFrame
                .Build(device);
        };
        psoManager->RegisterPsoGenerator("GpuParticleEmitCS", config);
    }

    // 3. UpdateParticle.CS
    {
        PsoConfig config{};
        config.shaderPaths.push_back({ ShaderType::CS, L"resources/shaders/GpuParticle/UpdateParticle.CS.hlsl", "main", L"cs_6_0" });
        config.rootSignatureGenerator = [device]() {
            return RootSignatureBuilder()
                .SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE)
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, D3D12_SHADER_VISIBILITY_ALL) // u0: gParticles
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 1, D3D12_SHADER_VISIBILITY_ALL) // u1: gFreeListIndex
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 2, D3D12_SHADER_VISIBILITY_ALL) // u2: gFreeList
                .AddCBV(0, D3D12_SHADER_VISIBILITY_ALL)                                               // b0: gPerFrame
                .Build(device);
        };
        psoManager->RegisterPsoGenerator("GpuParticleUpdateCS", config);
    }

    // 4. Graphics PSO (GpuParticle)
    {
        PsoConfig config{};
        config.shaderPaths.push_back({ ShaderType::VS, L"resources/shaders/GpuParticle/GpuParticle.VS.hlsl", "main", L"vs_6_0" });
        config.shaderPaths.push_back({ ShaderType::PS, L"resources/shaders/GpuParticle/GpuParticle.PS.hlsl", "main", L"ps_6_0" });
        config.rootSignatureGenerator = [device, psoManager]() {
            auto sampler = psoManager->StaticSamplers();
            sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            return RootSignatureBuilder()
                .AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX)                                                  // b0: gPerView
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX) // t0 (VS): gParticles
                .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)  // t0 (PS): gTexture
                .AddStaticSampler(sampler)
                .Build(device);
        };
        config.inputLayoutGenerator = []() {
            InputLayout layout{};
            layout.inputElement = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            };
            layout.inputLayout.pInputElementDescs = layout.inputElement.data();
            layout.inputLayout.NumElements = static_cast<UINT>(layout.inputElement.size());
            return layout;
        };
        config.depthEnable = true;
        config.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        config.cullMode = D3D12_CULL_MODE_NONE;
        psoManager->RegisterPsoGenerator("GpuParticle", config);
    }
}

void GpuParticleManager::CreateSharedResources() {
    auto dxCommon = DXCommon::GetInstance();
    perFrameResource_ = dxCommon->CreateBufferResource(Align256(sizeof(PerFrame)));
    perFrameResource_->Map(0, nullptr, reinterpret_cast<void**>(&perFrameData_));
    perFrameData_->time = 0.0f;
    perFrameData_->deltaTime = DXCommon::kDeltaTime;
}

void GpuParticleManager::CreatePrimitiveBuffers() {
    CreatePrimitiveBuffer(EffectType::Plane);
    CreatePrimitiveBuffer(EffectType::Ring);
    CreatePrimitiveBuffer(EffectType::Cylinder);
}

void GpuParticleManager::CreatePrimitiveBuffer(EffectType type) {
    std::vector<VertexData> vertices;
    switch (type) {
    case EffectType::Plane:
        vertices = PrimitiveVertexPlane();
        break;
    case EffectType::Ring:
        vertices = PrimitiveVertexRing();
        break;
    case EffectType::Cylinder:
        vertices = PrimitiveVertexCylinder();
        break;
    }

    PrimitiveResource prim{};
    prim.vertexCount = static_cast<uint32_t>(vertices.size());
    size_t sizeVB = sizeof(VertexData) * vertices.size();
    prim.resource = DXCommon::GetInstance()->CreateBufferResource(sizeVB);
    prim.vbv.BufferLocation = prim.resource->GetGPUVirtualAddress();
    prim.vbv.SizeInBytes = static_cast<UINT>(sizeVB);
    prim.vbv.StrideInBytes = sizeof(VertexData);

    void* mapped = nullptr;
    prim.resource->Map(0, nullptr, &mapped);
    std::memcpy(mapped, vertices.data(), sizeVB);
    prim.resource->Unmap(0, nullptr);

    primitiveResources_[type] = std::move(prim);
}

std::vector<GpuParticleManager::VertexData> GpuParticleManager::PrimitiveVertexPlane() {
    return {
        {{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    };
}

std::vector<GpuParticleManager::VertexData> GpuParticleManager::PrimitiveVertexRing() {
    const uint32_t kRingDivide = 32;
    const float kOuterRadius = 1.0f;
    const float kInnerRadius = 0.2f;
    const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kRingDivide);
    std::vector<VertexData> vertices;
    vertices.reserve(kRingDivide * 6);
    for (uint32_t i = 0; i < kRingDivide; ++i) {
        float s = std::sin(i * radianPerDivide);
        float c = std::cos(i * radianPerDivide);
        float sNext = std::sin((i + 1) * radianPerDivide);
        float cNext = std::cos((i + 1) * radianPerDivide);
        float u = float(i) / float(kRingDivide);
        float uNext = float(i + 1) / float(kRingDivide);

        VertexData p1 = { {c * kInnerRadius, s * kInnerRadius, 0.0f, 1.0f}, {u, 1.0f}, {0.0f, 0.0f, 1.0f} };
        VertexData p2 = { {c * kOuterRadius, s * kOuterRadius, 0.0f, 1.0f}, {u, 0.0f}, {0.0f, 0.0f, 1.0f} };
        VertexData p3 = { {cNext * kInnerRadius, sNext * kInnerRadius, 0.0f, 1.0f}, {uNext, 1.0f}, {0.0f, 0.0f, 1.0f} };
        VertexData p4 = { {cNext * kOuterRadius, sNext * kOuterRadius, 0.0f, 1.0f}, {uNext, 0.0f}, {0.0f, 0.0f, 1.0f} };

        vertices.push_back(p1);
        vertices.push_back(p2);
        vertices.push_back(p3);
        vertices.push_back(p2);
        vertices.push_back(p4);
        vertices.push_back(p3);
    }
    return vertices;
}

std::vector<GpuParticleManager::VertexData> GpuParticleManager::PrimitiveVertexCylinder() {
    const uint32_t kDivide = 32;
    const float kTopRadius = 0.5f;
    const float kBottomRadius = 0.5f;
    const float kHeight = 1.0f;
    const float radianPerDivide = 2.0f * std::numbers::pi_v<float> / float(kDivide);
    std::vector<VertexData> vertices;
    vertices.reserve(kDivide * 6);
    for (uint32_t i = 0; i < kDivide; ++i) {
        float s = std::sin(i * radianPerDivide);
        float c = std::cos(i * radianPerDivide);
        float sNext = std::sin((i + 1) * radianPerDivide);
        float cNext = std::cos((i + 1) * radianPerDivide);
        float u = float(i) / float(kDivide);
        float uNext = float(i + 1) / float(kDivide);

        VertexData p1 = { {-s * kTopRadius, kHeight, c * kTopRadius, 1.0f}, {u, 0.0f}, {-s, 0.0f, c} };
        VertexData p2 = { {-sNext * kTopRadius, kHeight, cNext * kTopRadius, 1.0f}, {uNext, 0.0f}, {-sNext, 0.0f, cNext} };
        VertexData p3 = { {-s * kBottomRadius, 0.0f, c * kBottomRadius, 1.0f}, {u, 1.0f}, {-s, 0.0f, c} };
        VertexData p4 = { {-sNext * kBottomRadius, 0.0f, cNext * kBottomRadius, 1.0f}, {uNext, 1.0f}, {-sNext, 0.0f, cNext} };

        vertices.push_back(p1);
        vertices.push_back(p2);
        vertices.push_back(p3);
        vertices.push_back(p2);
        vertices.push_back(p4);
        vertices.push_back(p3);
    }
    return vertices;
}

void GpuParticleManager::InitGroupResources(GpuParticleGroup& group) {
    auto dxCommon = DXCommon::GetInstance();
    auto srvManager = SrvManager::GetInstance();

    group.particleResource = dxCommon->CreateUAVBufferResource(sizeof(ParticleCS) * kMaxParticles);
    group.particleUavIndex = srvManager->AllocateSRV();
    srvManager->CreateUAVForStructuredBuffer(group.particleUavIndex, group.particleResource.Get(), kMaxParticles, sizeof(ParticleCS));
    group.particleSrvIndex = srvManager->AllocateSRV();
    srvManager->CreateSRVForStructuredBuffer(group.particleSrvIndex, group.particleResource.Get(), kMaxParticles, sizeof(ParticleCS));

    group.freeListIndexResource = dxCommon->CreateUAVBufferResource(sizeof(int32_t));
    group.freeListIndexUavIndex = srvManager->AllocateSRV();
    srvManager->CreateUAVForStructuredBuffer(group.freeListIndexUavIndex, group.freeListIndexResource.Get(), 1, sizeof(int32_t));

    group.freeListResource = dxCommon->CreateUAVBufferResource(sizeof(uint32_t) * kMaxParticles);
    group.freeListUavIndex = srvManager->AllocateSRV();
    srvManager->CreateUAVForStructuredBuffer(group.freeListUavIndex, group.freeListResource.Get(), kMaxParticles, sizeof(uint32_t));

    group.perViewResource = dxCommon->CreateBufferResource(Align256(sizeof(PerView)));
    group.perViewResource->Map(0, nullptr, reinterpret_cast<void**>(&group.perViewData));
    *group.perViewData = {};
    group.perViewData->viewProjection = Makeidentity4x4();
    group.perViewData->billboardMatrix = Makeidentity4x4();
    group.perViewData->useBillboard = group.useBillboard ? 1u : 0u;
    group.perViewData->useVelocityStretch = group.useVelocityStretch ? 1u : 0u;

    for (uint32_t i = 0; i < kMaxEmitRequestsPerFrame; ++i) {
        group.emitRequestResources[i] = dxCommon->CreateBufferResource(Align256(sizeof(EmitterConfigGPU)));
        group.emitRequestResources[i]->Map(0, nullptr, reinterpret_cast<void**>(&group.emitRequestData[i]));
        *group.emitRequestData[i] = {};
    }
}

void GpuParticleManager::CreateParticleGroup(
    const std::string& groupName,
    const std::string& textureFilepath,
    const EmitterConfig& config,
    bool useBillboard,
    bool useVelocityStretch,
    BlendMode blendMode,
    EffectType effectType
) {
    TextureManager::GetInstance()->LoadTexture(textureFilepath);
    uint32_t texSrv = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilepath);

    auto it = groups_.find(groupName);
    if (it != groups_.end() && it->second) {
        it->second->effectType = effectType;
        it->second->config = config;
        it->second->useBillboard = useBillboard;
        it->second->useVelocityStretch = useVelocityStretch;
        it->second->blendMode = blendMode;
        it->second->textureSrvIndex = texSrv;
        return;
    }

    auto group = std::make_unique<GpuParticleGroup>();
    group->name = groupName;
    group->effectType = effectType;
    group->config = config;
    group->useBillboard = useBillboard;
    group->useVelocityStretch = useVelocityStretch;
    group->blendMode = blendMode;
    group->textureSrvIndex = texSrv;
    InitGroupResources(*group);

    groups_[groupName] = std::move(group);
    groupOrder_.push_back(groupName);
}

void GpuParticleManager::AddGroupToSet(const std::string& setName, const std::string& groupName) {
    auto& list = effectSets_[setName];
    if (std::find(list.begin(), list.end(), groupName) == list.end()) {
        list.push_back(groupName);
    }
}

void GpuParticleManager::RegisterEffectGroup(
    const std::string& setName,
    const std::string& groupName,
    const std::string& textureFilepath,
    const EmitterConfig& config,
    bool useBillboard,
    bool useVelocityStretch,
    BlendMode blendMode,
    EffectType effectType
) {
    CreateParticleGroup(groupName, textureFilepath, config, useBillboard, useVelocityStretch, blendMode, effectType);
    AddGroupToSet(setName, groupName);
}

void GpuParticleManager::Emit(const std::string& setName, const Vector3& position) {
    if (!isActive_) return;
    auto it = effectSets_.find(setName);
    if (it == effectSets_.end()) return;
    for (const auto& groupName : it->second) {
        EmitGroup(groupName, position);
    }
}

void GpuParticleManager::Emit(const std::string& setName, const Vector3& position, const Vector3& direction) {
    if (!isActive_) return;
    auto it = effectSets_.find(setName);
    if (it == effectSets_.end()) return;
    for (const auto& groupName : it->second) {
        EmitGroup(groupName, position, direction);
    }
}

void GpuParticleManager::EmitGroup(const std::string& groupName, const Vector3& position) {
    if (!isActive_) return;
    auto it = groups_.find(groupName);
    if (it == groups_.end() || !it->second) return;
    if (it->second->pendingEmits.size() >= kMaxEmitRequestsPerFrame) return;
    it->second->pendingEmits.push_back({ position, { 0.0f, 0.0f, 1.0f }, false });
}

void GpuParticleManager::EmitGroup(const std::string& groupName, const Vector3& position, const Vector3& direction) {
    if (!isActive_) return;
    auto it = groups_.find(groupName);
    if (it == groups_.end() || !it->second) return;
    if (it->second->pendingEmits.size() >= kMaxEmitRequestsPerFrame) return;
    it->second->pendingEmits.push_back({ position, direction, true });
}

void GpuParticleManager::Emit(const Vector3& position, uint32_t count, float radius) {
    (void)count;
    (void)radius;
    Emit("EnemyDefeat", position);
}

void GpuParticleManager::ResetParticles() {
    for (auto& [name, group] : groups_) {
        if (!group) continue;
        group->isInitializedGpu = false;
        group->pendingEmits.clear();
    }
}

GpuParticleManager::EmitterConfigGPU GpuParticleManager::MakeGpuConfig(const GpuParticleGroup& group, const EmitRequest& req) const {
    const auto& src = group.config;
    EmitterConfigGPU dst{};
    dst.translate = req.position;
    dst.count = src.count;
    dst.emitRangeMin = src.emitRangeMin;
    dst.emitRangeMax = src.emitRangeMax;
    dst.lifeTimeMin = src.lifeTimeMin;
    dst.lifeTimeMax = src.lifeTimeMax;
    dst.startScaleMin = src.startScaleMin;
    dst.startScaleMax = src.startScaleMax;
    dst.endScaleMin = src.endScaleMin;
    dst.endScaleMax = src.endScaleMax;
    dst.rotateMin = src.rotateMin;
    dst.rotateMax = src.rotateMax;
    dst.velocityMin = src.velocityMin;
    dst.velocityMax = src.velocityMax;
    dst.startColorMin = src.startColorMin;
    dst.startColorMax = src.startColorMax;
    dst.endColorMin = src.endColorMin;
    dst.endColorMax = src.endColorMax;
    dst.drag = src.drag;
    dst.gravity = src.gravity;

    if (req.useDirection) {
        float xzLenSq = req.direction.x * req.direction.x + req.direction.z * req.direction.z;
        if (xzLenSq > 1e-6f) {
            float yaw = std::atan2(req.direction.x, req.direction.z);
            dst.rotateMin.y += yaw;
            dst.rotateMax.y += yaw;
            dst.velocityMin = RotateY(dst.velocityMin, yaw);
            dst.velocityMax = RotateY(dst.velocityMax, yaw);
            // 回転後に min > max となった成分を入れ替え
            if (dst.velocityMin.x > dst.velocityMax.x) std::swap(dst.velocityMin.x, dst.velocityMax.x);
            if (dst.velocityMin.z > dst.velocityMax.z) std::swap(dst.velocityMin.z, dst.velocityMax.z);
        }
    }

    return dst;
}

void GpuParticleManager::Update() {
    if (!isActive_) return;

    totalTime_ += DXCommon::kDeltaTime;
    if (perFrameData_) {
        perFrameData_->time = totalTime_;
        perFrameData_->deltaTime = DXCommon::kDeltaTime;
    }

    if (camera_) {
        Matrix4x4 backFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
        Matrix4x4 billboardMatrix = Multiply(backFrontMatrix, camera_->GetWorldMatrix());
        billboardMatrix.m[3][0] = 0.0f;
        billboardMatrix.m[3][1] = 0.0f;
        billboardMatrix.m[3][2] = 0.0f;
        Matrix4x4 viewProjection = Multiply(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());

        for (const auto& name : groupOrder_) {
            auto& group = groups_[name];
            if (!group || !group->perViewData) continue;
            group->perViewData->billboardMatrix = billboardMatrix;
            group->perViewData->viewProjection = viewProjection;
            group->perViewData->useBillboard = group->useBillboard ? 1u : 0u;
            group->perViewData->useVelocityStretch = group->useVelocityStretch ? 1u : 0u;
        }
    }
}

void GpuParticleManager::Draw() {
    if (!isActive_ || !camera_) return;

    auto commandList = DXCommon::GetInstance()->GetCommandList();
    auto srvManager = SrvManager::GetInstance();
    auto psoManager = PSOManager::GetInstance();

    for (const auto& name : groupOrder_) {
        auto it = groups_.find(name);
        if (it == groups_.end() || !it->second) continue;
        auto& group = *it->second;

        // 1. リソース状態遷移 (SRV/COMMON -> UAV)
        if (group.particleResourceState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
            std::vector<D3D12_RESOURCE_BARRIER> preBarriers;
            D3D12_RESOURCE_BARRIER pBarrier{};
            pBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            pBarrier.Transition.pResource = group.particleResource.Get();
            pBarrier.Transition.StateBefore = group.particleResourceState;
            pBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            pBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            preBarriers.push_back(pBarrier);

            if (!group.isFreeListBarrierInitialized) {
                ID3D12Resource* flResources[2] = { group.freeListIndexResource.Get(), group.freeListResource.Get() };
                for (int i = 0; i < 2; ++i) {
                    D3D12_RESOURCE_BARRIER flBarrier{};
                    flBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    flBarrier.Transition.pResource = flResources[i];
                    flBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
                    flBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    flBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    preBarriers.push_back(flBarrier);
                }
                group.isFreeListBarrierInitialized = true;
            }

            commandList->ResourceBarrier(static_cast<UINT>(preBarriers.size()), preBarriers.data());
            group.particleResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }

        auto insertUavBarriers = [&]() {
            D3D12_RESOURCE_BARRIER uavBarriers[3]{};
            uavBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarriers[0].UAV.pResource = group.particleResource.Get();
            uavBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarriers[1].UAV.pResource = group.freeListIndexResource.Get();
            uavBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarriers[2].UAV.pResource = group.freeListResource.Get();
            commandList->ResourceBarrier(3, uavBarriers);
        };

        // 2. InitializeParticle.CS
        if (!group.isInitializedGpu) {
            const auto& initPso = psoManager->GetPso("GpuParticleInitializeCS");
            commandList->SetComputeRootSignature(initPso.rootSignature.Get());
            commandList->SetPipelineState(initPso.pipelineState.Get());
            srvManager->SetComputeRootDescriptorTable(0, group.particleUavIndex);
            srvManager->SetComputeRootDescriptorTable(1, group.freeListIndexUavIndex);
            srvManager->SetComputeRootDescriptorTable(2, group.freeListUavIndex);
            commandList->Dispatch(1, 1, 1);
            insertUavBarriers();
            group.isInitializedGpu = true;
        }

        // 3. EmitParticle.CS
        if (!group.pendingEmits.empty()) {
            const auto& emitPso = psoManager->GetPso("GpuParticleEmitCS");
            for (size_t i = 0; i < group.pendingEmits.size(); ++i) {
                *group.emitRequestData[i] = MakeGpuConfig(group, group.pendingEmits[i]);

                commandList->SetComputeRootSignature(emitPso.rootSignature.Get());
                commandList->SetPipelineState(emitPso.pipelineState.Get());
                srvManager->SetComputeRootDescriptorTable(0, group.particleUavIndex);
                srvManager->SetComputeRootDescriptorTable(1, group.freeListIndexUavIndex);
                srvManager->SetComputeRootDescriptorTable(2, group.freeListUavIndex);
                commandList->SetComputeRootConstantBufferView(3, group.emitRequestResources[i]->GetGPUVirtualAddress());
                commandList->SetComputeRootConstantBufferView(4, perFrameResource_->GetGPUVirtualAddress());
                commandList->Dispatch(1, 1, 1);
                insertUavBarriers();
            }
            group.pendingEmits.clear();
        }

        // 4. UpdateParticle.CS
        {
            const auto& updatePso = psoManager->GetPso("GpuParticleUpdateCS");
            commandList->SetComputeRootSignature(updatePso.rootSignature.Get());
            commandList->SetPipelineState(updatePso.pipelineState.Get());
            srvManager->SetComputeRootDescriptorTable(0, group.particleUavIndex);
            srvManager->SetComputeRootDescriptorTable(1, group.freeListIndexUavIndex);
            srvManager->SetComputeRootDescriptorTable(2, group.freeListUavIndex);
            commandList->SetComputeRootConstantBufferView(3, perFrameResource_->GetGPUVirtualAddress());
            commandList->Dispatch(1, 1, 1);

            D3D12_RESOURCE_BARRIER barriers[3]{};
            barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barriers[0].Transition.pResource = group.particleResource.Get();
            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
            barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            barriers[1].UAV.pResource = group.freeListIndexResource.Get();
            barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            barriers[2].UAV.pResource = group.freeListResource.Get();
            commandList->ResourceBarrier(3, barriers);
            group.particleResourceState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        }

        // 5. DrawInstanced
        {
            const auto& prim = primitiveResources_[group.effectType];
            const auto& drawPso = psoManager->GetPso("GpuParticle", group.blendMode);
            commandList->SetGraphicsRootSignature(drawPso.rootSignature.Get());
            commandList->SetPipelineState(drawPso.pipelineState.Get());
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->IASetVertexBuffers(0, 1, &prim.vbv);

            commandList->SetGraphicsRootConstantBufferView(0, group.perViewResource->GetGPUVirtualAddress());
            srvManager->SetGraphicsRootDescriptorTable(1, group.particleSrvIndex);
            srvManager->SetGraphicsRootDescriptorTable(2, group.textureSrvIndex);

            commandList->DrawInstanced(prim.vertexCount, kMaxParticles, 0, 0);
        }
    }
}
