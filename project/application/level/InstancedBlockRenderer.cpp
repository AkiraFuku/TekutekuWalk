#include "InstancedBlockRenderer.h"
#include "StageBlock.h"
#include "Camera.h"
#include "Model.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "SrvManager.h"
#include "PSOManager.h"
#include "DXCommon.h"
#include "MathFunction.h"
#include <cassert>

InstancedBlockRenderer* InstancedBlockRenderer::GetInstance()
{
    static InstancedBlockRenderer instance;
    return &instance;
}

void InstancedBlockRenderer::Initialize()
{
    if (isInitialized_) return;
    isInitialized_ = true;

    // インスタンスデータ用 StructuredBuffer の作成
    size_t bufferSize = sizeof(BlockInstanceDataGPU) * kMaxInstancesPerBatch;
    instancingResource_ = DXCommon::GetInstance()->CreateBufferResource(bufferSize);
    instancingResource_->Map(0, nullptr, reinterpret_cast<void**>(&instancingDataMapped_));

    // SRV の割り当て
    instancingSrvIndex_ = SrvManager::GetInstance()->AllocateSRV();
    SrvManager::GetInstance()->CreateSRVForStructuredBuffer(
        instancingSrvIndex_,
        instancingResource_.Get(),
        kMaxInstancesPerBatch,
        sizeof(BlockInstanceDataGPU)
    );
}

void InstancedBlockRenderer::Finalize()
{
    if (!isInitialized_) return;
    if (instancingResource_) {
        instancingResource_->Unmap(0, nullptr);
        instancingResource_.Reset();
    }
    instancingDataMapped_ = nullptr;
    isInitialized_ = false;
}

void InstancedBlockRenderer::Begin()
{
    batches_.clear();
}

void InstancedBlockRenderer::AddBlock(StageBlock* block, Camera* camera)
{
    if (!block || block->IsDisabled()) return;

    Object3d* obj = block->GetObject();
    if (!obj) return;

    // モデルとテクスチャ情報を取得
    std::string modelName = "box";
    std::string texturePath = block->GetTexturePath().empty() ? "resources/checker/checker_gray.png" : block->GetTexturePath();

    // プレハブ定義またはブロック情報からキーを決定
    if (!block->GetPrefabId().empty()) {
        modelName = block->GetPrefabId();
    }

    BatchKey key{ modelName, texturePath };
    auto it = batches_.find(key);
    if (it == batches_.end()) {
        Batch newBatch;
        newBatch.key = key;
        newBatch.model = ModelManager::GetInstance()->findModel("box").get();
        newBatch.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(texturePath);
        it = batches_.emplace(key, std::move(newBatch)).first;
    }

    Batch& batch = it->second;
    if (batch.instances.size() >= kMaxInstancesPerBatch) {
        return; // バッファ上限ガード
    }

    // WVP およびワールド行列の計算
    Matrix4x4 world = obj->GetWorldMatrix();
    Matrix4x4 wvp = world;
    if (camera) {
        wvp = Multiply(world, camera->GetViewProtectionMatrix());
    }

    BlockInstanceDataGPU instData;
    instData.WVP = wvp;
    instData.World = world;
    instData.WorldInverseTranspose = Transpose(Inverse(world));
    instData.color = { 1.0f, 1.0f, 1.0f, 1.0f };

    batch.instances.push_back(instData);
}

void InstancedBlockRenderer::Render(Camera* camera)
{
    if (!isEnabled_ || batches_.empty() || !instancingDataMapped_) return;

    auto commandList = DXCommon::GetInstance()->GetCommandList();
    if (!commandList) return;

    // インスタンシング用 PSO の取得
    PsoSet pso = PSOManager::GetInstance()->GetPso("InstancedObject3d", BlendMode::Normal);
    if (!pso.pipelineState || !pso.rootSignature) return;

    commandList->SetGraphicsRootSignature(pso.rootSignature.Get());
    commandList->SetPipelineState(pso.pipelineState.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // インスタンシングSRV (t5: ルートインデックス 1)
    commandList->SetGraphicsRootDescriptorTable(
        1,
        SrvManager::GetInstance()->GetGPUDescriptorHandle(instancingSrvIndex_)
    );

    for (auto& [key, batch] : batches_) {
        if (batch.instances.empty() || !batch.model) continue;

        uint32_t count = static_cast<uint32_t>(batch.instances.size());
        // インスタンスデータを GPU マップ領域へ一括転送
        memcpy(instancingDataMapped_, batch.instances.data(), sizeof(BlockInstanceDataGPU) * count);

        // テクスチャ設定 (t0: ルートインデックス 2)
        commandList->SetGraphicsRootDescriptorTable(
            2,
            SrvManager::GetInstance()->GetGPUDescriptorHandle(batch.textureIndex)
        );

        // モデルの描画（単一のDrawIndexedInstancedにより一括描画）
        batch.model->DrawInstanced(count);
    }
}
