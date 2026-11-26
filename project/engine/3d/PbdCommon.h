#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include "Camera.h"
#include "CameraManager.h"
#include "Singleton.h"

class PbdCommon : public Singleton<PbdCommon> {
    friend class Singleton<PbdCommon>;
    friend struct std::default_delete<PbdCommon>;
public:
    void Initialize();

    void PbdCommonDraw();

    void SetDefaultCamera(Camera* camera) {
        if (camera) {
            CameraManager::GetInstance()->RegisterCamera("Default", camera);
            CameraManager::GetInstance()->SetActiveGameCamera("Default");
        }
    }

    Camera* GetDefaultCamera() const {
        return CameraManager::GetInstance()->GetActiveGameCamera();
    }

    ID3D12RootSignature* GetRootSignature() const {
        return rootSignature_.Get();
    }

    ID3D12PipelineState* GetPipelineState() const {
        return graphicsPipelineState_.Get();
    }

protected:
    PbdCommon() = default;
    ~PbdCommon() override = default;

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
};
