#pragma once
#include "LevelData.h"
#include "LevelLoader.h"
#include "StageBlock.h"
#include "Terrain.h"
#include "EventTrigger.h"
#include "RailPath.h"
#include "PbdRope.h"
#include "PbdCloth.h"
#include "Camera.h"
#include "MathFunction.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

class Enemy;
class GameScene;

/// <summary>
/// レベルローダーが生成した LevelData を受け取り、
/// StageBlock / EventTrigger / PbdRope / PbdCloth / RailPath 等を
/// ゲームシーン上に実体化・管理するクラス。
///
/// GameScene::Initialize() から Load() を呼ぶだけで、
/// Blenderエディタで配置した全オブジェクトが自動セットアップされる。
/// ホットリロード（F5）にも対応。
/// </summary>
class StageManager
{
public:
    /// 敵スポーン要求用コールバック型
    using SpawnEnemyFunc = std::function<void(const LevelObjectData& data)>;

    StageManager() = default;
    ~StageManager() = default;

    /// <summary>
    /// JSON ファイルを読み込んで全オブジェクトを実体化する。
    /// 再呼び出しで既存の全オブジェクトをクリアしてリロード（Hot Reload 対応）。
    /// </summary>
    void Load(const std::string& jsonPath, Camera* camera, SpawnEnemyFunc spawnEnemy = nullptr);

    /// <summary>
    /// 全オブジェクトの毎フレーム更新。
    /// プレイヤー座標を渡すことでトリガー判定も行う。
    /// </summary>
    void Update(const Vector3& playerPos);

    /// <summary>全オブジェクトの描画</summary>
    void Draw();

    /// <summary>デバッグ描画（コライダー・トリガー領域・レールのワイヤーフレーム）</summary>
    void DebugDraw();

    // ─── ゲッター ─────────────────────────────────────────────────────

    /// 全地形・ブロックのワールド三角形（接地/壁判定用）をまとめて取得
    const std::vector<Triangle>& GetAllTriangles() const { return allTriangles_; }

    /// ステージ進行レールを取得（存在しなければ nullptr）
    RailPath* GetStageRail()  const { return stageRail_.get(); }

    /// カメラレールを取得（存在しなければ nullptr）
    RailPath* GetCameraRail() const { return cameraRail_.get(); }

    /// プレイヤー開始位置を取得（PLAYER_SPAWN が存在しなければゼロ）
    Vector3 GetPlayerSpawnPosition() const { return playerSpawnPos_; }
    Vector2 GetPlayerSpawnRailPosition() const { return playerSpawnRailPos_; }

    /// ゴール位置を取得（GOAL が存在しなければゼロ）
    Vector3 GetGoalPosition() const { return goalPos_; }
    Vector2 GetGoalRailPosition() const { return goalRailPos_; }

    /// イベントトリガーリストへの参照（外部からコールバックを設定するため）
    std::vector<std::unique_ptr<EventTrigger>>& GetTriggers() { return triggers_; }

    /// PBD ロープリストへの参照
    std::vector<std::unique_ptr<PbdRope>>& GetRopes() { return ropes_; }

    /// PBD 布リストへの参照
    std::vector<std::unique_ptr<PbdCloth>>& GetCloths() { return cloths_; }

    ///// コインリストへの参照
    //std::vector<std::unique_ptr<Coin>>& GetCoins() { return coins_; }
    //const std::vector<std::unique_ptr<Coin>>& GetCoins() const { return coins_; }

    /// ステージブロックリストへの参照
    std::vector<std::unique_ptr<StageBlock>>& GetBlocks() { return blocks_; }

    /// 地形（Terrain）を取得（存在しなければ nullptr）
    Terrain* GetTerrain() const { return terrain_.get(); }

    /// ステージ定義のカメラ設定
    float GetStageCameraDrawDistance() const { return stageCameraDrawDistance_; }
    const std::string& GetStageCameraMode() const { return stageCameraMode_; }
    float GetStageCameraDistance() const { return stageCameraDistance_; }
    float GetStageCameraHeight() const { return stageCameraHeight_; }

    /// デバッグ描画トグル
    void SetDebugDrawEnabled(bool enabled) { isDebugDrawEnabled_ = enabled; }
    bool IsDebugDrawEnabled() const { return isDebugDrawEnabled_; }

    /// 現在ロード中のファイルパス（ホットリロード用）
    const std::string& GetLoadedPath() const { return loadedPath_; }

private:
    /// LevelObjectData ツリーを再帰的に処理して各マネージャーへ配分する
    void ProcessObject(const LevelObjectData& data, Camera* camera, SpawnEnemyFunc& spawnEnemy);

    /// 全ブロック/地形から三角形リストを再構築する
    void RebuildTriangles();

    // ─── 管理リスト ──────────────────────────────────────────────────
    std::unique_ptr<Terrain>                  terrain_;  ///< kTerrain
    std::vector<std::unique_ptr<StageBlock>>   blocks_;   ///< kBlock
    std::vector<std::unique_ptr<StageBlock>>   props_;    ///< kProp（当たり判定なし装飾）
    std::vector<std::unique_ptr<EventTrigger>> triggers_; ///< kTrigger
    std::vector<std::unique_ptr<PbdRope>>      ropes_;    ///< kPbdRope
    std::vector<std::unique_ptr<PbdCloth>>     cloths_;   ///< kPbdCloth
    //std::vector<std::unique_ptr<Coin>>         coins_;    ///< kCoin

    // ─── レール ──────────────────────────────────────────────────────
    std::unique_ptr<RailPath> stageRail_;
    std::unique_ptr<RailPath> cameraRail_;

    // ─── 特殊位置マーカー ─────────────────────────────────────────────
    Vector3 playerSpawnPos_ = { 0.0f, 0.0f, 0.0f };
    Vector2 playerSpawnRailPos_ = { 0.0f, 0.0f };
    Vector3 goalPos_        = { 0.0f, 0.0f, 0.0f };
    Vector2 goalRailPos_    = { 0.0f, 0.0f };

    // ─── 接地判定三角形（全ブロック/地形から集約）─────────────────────
    std::vector<Triangle> allTriangles_;

    // ─── カメラ参照（ホットリロード時の再設定用）─────────────────────
    Camera* camera_ = nullptr;

    // ─── ロードパス（ホットリロード用）──────────────────────────────
    std::string loadedPath_;

    // ─── ステージ定義カメラ設定 ───────────────────────────────────────
    float stageCameraDrawDistance_ = 80.0f;
    std::string stageCameraMode_   = "AUTO_OFFSET";
    float stageCameraDistance_     = 25.0f;
    float stageCameraHeight_       = 5.0f;

    // ─── デバッグ描画フラグ ───────────────────────────────────────────
    bool isDebugDrawEnabled_ = false;
};
