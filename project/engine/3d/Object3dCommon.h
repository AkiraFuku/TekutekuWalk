#pragma once
#include "d3d12.h"
#include <wrl/client.h>
#include <cstdint>
#include "DXCommon.h"
#include "Camera.h"
#include "CameraManager.h"
#include "SkyBox.h"
class Object3dCommon
{
public:

    // シングルトン化
    static Object3dCommon* GetInstance();
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

    static std::unique_ptr<Object3dCommon> instance;
     friend struct std::default_delete<Object3dCommon>;
private:

    float globalCurvature_ = 0.0f; // グローバル空間湾曲率 (例: 0.001f)

    // シングルトンパターン
    Object3dCommon() = default;
    ~Object3dCommon() = default;
    Object3dCommon(const Object3dCommon&) = delete;
    Object3dCommon& operator=(const Object3dCommon&) = delete;

    HRESULT hr_;
    SkyBox* defaultBox_=nullptr;


    //ルートシグネチャ
    Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature_;
    void CreateRootSignature();
    //グラフィックパイプラインステート
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
    void CreatePSO();
};

