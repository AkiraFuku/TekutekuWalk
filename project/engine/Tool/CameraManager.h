#pragma once
#include "Camera.h"
#include <unordered_map>
#include <string>
#include <memory>

class CameraManager
{
public:
    // シングルトン取得
    static CameraManager* GetInstance();

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

    // 終了処理
    void Finalize();

private:
    // シングルトンパターン
    CameraManager() = default;
    ~CameraManager() = default;
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    static std::unique_ptr<CameraManager> instance_;

    // カメラ管理
    std::unordered_map<std::string, Camera*> cameras_;
    std::string activeGameCameraName_;
    std::string activeEditorCameraName_;
    bool isEditorMode_ = false;
};
