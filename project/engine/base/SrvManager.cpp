#include "SrvManager.h"
#include "DXCommon.h"

#include "DirectXTex.h"

std::unique_ptr<SrvManager> SrvManager::instance = nullptr;
const uint32_t SrvManager::kMaxSRVCount = 512;
// インスタンス取得の実装
SrvManager *SrvManager::GetInstance() {
  if (instance == nullptr) {
    // privateコンストラクタを呼び出せるヘルパー構造体
    struct Helper : public SrvManager {
      Helper() : SrvManager() {}
    };
    instance = std::make_unique<Helper>();
  }
  return instance.get();
} // 静的メンバ変数の初期化
void SrvManager::Initialize() {

  descriptorHeap_ = DXCommon::GetInstance()->CreateDescriptorHeap(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);
  descriptorSize_ =
      DXCommon::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}
void SrvManager::Finalize() {
  // インスタンスを破棄（デストラクタが呼ばれ、ComPtrも解放される）
  instance.reset();
}
void SrvManager::PreDraw() {
  ID3D12DescriptorHeap *descritptorHeaps[] = {descriptorHeap_.Get()};
  DXCommon::GetInstance()->GetCommandList()->SetDescriptorHeaps(
      1, descritptorHeaps);
}
bool SrvManager::IsMax() {
  if (useIndex > kMaxSRVCount) {
    return false;
  }
  return true;
}
uint32_t SrvManager::AllocateSRV() {
  assert(useIndex < kMaxSRVCount);
  int index = useIndex;
  useIndex++;
  return index;
}
D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index) {
  D3D12_CPU_DESCRIPTOR_HANDLE handleCPU =
      descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
  handleCPU.ptr += (descriptorSize_ * index);
  return handleCPU;
}
D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index) {
  D3D12_GPU_DESCRIPTOR_HANDLE handleGPU =
      descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
  handleGPU.ptr += (descriptorSize_ * index);
  return handleGPU;
}

void SrvManager::CreateSRVForTexture2D(uint32_t srvIndex,
                                       ID3D12Resource *pResource,
                                       DirectX::TexMetadata metadata) {
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = metadata.format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

  if (metadata.IsCubemap()) {
    // キューブマップの変換
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE; // キューブマップ
    srvDesc.TextureCube.MostDetailedMip = 0; // 最初のミップマップ
    srvDesc.TextureCube.MipLevels =
        UINT(metadata.mipLevels); // 最初のミップマップ
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    // SRV

  } else {
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
    srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels); // 最初のミップマップ
    // SRV
  }
  DXCommon::GetInstance()->GetDevice()->CreateShaderResourceView(
      pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVForTextureDepth(uint32_t srvIndex,
                                          ID3D12Resource *pResource) {
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // 深度バッファはfloat
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
  srvDesc.Texture2D.MipLevels = 1; // 最初のミップマップ

  DXCommon::GetInstance()->GetDevice()->CreateShaderResourceView(
      pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVForRenderTarget(uint32_t srvIndex,
                                          ID3D12Resource *pResource,
                                          DXGI_FORMAT format) {
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = format;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension =
      D3D12_SRV_DIMENSION_TEXTURE2D; // レンダーテクスチャは通常2D
  srvDesc.Texture2D.MipLevels = 1;

  DXCommon::GetInstance()->GetDevice()->CreateShaderResourceView(
      pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVForStructuredBuffer(uint32_t srvIndex,
                                              ID3D12Resource *pResource,
                                              UINT numElements,
                                              UINT structureByteStride) {
  D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
  srvDesc.Format = DXGI_FORMAT_UNKNOWN;
  srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  srvDesc.Buffer.FirstElement = 0;
  srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
  srvDesc.Buffer.NumElements = numElements;
  srvDesc.Buffer.StructureByteStride = structureByteStride;
  D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU =
      GetCPUDescriptorHandle(srvIndex);
  D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU =
      GetGPUDescriptorHandle(srvIndex);
  DXCommon::GetInstance()->GetDevice()->CreateShaderResourceView(
      pResource, &srvDesc, instancingSrvHandleCPU);
}
void SrvManager::CreateSRVForMatrixPalette(
    ID3D12Resource *pResource, UINT numElements, UINT structureByteStride,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor) {
  D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
  paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
  paletteSrvDesc.Shader4ComponentMapping =
      D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
  paletteSrvDesc.Buffer.FirstElement = 0;
  paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
  paletteSrvDesc.Buffer.NumElements = numElements;
  paletteSrvDesc.Buffer.StructureByteStride = structureByteStride;
  DXCommon::GetInstance()->GetDevice()->CreateShaderResourceView(
      pResource, &paletteSrvDesc, cpuDescriptor);
}
void SrvManager::CreateUAVForStructuredBuffer(uint32_t uavIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride)
{

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = numElements;
    uavDesc.Buffer.StructureByteStride = structureByteStride;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    DXCommon::GetInstance()->GetDevice()->CreateUnorderedAccessView(
        pResource, nullptr, &uavDesc, GetCPUDescriptorHandle(uavIndex));

}
void SrvManager::SetComputeRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex){

    DXCommon::GetInstance()->GetCommandList()->SetComputeRootDescriptorTable(
        RootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}
void SrvManager::SetGraphicsRootDescriptorTable(UINT RootParameterIndex,
                                                uint32_t srvIndex) {
  DXCommon::GetInstance()->GetCommandList()->SetGraphicsRootDescriptorTable(
      RootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}
