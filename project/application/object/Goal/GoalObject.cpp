#include "GoalObject.h"
#include "RailMover.h"
#include "ModelManager.h"
#include "PrimitiveDrawer.h"
#include "Player.h"
#include "Collider.h"
#include "DXCommon.h"
#include "JsonManager.h"
#include "Logger.h"

#ifdef USE_IMGUI
#include "imgui.h"
#endif

void GoalObject::LoadGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    if (!jsonMgr->Load("GameObjectsConfig", filePath)) {
        return;
    }
    const auto& root = jsonMgr->Get("GameObjectsConfig");
    if (!root.contains("goal")) return;

    const auto& j = root["goal"];
    s_config.modelDir = j.value("modelDir", s_config.modelDir);
    s_config.modelFile = j.value("modelFile", s_config.modelFile);
    s_config.radius = j.value("radius", s_config.radius);
    s_config.rotateSpeed = j.value("rotateSpeed", s_config.rotateSpeed);
}

void GoalObject::SaveGlobalConfig(const std::string& filePath) {
    auto jsonMgr = JsonManager::GetInstance();
    nlohmann::json root;
    if (jsonMgr->Load("GameObjectsConfig", filePath)) {
        root = jsonMgr->Get("GameObjectsConfig");
    }

    root["goal"] = {
        {"modelDir", s_config.modelDir},
        {"modelFile", s_config.modelFile},
        {"radius", s_config.radius},
        {"rotateSpeed", s_config.rotateSpeed}
    };

    jsonMgr->Save(filePath, root);
}

void GoalObject::ImGuiDrawDebugInfo() {
#ifdef USE_IMGUI
    ImGui::Begin("Debug/Goal");
    ImGui::Text("--- Goal Global Config ---");

    if (ImGui::Button("💾 Save Goal Config")) {
        SaveGlobalConfig();
    }
    ImGui::SameLine();
    if (ImGui::Button("🔄 Reload")) {
        LoadGlobalConfig();
    }

    ImGui::SliderFloat("Radius", &s_config.radius, 0.1f, 3.0f);
    ImGui::SliderFloat("Rotate Speed", &s_config.rotateSpeed, 0.0f, 20.0f);

    ImGui::End();
#endif
}

GoalObject::GoalObject() = default;
GoalObject::~GoalObject() = default;

void GoalObject::Initialize() {
    static bool s_loaded = false;
    if (!s_loaded) {
        LoadGlobalConfig();
        s_loaded = true;
    }

    radius_ = s_config.radius;

    object_ = std::make_unique<Object3d>();
    ModelManager::GetInstance()->LoadModel(s_config.modelDir, s_config.modelFile);
    object_->Initialize();
    object_->SetModel(s_config.modelFile);

    railMover_ = std::make_unique<RailMover>();
    auto collider = std::make_unique<Collider>();
    collider->initialize(this, radius_);
    SetCollider(std::move(collider));
}

void GoalObject::SetRail(RailPath* rail) {

    if (railMover_) {
        railMover_->SetPath(rail);
        object_->SetTranslate(railMover_->GetCurrentPosition());
    }
}

void GoalObject::SetRailPosition(const Vector2& position) {
    if (railMover_) {
        float progress = position.x; // 進捗をx成分から取得（例）
        worldY=position.y;


        railMover_->SetProgress(progress); // 進捗をRailMoverにバインド 
        // 初期座標の更新
        Vector3 railPos = railMover_->GetCurrentPosition();
        object_->SetTranslate({ railPos.x, position.y, railPos.z });
        object_->Update();
    }
}

void GoalObject::SetCamera(Camera* camera) {
     //if (railMover_) {
     //    object_->SetTranslate(railMover_->GetCurrentPosition());
     //}
    if (object_) object_->SetCamera(camera);
}

void GoalObject::Update() {
#ifdef USE_IMGUI
    ImGuiDrawDebugInfo();
#endif

    if (railMover_) {
        Vector3 pos = railMover_->GetCurrentPosition();
        pos.y=worldY;
        // SetRailPositionで設定した Y座標（高さ）を維持したい場合、
        // メンバ変数に高さを保持しておく必要があります
        object_->SetTranslate(pos);

    }
    object_;


    object_->Update();
}

void GoalObject::Draw() {
     object_->Draw();

    Vector4 color = isCleared_ ? Vector4{ 0.0f, 1.0f, 0.0f, 1.0f } : Vector4{ 1.0f, 1.0f, 0.0f, 1.0f };

    Sphere collisionSphere = { object_->GetTranslate(), radius_, {0,0,0,1} };
    PrimitiveDrawer::GetInstance()->DrawSphere(collisionSphere, color);
}

void GoalObject::OnCollision(GameObject* other) {
    if (other->GetCategory() == CollisionCategory::Player) {
        isCleared_ = true;
    }

}

