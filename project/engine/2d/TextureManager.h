#pragma once
#include <wrl.h>
#include <string>
#include <unordered_map>
#include<d3d12.h>
#include"DirectXTex.h"
#include"d3dx12.h"
#include "Singleton.h"

class DXCommon;

class TextureManager : public Singleton<TextureManager>
{
    friend class Singleton<TextureManager>;
    friend struct std::default_delete<TextureManager>;
private:

    //テクスチャデータ構造体
    struct TextureData
    {
        uint32_t srvIndex;
        DirectX::TexMetadata metadata;
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
        Microsoft::WRL::ComPtr<ID3D12Resource>intermediateResource;
    };

    std::unordered_map<std::string, TextureData> textureDates;

    
    

    static uint32_t kSRVIndexTop;
protected:
    TextureManager() = default;
    ~TextureManager() override = default;
public:
    void Initialize();
    //テクスチャ読込・登録
    void LoadTexture(const std::string& filePath);
    //中間リソース解放
    void ReleaseIntermediateResources();
    //SRVIndex開始番号取得
    uint32_t GetTextureIndexByFilePath(const std::string& filePath);
    //テクスチャ番号からGPUハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureindex);
    //メタデータ
    const DirectX::TexMetadata& GetMetaData(const std::string& filePath);

public: // 外部入出力

    // ----- Getter -----
    const DirectX::Image GetImage(const std::string& filePath) const;


};

