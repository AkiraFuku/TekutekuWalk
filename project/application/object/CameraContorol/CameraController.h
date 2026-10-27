#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include <functional>
#include <memory>
#include <string>

class Object3d;
struct Move {
	Vector2 max;
	Vector2 min;
};
class Camera;
class Player;
class RailMover;
class RailPath;

/// <summary>
/// カメラの設定パラメータ・オフセットを保持するデータ構造体
/// </summary>
struct CameraConfig {
	int mode = 0; ///< 0: AutoOffset, 1: RailCamera
	struct AutoOffset {
		float distance = 8.5f;      ///< レールからの水平外側オフセット距離
		float height = 2.8f;        ///< レールからの高さオフセット
		float lookAtHeight = 1.2f;  ///< 注視点の高さオフセット
		float smoothSpeed = 0.15f;  ///< カメラ位置の追従補間率
		bool flipSide = false;      ///< オフセット向き反転
	} autoOffset;

	struct Rendering {
		float drawDistance = 80.0f; ///< 描画距離範囲 (FarClip)
	} rendering;

	struct ClearPhase {
		Vector3 targetOffsetGoal = { 0.0f, 0.0f, -6.0f };
		Vector3 clearOffset = { 0.0f, 1.0f, -5.0f };
	} clearPhase;
};

class CameraController {
public:
	enum class CameraMode {
		AutoOffset, ///< レール形状から外側・高さオフセットを自動計算追従（推奨）
		RailCamera, ///< 既存のCameraRailに沿って追従
	};

	struct Rect {
		float left = 0.0f;   // 左端
		float right = 1.0f;  // 右端
		float bottom = 0.0f; // 下端
		float top = 1.0f;    // 上端
	};

    CameraController() ;
    ~CameraController();
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(Camera* camera);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	// JSON設定の読み込み・保存
	void LoadConfig(const std::string& filePath = "resources/config/camera.json");
	void SaveConfig(const std::string& filePath = "resources/config/camera.json");
	const CameraConfig& GetConfig() const { return config_; }
	CameraConfig& GetConfig() { return config_; }

	void SetTarget(Player* target) { target_ = target; }
	void SetMoveArea(const Rect area) { moveArea_ = area; }

    void SetRailPath(const RailPath* path) ;
    void SetStageRail(const RailPath* path) { stageRail_ = path; }
    void SetRailProgress(float progress);
    
    // カメラモード・オフセット設定
    void SetCameraMode(CameraMode mode) { 
		mode_ = mode; 
		config_.mode = (mode == CameraMode::AutoOffset) ? 0 : 1;
	}
    CameraMode GetCameraMode() const { return mode_; }
    void SetAutoOffsetParams(float distance, float height, bool flipSide = false) {
        config_.autoOffset.distance = distance;
        config_.autoOffset.height = height;
        config_.autoOffset.flipSide = flipSide;
    }
    void SetDrawDistance(float distance);
    float GetDrawDistance() const { return config_.rendering.drawDistance; }
    Camera* GetCamera() const { return camera_; }

	void Reset();
	// シェイクを要求する関数（デフォルトで二次関数的な減衰イージングを指定）
    void RequestShake(
        float duration, 
        float power, 
        std::function<float(float)> easingFunc = [](float t) { return (1.0f - t) * (1.0f - t); } // EaseOutQuad のような減衰
    );
    void ShakeCamera(float duration = 0.1f, float power = 1.0f) {
        RequestShake(duration, power);
    }
	void SetClearOffset() { 
        targetOffsetGoal_ = config_.clearPhase.targetOffsetGoal; 
    }
	void TriggerClearFocus() { isClearPhase_ = true; }
private:
	// カメラ
	Camera* camera_ = nullptr;
    const RailPath* stageRail_ = nullptr; ///< ステージレール（自動オフセット計算用）
    CameraMode mode_ = CameraMode::AutoOffset; ///< デフォルトは自動オフセット計算カメラ
	CameraConfig config_;

	Player* target_ = nullptr;
    Object3d* target = nullptr;
    std::unique_ptr<RailMover> railMover_;
	Rect moveArea_ = {0.0f, 100.0f, 0.0f, 100.0f};
	Vector3 targetOffset_ = {0.0f, 0.0f, -15.0f};
	Vector3 targetOffsetGoal_ = {0.0f, 0.0f, -15.0f};
	bool isClearPhase_ = false;
	Vector3 desetination_;
	static inline const float kInterpolationRate = 0.1f;                 //
	static inline const float kVelocityBias = 30.0f;                     //
	static inline const Rect targetMargin_ = {-9.0f, 9.0f, -5.0f, 5.0f}; // ターゲットのマージン

    void RotateCamera();
    void RailCamera();
    void AutoOffsetCamera();
    // シェイク用変数
    float shakeTimer_ = 0.0f;
    float shakeDuration_ = 0.0f; // 追加: シェイク開始時のトータル時間を記録
    float shakePower_ = 0.0f;
    Vector3 shakeOffset_ = { 0.0f, 0.0f, 0.0f };

    // 追加: イージング関数を保存する変数
    std::function<float(float)> shakeEasing_ = nullptr;
    void UpdateShake();
};


