#include "CameraManager.h"

// 静的メンバ変数の初期化
std::unique_ptr<CameraManager> CameraManager::instance_ = nullptr;

CameraManager* CameraManager::GetInstance()
{
    if (instance_ == nullptr) {
        struct Helper : public CameraManager {
            Helper() : CameraManager() {}
        };
        instance_ = std::make_unique<Helper>();
    }
    return instance_.get();
}

void CameraManager::RegisterCamera(const std::string& name, Camera* camera)
{
    if (camera != nullptr) {
        cameras_[name] = camera;
    }
}

void CameraManager::UnregisterCamera(const std::string& name)
{
    auto it = cameras_.find(name);
    if (it != cameras_.end()) {
        cameras_.erase(it);
    }
}

Camera* CameraManager::GetCamera(const std::string& name) const
{
    auto it = cameras_.find(name);
    if (it != cameras_.end()) {
        return it->second;
    }
    return nullptr;
}

void CameraManager::SetActiveGameCamera(const std::string& name)
{
    if (cameras_.find(name) != cameras_.end()) {
        activeGameCameraName_ = name;
    }
}

Camera* CameraManager::GetActiveGameCamera() const
{
    if (!activeGameCameraName_.empty()) {
        auto it = cameras_.find(activeGameCameraName_);
        if (it != cameras_.end()) {
            return it->second;
        }
    }
    return nullptr;
}

void CameraManager::SetActiveEditorCamera(const std::string& name)
{
    if (cameras_.find(name) != cameras_.end()) {
        activeEditorCameraName_ = name;
    }
}

Camera* CameraManager::GetActiveEditorCamera() const
{
    if (!activeEditorCameraName_.empty()) {
        auto it = cameras_.find(activeEditorCameraName_);
        if (it != cameras_.end()) {
            return it->second;
        }
    }
    return nullptr;
}

Camera* CameraManager::GetActiveCamera() const
{
    // エディタモード有効時はエディタカメラ優先
    if (isEditorMode_) {
        Camera* editorCamera = GetActiveEditorCamera();
        if (editorCamera != nullptr) {
            return editorCamera;
        }
    }

    // ゲームカメラを返す
    return GetActiveGameCamera();
}

void CameraManager::Finalize()
{
    instance_.reset();
}
