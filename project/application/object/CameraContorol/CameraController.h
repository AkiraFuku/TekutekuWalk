#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Object3d;
struct Move {
	Vector2 max;
	Vector2 min;
};
class Camera;
class Player;
class Enemy;
class RailMover;
class RailPath;

/// <summary>
/// カメラの設定パラメータ・オフセットを保持するデータ構造体
/// </summary>
struct CameraConfig {
	int mode = 0; ///< 0: AutoOffset, 1: RailCamera
	struct AutoOffset {
		float distance = 7.5f;      ///< レールからの水平外側オフセット距離
		float height = 2.5f;        ///< レールからの高さオフセット
		float lookAtHeight = 1.15f; ///< 注視点の高さオフセット
		float smoothSpeed = 0.14f;  ///< カメラ位置の追従補間率
		bool flipSide = false;      ///< オフセット向き反転
	} autoOffset;

	struct Rendering {
		float drawDistance = 500.0f; ///< 描画距離範囲 (FarClip)
	} rendering;

	struct ClearPhase {
		Vector3 targetOffsetGoal = { 0.0f, 0.0f, -6.0f };
		Vector3 clearOffset = { 0.0f, 1.0f, -5.0f };
	} clearPhase;

	struct EnemyFraming {
		bool enabled = true;               ///< 近くの敵をカメラに収める機能の有効/無効
		float detectRange = 12.0f;         ///< 敵を検出するプレイヤーからの水平距離
		float detectRangeY = 10.0f;        ///< 敵を検出するプレイヤーからの垂直距離
		float blendStartRatio = 0.6f;      ///< 境界フェードを開始する距離割合 (0.0〜0.99)
		float focusWeight = 0.35f;         ///< 注視点を敵側へ寄せる割合 (水平方向)
		float focusWeightY = 0.5f;         ///< 注視点を敵側へ寄せる割合 (Y軸・高さ方向)
		float camYFollowRatio = 0.4f;      ///< 注視点Yシフトに対するカメラ高さの追従割合
		float minCamOffsetY = -0.5f;       ///< 敵が下にいる際のカメラYオフセット下限
		float extraHeightRatio = 0.25f;    ///< 追加ズーム距離に対するカメラ高さの連動割合
		float paddingHorizontal = 4.0f;    ///< 画面左右の余白マージン
		float paddingVertical = 3.0f;      ///< 画面上下の余白マージン
		float maxExtraDistance = 8.0f;     ///< 追加で引くカメラ距離の上限
		float focusSmoothSpeed = 0.03f;    ///< 注視点寄せの補間率 (XZ方向)
		float focusSmoothSpeedY = 0.10f;   ///< 注視点・カメラ高さの補間率 (Y方向)
		float zoomSmoothSpeed = 0.03f;     ///< ズーム距離の補間率
	} enemyFraming;
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
	void SetEnemies(const std::vector<std::unique_ptr<Enemy>>* enemies) { enemies_ = enemies; }
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
	const std::vector<std::unique_ptr<Enemy>>* enemies_ = nullptr;
    Object3d* target = nullptr;
    std::unique_ptr<RailMover> railMover_;
	Rect moveArea_ = {0.0f, 100.0f, 0.0f, 100.0f};
	Vector3 targetOffset_ = {0.0f, 0.0f, -15.0f};
	Vector3 targetOffsetGoal_ = {0.0f, 0.0f, -15.0f};
	bool isClearPhase_ = false;
	Vector3 currentFocusPos_ = {0.0f, 0.0f, 0.0f};
	bool isFocusInitialized_ = false;
	float smoothedFocusY_ = 0.0f;
	bool isYInitialized_ = false;
	float currentExtraDistance_ = 0.0f;
	Vector3 currentEnemyFocusOffset_ = {0.0f, 0.0f, 0.0f};
	float currentEnemyCamOffsetY_ = 0.0f;
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
