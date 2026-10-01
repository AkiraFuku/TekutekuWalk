#pragma once
#include"MathFunction.h"
#include "DrawFunction.h"
#include "Sprite.h"
#include"Object3D.h"
#include "Audio.h"
#include "Player.h"
#include "Scene.h"
#include <memory>
#include "SkyBox.h"
#include "CameraController.h"
#include "Vector2.h"
#include "Projectile.h"
#include "Animation.h"
#include "Enemy.h"
#include "GaidUI.h"
class StageManager;
class ParticleEmitter;
class RailPath;
class GoalObject;
class Phase;
class PlayerHPUI;
class ScoreUI;
class GameScene :public Scene
{
public:
    void Initialize()override;
    void Finalize()override;
    void Update()override;
    void Draw()override;

    /// <summary>
    /// フェーズの切り替え条件を満たしているかチェックする
    /// </summary>
    void CheckPhaseTransition();
    /// <summary>
    /// フェーズを変更する
    /// </summary>
    /// <param name="nextPhase"></param>


    //プレイヤーが落下下かの判定
    void CheckPlayerFall();


    Player* GetPlayer() override {
        return player.get();
    }
    CameraController* GetCamera() {
        return cameraController.get();
    }
    void RequestCameraShake(float duration = 0.1f, float power = 1.0f) override;
    RailPath* GetStageRaill();
    const std::vector<std::unique_ptr<Enemy>>& GetEnemies() {
        return enemies_;
    }
    const std::vector<std::unique_ptr<Projectile>>& GetProjectile() {
        return projectiles_;
    }
    const std::vector<Triangle>& GetTriangle() const override;
    GoalObject* GetGoal() {
        return goal_.get();
    }
    ScoreUI* GetScore() {
        return scoreUI_.get();
    }
    StageManager* GetStageManager() {
        return stageManager_.get();
    }

    GameScene();
    ~GameScene() override;

    Enemy* AddEnemy(Vector2 pos, Enemy::EnemyType enemyType = Enemy::EnemyType::Normal, const std::unordered_map<std::string, std::string>& properties = {});
    void AddProjectile(const Projectile::ProjectileSpawnParam& param, Projectile::ProjectileOwner owner);
    void AddTriangles(std::vector<Triangle> triangles);

    Audio::SoundHandle getBGMSoundoHandle() {
        return handle_;

    }

private:


    /// <summary>
    /// クリアフラグが立ったら遷移
    /// </summary>
    void CheckClear();
    //std::unique_ptr<Object3d> object3d;
    std::unique_ptr<Animation> animation;
    std::unique_ptr<Player> player;
    std::unique_ptr<RailPath> stageRail;
    std::unique_ptr<RailPath> cameraRail;

    std::unique_ptr<GaidUI> gaidUI_;
    std::unique_ptr<CameraController> cameraController;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::unique_ptr<SkyBox> skyBox;
    std::unique_ptr<CameraController> debugCameraC;

    bool isDebugCamera_ = false;
    std::unique_ptr<ParticleEmitter>emitter_;

    std::unique_ptr<GoalObject> goal_;

    Vector3 position_ = { 2.0f,0.0f,0.0f };
    Quaternion rotation_ = { 0.0f,0.0f,0.0f,1.0f };

    bool isCleared_ = false;
    bool isDefeated_ = false;
    Audio::SoundHandle handle_ = 0;
    // 投射物リスト
    std::vector<std::unique_ptr<Projectile>> projectiles_;
    //シーン内三角形リスト
    std::vector<Triangle>triangles_;

    // テスト用レイキャスト衝突判定メンバ変数
    std::unique_ptr<Object3d> boxObject_;
    bool isBoxHit_ = false;
    Vector3 boxPoint_ = { 0.0f,0.0f,0.0f };
    Vector3 boxHitPoint_ = {};
    float boxHitDistance_ = 0.0f;
    Triangle hitTriangle_ = {};
    Ray debugRay_ = {};

    //テスト用地面
    std::unique_ptr<Object3d> TestGround_;
    //ステージマネージャー（LevelLoader経由でステージ・敵等を統括）
    std::unique_ptr<StageManager> stageManager_;
    //生存限界
    float fallLimit_ = -8.5f;

    //UI
    std::unique_ptr<PlayerHPUI> playerHPUI_;
    std::unique_ptr<ScoreUI> scoreUI_;

    // ヒットストップ用変数
    bool isHitStop_ = false;
    float stopTimer_ = 0; // 残りヒットストップフレーム数

    void UpdateHitStop();

    // ── エネミースポーン待機（カメラ直前出現・一度で機能停止）──
    struct EnemySpawnTrigger {
        Vector2 railPos = { 0.0f, 0.0f };
        Enemy::EnemyType enemyType = Enemy::EnemyType::Normal;
        float triggerDistance = 25.0f; // 感知距離
        bool hasSpawned = false;       // 一度出現したら機能停止
        std::unordered_map<std::string, std::string> properties;
    };
    std::vector<EnemySpawnTrigger> enemySpawnTriggers_;
    void UpdateEnemySpawners();

public:
    // ヒットストップを開始する関数
    void TriggerHitStop(float frames = 0.1f) override {
        stopTimer_ = frames;
    }
    bool IsHitStopActive() const {
        return stopTimer_ > 0;
    }
};

