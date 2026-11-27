#pragma once
#include "Camera.h"
#include <unordered_map>
#include <string>
#include <memory>
#include "Singleton.h"

class CameraManager : public Singleton<CameraManager>
{
public:
    friend class Singleton<CameraManager>;
    friend struct std::default_delete<CameraManager>;
    ~CameraManager() override = default;

    // カメラの登録・削除
    void RegisterCamera(const std::string& name, Camera* camera);
    void UnregisterCamera(const std::string& name);
    Camera* GetCamera(const std::string& name) const;

    // ゲームカメラの管理
    void SetActiveGameCamera(const std::string& name);
    Camera* GetActiveGameCamera() const;

    // エディタカメラの管理
    void SetActiveEditorCamera(const std::string& name);
    Camera* GetActiveEditorCamera() const;

    // 現在アクティブなカメラ取得（エディタモード時はエディタカメラ優先）
    Camera* GetActiveCamera() const;

    // モード切り替え
    bool IsEditorMode() const { return isEditorMode_; }
    void SetEditorMode(bool enable) { isEditorMode_ = enable; }

protected:
    CameraManager() = default;

private:
    // カメラ管理
    std::unordered_map<std::string, Camera*> cameras_;
    std::string activeGameCameraName_;
    std::string activeEditorCameraName_;
    bool isEditorMode_ = false;
};
