#pragma once
#include "d3d12.h"
#include <wrl/client.h>
#include <cstdint>

#include <memory>
#include "Singleton.h"

class SpriteCommon : public Singleton<SpriteCommon>
{
    friend class Singleton<SpriteCommon>;
    friend struct std::default_delete<SpriteCommon>;
public:
    void Initialize();

   
    void SpriteCommonDraw();

protected:
    SpriteCommon() = default;
    ~SpriteCommon() override = default;
private:
    HRESULT hr_;

    

    //ルートシグネチャ
    Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature_;
    //グラフィックパイプラインステート
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;


};

