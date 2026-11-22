#include "GpuParticleManager.h"
#include "PSOManager.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include <numbers>
#include <vector>
#include <cstring>

namespace {
size_t Align256(size_t size) {
    return (size + 255) & ~size_t(255);
}
}

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
    CreateResources();
    CreateVertexBuffer();

    // デフォルトテクスチャ設定
    SetTexture("resources/Efect.png");

    isInitializedGpu_ = false;
    totalTime_ = 0.0f;
}

void GpuParticleManager::Finalize() {
    if (perViewResource_) { perViewResource_->Unmap(0, nullptr); perViewData_ = nullptr; }
    if (perFrameResource_) { perFrameResource_->Unmap(0, nullptr); perFrameData_ = nullptr; }
    if (emitterResource_) { emitterResource_->Unmap(0, nullptr); emitterData_ = nullptr; }
    for (uint32_t i = 0; i < kMaxEmitRequestsPerFrame; ++i) {
        if (emitRequestResources_[i]) {
            emitRequestResources_[i]->Unmap(0, nullptr);
            emitRequestData_[i] = nullptr;
            emitRequestResources_[i].Reset();
        }
    }

    particleResource_.Reset();
    freeListIndexResource_.Reset();
    freeListResource_.Reset();
    perViewResource_.Reset();
    perFrameResource_.Reset();
    emitterResource_.Reset();
    vertexResource_.Reset();

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

void GpuParticleManager::CreateResources() {
    auto dxCommon = DXCommon::GetInstance();
    auto srvManager = SrvManager::GetInstance();

    // 1. gParticles (DEFAULT + UAV + SRV)
    particleResource_ = dxCommon->CreateUAVBufferResource(sizeof(ParticleCS) * kMaxParticles);
    particleUavIndex_ = srvManager->AllocateSRV();
    srvManager->CreateUAVForStructuredBuffer(particleUavIndex_, particleResource_.Get(), kMaxParticles, sizeof(ParticleCS));
    particleSrvIndex_ = srvManager->AllocateSRV();
    srvManager->CreateSRVForStructuredBuffer(particleSrvIndex_, particleResource_.Get(), kMaxParticles, sizeof(ParticleCS));

    // 2. gFreeListIndex (DEFAULT + UAV)
    freeListIndexResource_ = dxCommon->CreateUAVBufferResource(sizeof(int32_t));
    freeListIndexUavIndex_ = srvManager->AllocateSRV();
    srvManager->CreateUAVForStructuredBuffer(freeListIndexUavIndex_, freeListIndexResource_.Get(), 1, sizeof(int32_t));

    // 3. gFreeList (DEFAULT + UAV)
    freeListResource_ = dxCommon->CreateUAVBufferResource(sizeof(uint32_t) * kMaxParticles);
    freeListUavIndex_ = srvManager->AllocateSRV();
    srvManager->CreateUAVForStructuredBuffer(freeListUavIndex_, freeListResource_.Get(), kMaxParticles, sizeof(uint32_t));

    // 4. PerView CBV
    perViewResource_ = dxCommon->CreateBufferResource(Align256(sizeof(PerView)));
    perViewResource_->Map(0, nullptr, reinterpret_cast<void**>(&perViewData_));
    perViewData_->viewProjection = Makeidentity4x4();
    perViewData_->billboardMatrix = Makeidentity4x4();

    // 5. PerFrame CBV
    perFrameResource_ = dxCommon->CreateBufferResource(Align256(sizeof(PerFrame)));
    perFrameResource_->Map(0, nullptr, reinterpret_cast<void**>(&perFrameData_));
    perFrameData_->time = 0.0f;
    perFrameData_->deltaTime = DXCommon::kDeltaTime;

    // 6. EmitterSphere CBV
    emitterResource_ = dxCommon->CreateBufferResource(Align256(sizeof(EmitterSphere)));
    emitterResource_->Map(0, nullptr, reinterpret_cast<void**>(&emitterData_));
    emitterData_->translate = { 0.0f, 1.0f, 0.0f };
    emitterData_->radius = 1.0f;
    emitterData_->count = 10;
    emitterData_->frequency = 0.1f;
    emitterData_->frequencyTime = 0.0f;
    emitterData_->emit = 0;

    // 7. ワンショットEmitリクエスト用CBV配列
    for (uint32_t i = 0; i < kMaxEmitRequestsPerFrame; ++i) {
        emitRequestResources_[i] = dxCommon->CreateBufferResource(Align256(sizeof(EmitterSphere)));
        emitRequestResources_[i]->Map(0, nullptr, reinterpret_cast<void**>(&emitRequestData_[i]));
        *emitRequestData_[i] = {};
    }
}

void GpuParticleManager::CreateVertexBuffer() {
    std::vector<VertexData> vertices = {
        {{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
        {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
    };

    vertexCount_ = static_cast<uint32_t>(vertices.size());
    size_t sizeVB = sizeof(VertexData) * vertices.size();
    vertexResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeVB);
    vbv_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vbv_.SizeInBytes = static_cast<UINT>(sizeVB);
    vbv_.StrideInBytes = sizeof(VertexData);

    void* mapped = nullptr;
    vertexResource_->Map(0, nullptr, &mapped);
    std::memcpy(mapped, vertices.data(), sizeVB);
    vertexResource_->Unmap(0, nullptr);
}

void GpuParticleManager::SetTexture(const std::string& textureFilepath) {
    TextureManager::GetInstance()->LoadTexture(textureFilepath);
    textureSrvIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilepath);
}

void GpuParticleManager::SetEmitterPosition(const Vector3& pos) {
    if (emitterData_) emitterData_->translate = pos;
}

void GpuParticleManager::SetEmitterRadius(float radius) {
    if (emitterData_) emitterData_->radius = radius;
}

void GpuParticleManager::SetEmitterCount(uint32_t count) {
    if (emitterData_) emitterData_->count = count;
}

void GpuParticleManager::SetEmitterFrequency(float frequency) {
    if (emitterData_) emitterData_->frequency = frequency;
}

void GpuParticleManager::ResetParticles() {
    isInitializedGpu_ = false;
    pendingEmits_.clear();
}

void GpuParticleManager::Emit(const Vector3& position, uint32_t count, float radius) {
    if (!isActive_ || pendingEmits_.size() >= kMaxEmitRequestsPerFrame) {
        return;
    }
    pendingEmits_.push_back({ position, radius, count });
}

void GpuParticleManager::Update() {
    if (!isActive_) return;

    totalTime_ += DXCommon::kDeltaTime;
    perFrameData_->time = totalTime_;
    perFrameData_->deltaTime = DXCommon::kDeltaTime;

    if (autoEmit_) {
        emitterData_->frequencyTime += DXCommon::kDeltaTime;
        if (emitterData_->frequency <= emitterData_->frequencyTime) {
            emitterData_->frequencyTime -= emitterData_->frequency;
            emitterData_->emit = 1;
        } else {
            emitterData_->emit = 0;
        }
    } else {
        emitterData_->emit = 0;
    }

    if (camera_) {
        Matrix4x4 backFrontMatrix = MakeRotateYMatrix(std::numbers::pi_v<float>);
        Matrix4x4 billboardMatrix = Multiply(backFrontMatrix, camera_->GetWorldMatrix());
        billboardMatrix.m[3][0] = 0.0f;
        billboardMatrix.m[3][1] = 0.0f;
        billboardMatrix.m[3][2] = 0.0f;

        Matrix4x4 viewProjection = Multiply(camera_->GetViewMatrix(), camera_->GetProjectionMatrix());
        perViewData_->billboardMatrix = billboardMatrix;
        perViewData_->viewProjection = viewProjection;
    }
}

void GpuParticleManager::Draw() {
    if (!isActive_ || !camera_) return;

    auto commandList = DXCommon::GetInstance()->GetCommandList();
    auto srvManager = SrvManager::GetInstance();
    auto psoManager = PSOManager::GetInstance();

    // 1. リソース状態遷移
    if (particleResourceState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
        std::vector<D3D12_RESOURCE_BARRIER> preBarriers;
        D3D12_RESOURCE_BARRIER pBarrier{};
        pBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        pBarrier.Transition.pResource = particleResource_.Get();
        pBarrier.Transition.StateBefore = particleResourceState_;
        pBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        pBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        preBarriers.push_back(pBarrier);

        if (!isFreeListBarrierInitialized_) {
            ID3D12Resource* flResources[2] = { freeListIndexResource_.Get(), freeListResource_.Get() };
            for (int i = 0; i < 2; ++i) {
                D3D12_RESOURCE_BARRIER flBarrier{};
                flBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                flBarrier.Transition.pResource = flResources[i];
                flBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
                flBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                flBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                preBarriers.push_back(flBarrier);
            }
            isFreeListBarrierInitialized_ = true;
        }

        commandList->ResourceBarrier(static_cast<UINT>(preBarriers.size()), preBarriers.data());
        particleResourceState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }

    if (!isInitializedGpu_) {
        const auto& initPso = psoManager->GetPso("GpuParticleInitializeCS");
        commandList->SetComputeRootSignature(initPso.rootSignature.Get());
        commandList->SetPipelineState(initPso.pipelineState.Get());
        srvManager->SetComputeRootDescriptorTable(0, particleUavIndex_);
        srvManager->SetComputeRootDescriptorTable(1, freeListIndexUavIndex_);
        srvManager->SetComputeRootDescriptorTable(2, freeListUavIndex_);
        commandList->Dispatch(1, 1, 1);

        D3D12_RESOURCE_BARRIER uavBarriers[3]{};
        uavBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarriers[0].UAV.pResource = particleResource_.Get();
        uavBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarriers[1].UAV.pResource = freeListIndexResource_.Get();
        uavBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        uavBarriers[2].UAV.pResource = freeListResource_.Get();
        commandList->ResourceBarrier(3, uavBarriers);

        isInitializedGpu_ = true;
    }

    // 2. EmitParticle.CS
    {
        const auto& emitPso = psoManager->GetPso("GpuParticleEmitCS");
        auto dispatchEmit = [&](ID3D12Resource* cbvResource) {
            commandList->SetComputeRootSignature(emitPso.rootSignature.Get());
            commandList->SetPipelineState(emitPso.pipelineState.Get());
            srvManager->SetComputeRootDescriptorTable(0, particleUavIndex_);
            srvManager->SetComputeRootDescriptorTable(1, freeListIndexUavIndex_);
            srvManager->SetComputeRootDescriptorTable(2, freeListUavIndex_);
            commandList->SetComputeRootConstantBufferView(3, cbvResource->GetGPUVirtualAddress());
            commandList->SetComputeRootConstantBufferView(4, perFrameResource_->GetGPUVirtualAddress());
            commandList->Dispatch(1, 1, 1);

            D3D12_RESOURCE_BARRIER uavBarriers[3]{};
            uavBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarriers[0].UAV.pResource = particleResource_.Get();
            uavBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarriers[1].UAV.pResource = freeListIndexResource_.Get();
            uavBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
            uavBarriers[2].UAV.pResource = freeListResource_.Get();
            commandList->ResourceBarrier(3, uavBarriers);
        };

        if (autoEmit_ && emitterData_->emit != 0) {
            dispatchEmit(emitterResource_.Get());
        }

        for (size_t i = 0; i < pendingEmits_.size(); ++i) {
            const auto& req = pendingEmits_[i];
            emitRequestData_[i]->translate = req.translate;
            emitRequestData_[i]->radius = req.radius;
            emitRequestData_[i]->count = req.count;
            emitRequestData_[i]->frequency = 0.0f;
            emitRequestData_[i]->frequencyTime = 0.0f;
            emitRequestData_[i]->emit = 1;
            dispatchEmit(emitRequestResources_[i].Get());
        }
        pendingEmits_.clear();
    }

    // 3. UpdateParticle.CS
    {
        const auto& updatePso = psoManager->GetPso("GpuParticleUpdateCS");
        commandList->SetComputeRootSignature(updatePso.rootSignature.Get());
        commandList->SetPipelineState(updatePso.pipelineState.Get());
        srvManager->SetComputeRootDescriptorTable(0, particleUavIndex_);
        srvManager->SetComputeRootDescriptorTable(1, freeListIndexUavIndex_);
        srvManager->SetComputeRootDescriptorTable(2, freeListUavIndex_);
        commandList->SetComputeRootConstantBufferView(3, perFrameResource_->GetGPUVirtualAddress());
        commandList->Dispatch(1, 1, 1);

        // UAV -> SRV (VertexShader参照用) 遷移
        D3D12_RESOURCE_BARRIER barriers[3]{};
        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[0].Transition.pResource = particleResource_.Get();
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[1].UAV.pResource = freeListIndexResource_.Get();
        barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[2].UAV.pResource = freeListResource_.Get();
        commandList->ResourceBarrier(3, barriers);
        particleResourceState_ = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }

    // 4. DrawInstanced
    {
        const auto& drawPso = psoManager->GetPso("GpuParticle", BlendMode::Add);
        commandList->SetGraphicsRootSignature(drawPso.rootSignature.Get());
        commandList->SetPipelineState(drawPso.pipelineState.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &vbv_);

        commandList->SetGraphicsRootConstantBufferView(0, perViewResource_->GetGPUVirtualAddress());
        srvManager->SetGraphicsRootDescriptorTable(1, particleSrvIndex_);
        srvManager->SetGraphicsRootDescriptorTable(2, textureSrvIndex_);

        commandList->DrawInstanced(vertexCount_, kMaxParticles, 0, 0);
    }
}
