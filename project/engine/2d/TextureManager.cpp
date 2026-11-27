#include "TextureManager.h"
#include "DXCommon.h"
#include "StringUtility.h"
#include "SrvManager.h"
uint32_t TextureManager::kSRVIndexTop = 1;

void TextureManager::Initialize() {

    textureDates.reserve(SrvManager::kMaxSRVCount);

}

void TextureManager::LoadTexture(const std::string& filePath) {

    if (textureDates.contains(filePath)) {
        return;
    }
    assert(SrvManager::GetInstance()->IsMax());


    //テクスチャの読み込み
    DirectX::ScratchImage image{};
    std::wstring filePathW = StringUtility::ConvertString(filePath);
   
    HRESULT hr;
    if (filePathW.ends_with(L".dds"))
    {
        //キューブマップの読み込み
        hr = DirectX::LoadFromDDSFile(
            filePathW.c_str(),
            DirectX::DDS_FLAGS_NONE,
            nullptr,
            image
        );
    } else {
        hr = DirectX::LoadFromWICFile(
            filePathW.c_str(),
            DirectX::WIC_FLAGS_FORCE_SRGB,
            nullptr,
            image
        );

    }
    assert(SUCCEEDED(hr));

    //ミップマップの生成
    DirectX::ScratchImage mipImages{};

    if (DirectX::IsCompressed(image.GetMetadata().format))
    {
        //キューブマップ
        mipImages = std::move(image);
    } else {
        hr = DirectX::GenerateMipMaps(
            image.GetImages(),
            image.GetImageCount(),
            image.GetMetadata(),
            DirectX::TEX_FILTER_SRGB,
            0,
            mipImages
        );
        assert(SUCCEEDED(hr));

    }
    //テクスチャデータ追加
    TextureData& textureData = textureDates[filePath];
    textureData.metadata = mipImages.GetMetadata();//メタデータ
    textureData.resource = DXCommon::GetInstance()->CreateTextureResourse(textureData.metadata);//テクスチャリソース
    //SRVインデックス
    textureData.srvIndex = SrvManager::GetInstance()->AllocateSRV();
    textureData.srvHandleCPU = SrvManager::GetInstance()->GetCPUDescriptorHandle(textureData.srvIndex);
    textureData.srvHandleGPU = SrvManager::GetInstance()->GetGPUDescriptorHandle(textureData.srvIndex);

    SrvManager::GetInstance()->CreateSRVForTexture2D(textureData.srvIndex, textureData.resource.Get(), textureData.metadata);
    textureData.intermediateResource = DXCommon::GetInstance()->UploadTextureData(textureData.resource, mipImages);

}
void TextureManager::ReleaseIntermediateResources()
{
    for (auto& [key, textureData] : textureDates) {
        if (textureData.intermediateResource) {
            // ComPtrのResetを呼び出してリソースを解放し、nullptrにする
            textureData.intermediateResource.Reset();
        }
    }
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
    if (textureDates.contains(filePath))
    {
        // 読み込み済みなら、保存されている srvIndex を返す
        return textureDates[filePath].srvIndex;
    }

    //見つからなかった場合、その場で読み込む
    LoadTexture(filePath);



    // 3. 読み込んだ後のデータから srvIndex を返す
    return textureDates[filePath].srvIndex;

}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureindex)
{

    return SrvManager::GetInstance()->GetGPUDescriptorHandle(textureindex);
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath)
{
    // 指定されたファイルパスのテクスチャが読み込まれているかチェック
   // 読み込まれていなければアサートで停止（または LoadTexture(filePath) を呼ぶ設計も可）
    assert(textureDates.contains(filePath));

    // マップからデータを取得してメタデータを返す
    return textureDates[filePath].metadata;

}

const DirectX::Image TextureManager::GetImage(const std::string& filePath) const
{
    //テクスチャの読み込み
    DirectX::ScratchImage image{};
    std::wstring filePathW = StringUtility::ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(
        filePathW.c_str(),
        DirectX::WIC_FLAGS_FORCE_SRGB,
        nullptr,
        image

    );
    assert(SUCCEEDED(hr));
    //ミップマップの生成
    DirectX::ScratchImage mipImages{};
    hr = DirectX::GenerateMipMaps(
        image.GetImages(),
        image.GetImageCount(),
        image.GetMetadata(),
        DirectX::TEX_FILTER_SRGB,
        0,
        mipImages
    );
    assert(SUCCEEDED(hr));

    return *image.GetImage(0, 0, 0);
}
