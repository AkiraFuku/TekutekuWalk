#pragma once
#include "d3d12.h"
#include <wrl/client.h>
#include <cstdint>
#include "DXCommon.h"
#include "Camera.h"
#include "CameraManager.h"
#include "SkyBox.h"
#include "Singleton.h"
class Object3dCommon : public Singleton<Object3dCommon>
{
    friend class Singleton<Object3dCommon>;
    friend struct std::default_delete<Object3dCommon>;
public:

    void Finalize();

    void Initialize();

    void Object3dCommonDraw();
    void SetDefaultCamera(Camera* camera) {
        if (camera) {
            CameraManager::GetInstance()->RegisterCamera("Default", camera);
            CameraManager::GetInstance()->SetActiveGameCamera("Default");
        }
    }
    void SetDefaultSkyBox(SkyBox* box) {
       defaultBox_=box;
    }
    Camera* GetDefaultCamera()const {
        return CameraManager::GetInstance()->GetActiveGameCamera();
    }
    SkyBox* GetDefaultSkyBox()const {
        return defaultBox_;
    }

    // 空間湾曲（Curved World）のグローバル設定
    void SetGlobalCurvature(float curvature) { globalCurvature_ = curvature; }
    float GetGlobalCurvature() const { return globalCurvature_; }

protected:
    Object3dCommon() = default;
    ~Object3dCommon() override = default;

private:

    float globalCurvature_ = 0.0f; // グローバル空間湾曲率 (例: 0.001f)

    HRESULT hr_;
    SkyBox* defaultBox_=nullptr;


    //ルートシグネチャ
    Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature_;
    void CreateRootSignature();
    //グラフィックパイプラインステート
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
    void CreatePSO();
};

