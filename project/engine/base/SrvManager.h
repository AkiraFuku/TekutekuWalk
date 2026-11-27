#pragma once
#include <DirectXTex.h>
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <wrl.h>
#include "Singleton.h"

class SrvManager : public Singleton<SrvManager> {
public:
  friend class Singleton<SrvManager>;
  friend struct std::default_delete<SrvManager>;
  // 最大テクスチャ数
  static const uint32_t kMaxSRVCount;
  void Initialize();

  //
  uint32_t AllocateSRV();

  /// <summary>
  /// SRVのCPUディスクリプタハンドルを取得
  /// </summary>
  /// <param name="index"></param>
  /// <returns></returns>
  D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
  /// <summary>
  /// SRVのGPUディスクリプタハンドルを取得
  /// </summary>
  /// <param name="index"></param>
  /// <returns></returns>
  D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

  /// <summary>
  /// 2DテクスチャのSRVを作成する
  /// </summary>
  /// <param name="srvIndex"></param>
  /// <param name="pResource"></param>
  /// <param name="metadata"></param>
  void CreateSRVForTexture2D(uint32_t srvIndex, ID3D12Resource *pResource,
                             DirectX::TexMetadata metadata);

  /// <summary>
  /// 深度バッファのSRVを作成する
  /// </summary>
  /// <param name="srvIndex"></param>
  /// <param name="pResource"></param>
  // 深度ベースのアウトライン描画に必要
  void CreateSRVForTextureDepth(uint32_t srvIndex, ID3D12Resource *pResource);
  /// <summary>
  ///
  /// </summary>
  /// <param name="srvIndex"></param>
  /// <param name="pResource"></param>
  /// <param name="format"></param>
  void CreateSRVForRenderTarget(uint32_t srvIndex, ID3D12Resource *pResource,
                                DXGI_FORMAT format);

  /// <summary>
  ///
  /// </summary>
  /// <param name="srvIndex"></param>
  /// <param name="pResource"></param>
  /// <param name="numElements"></param>
  /// <param name="structureByteStride"></param>
  void CreateSRVForStructuredBuffer(uint32_t srvIndex,
                                    ID3D12Resource *pResource, UINT numElements,
                                    UINT structureByteStride);

  void CreateUAVForStructuredBuffer(uint32_t uavIndex,
                                    ID3D12Resource *pResource, UINT numElements,
                                    UINT structureByteStride);
  /// <summary>
  /// 行列パレットのSRVを作成する
  /// </summary>
  /// <param name="pResource"></param>
  /// <param name="numElements"></param>
  /// <param name="structureByteStride"></param>
  /// <param name="cpuDescriptor"></param>
  void CreateSRVForMatrixPalette(ID3D12Resource *pResource, UINT numElements,
                                 UINT structureByteStride,
                                 D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor);

  void SetGraphicsRootDescriptorTable(UINT RootParameterIndex,
                                      uint32_t srvIndex);
  void SetComputeRootDescriptorTable(UINT RootParameterIndex,
                                     uint32_t srvIndex);
  void PreDraw();
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() {
    return descriptorHeap_;
  }

  bool IsMax();

protected:
  SrvManager() = default;
  ~SrvManager() override = default;

private:
  uint32_t descriptorSize_;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
  uint32_t useIndex = 0;
};
