#pragma once
#include "Object3d.h"
#include <memory>
#include "Vector3.h"
#include "Vector2.h"
#include "GameObject.h"
class RailPath;
class RailMover;
class Player;

class GoalObject : public GameObject {
public:
    struct GoalConfig {
        std::string modelDir = "resources/Goal/";
        std::string modelFile = "goal.obj";
        float radius = 0.5f;
        float rotateSpeed = 5.0f;
    };

    static void LoadGlobalConfig(const std::string& filePath = "resources/config/game_objects.json");
    static void SaveGlobalConfig(const std::string& filePath = "resources/config/game_objects.json");
    static GoalConfig& GetGlobalConfig() { return s_config; }
    static void ImGuiDrawDebugInfo();

    GoalObject();
    ~GoalObject();

    void Initialize();
    void Update();
    void Draw();

    // レールと配置場所（進捗 0.0~1.0）を設定
    void SetRail(RailPath* rail);
    void SetRailPosition(const Vector2& position);

    void SetCamera(Camera* camera);

    // 衝突判定用
    void OnCollision(GameObject* other) override;
    bool IsCleared() const {
        return isCleared_;
    }

    Vector3 GetWorldPosition() const override {
        return object_->GetTranslate();
    }
    float GetRadius() const  {
        return radius_;
    }
    CollisionCategory GetCategory() const override {
        return CollisionCategory::Goal;
    }
private:
    std::unique_ptr<Object3d> object_;
    std::unique_ptr<RailMover> railMover_;
    float worldY=0.0f;
    const float rotateSpeed=5.0f;
    float radius_ = 0.5f;   // 当たり判定の大きさ（見た目の星モデルに合わせたサイズ）
    bool isCleared_ = false; // クリアフラグ

    static inline GoalConfig s_config;
};