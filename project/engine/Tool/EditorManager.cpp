#include "EditorManager.h"
#include "ImGuiManager.h"
#include "DXCommon.h"
#include "SrvManager.h"
#include "WinApp.h"
#include "Logger.h"
#include "SceneManager.h"
#include "Scene.h"
#include "MathFunction.h"
#include "Input.h"
#include "OffScreen.h"
#include "Object3dCommon.h"
#include "Object3d.h"
#include "ModelManager.h"
#include "JsonManager.h"
#include "LightManager.h"
#include "PrimitiveDrawer.h"
#include <algorithm>
#include <map>
#include <cmath>

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"
#endif

namespace {
class LightInspectable : public IEditorInspectable {
public:
    enum class Type { Directional, Point, Spot };
    LightInspectable(Type t, size_t idx) : type_(t), index_(idx) {}

    std::string GetInspectorName() const override {
        switch (type_) {
        case Type::Directional: return "Directional Light [" + std::to_string(index_) + "]";
        case Type::Point:       return "Point Light [" + std::to_string(index_) + "]";
        case Type::Spot:        return "Spot Light [" + std::to_string(index_) + "]";
        }
        return "Light";
    }
    std::string GetInspectorCategory() const override { return "Light"; }

    Vector3* GetInspectorPositionPtr() override {
        auto* lm = LightManager::GetInstance();
        if (type_ == Type::Point && index_ < lm->GetPointLightCount()) {
            return &lm->GetPointLight(index_).position;
        }
        if (type_ == Type::Spot && index_ < lm->GetSpotLightCount()) {
            return &lm->GetSpotLight(index_).position;
        }
        return nullptr;
    }

    void OnTransformModified() override {
        auto* lm = LightManager::GetInstance();
        if (type_ == Type::Point && index_ < lm->GetPointLightCount()) {
            lm->GetPointLight(index_).isDirty = true;
        } else if (type_ == Type::Spot && index_ < lm->GetSpotLightCount()) {
            lm->GetSpotLight(index_).isDirty = true;
        }
    }

    void OnInspectorGUI() override {
#ifdef USE_IMGUI
        auto* lm = LightManager::GetInstance();
        if (type_ == Type::Directional && index_ < lm->GetDirectionalLightCount()) {
            auto& d = lm->GetDirectionalLight(index_);
            if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool changed = false;
                changed |= ImGui::ColorEdit4("Color", &d.color.x);
                changed |= ImGui::DragFloat3("Direction", &d.direction.x, 0.02f, -1.0f, 1.0f);
                changed |= ImGui::DragFloat("Intensity", &d.intensity, 0.05f, 0.0f, 20.0f);
                if (changed) {
                    if (Length(d.direction) > 0.0001f) {
                        d.direction = Normalize(d.direction);
                    }
                    d.isDirty = true;
                }
            }
        } else if (type_ == Type::Point && index_ < lm->GetPointLightCount()) {
            auto& p = lm->GetPointLight(index_);
            if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool changed = false;
                changed |= ImGui::ColorEdit4("Color", &p.color.x);
                changed |= ImGui::DragFloat("Intensity", &p.intensity, 0.05f, 0.0f, 50.0f);
                changed |= ImGui::DragFloat("Radius", &p.radius, 0.1f, 0.1f, 200.0f);
                changed |= ImGui::DragFloat("Decay", &p.decay, 0.05f, 0.0f, 10.0f);
                if (changed) {
                    p.isDirty = true;
                }
            }
        } else if (type_ == Type::Spot && index_ < lm->GetSpotLightCount()) {
            auto& s = lm->GetSpotLight(index_);
            if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool changed = false;
                changed |= ImGui::ColorEdit4("Color", &s.color.x);
                changed |= ImGui::DragFloat3("Direction", &s.direction.x, 0.02f, -1.0f, 1.0f);
                changed |= ImGui::DragFloat("Intensity", &s.intensity, 0.05f, 0.0f, 50.0f);
                changed |= ImGui::DragFloat("Distance", &s.distance, 0.1f, 0.1f, 200.0f);
                changed |= ImGui::DragFloat("Decay", &s.decay, 0.05f, 0.0f, 10.0f);
                changed |= ImGui::SliderFloat("Cos Angle", &s.cosAngle, 0.0f, 1.0f);
                changed |= ImGui::SliderFloat("Cos Falloff", &s.cosFalloffStart, 0.0f, 1.0f);
                if (changed) {
                    if (Length(s.direction) > 0.0001f) {
                        s.direction = Normalize(s.direction);
                    }
                    s.isDirty = true;
                }
            }
        }
#endif
    }

private:
    Type type_;
    size_t index_;
};
} // namespace

std::unique_ptr<EditorManager> EditorManager::instance_ = nullptr;

EditorManager* EditorManager::GetInstance()
{
    if (instance_ == nullptr) {
        struct Helper : public EditorManager {
            Helper() : EditorManager() {}
        };
        instance_ = std::make_unique<Helper>();
    }
    return instance_.get();
}

void EditorManager::Initialize()
{
    editorCamera_ = std::make_unique<Camera>();
    editorCamera_->SetEditorName("Editor Camera");
    editorCamera_->SetTranslate({ 0.0f, 6.0f, -18.0f });
    editorCamera_->SetRotate({ 0.25f, 0.0f, 0.0f });
#ifdef USE_IMGUI
    editorCamera_->SetEditorMode(true);
    editorCamera_->SetShowDebugWindow(false);
#endif
    editorCamera_->Update();
    RegisterObject(editorCamera_.get());
}

void EditorManager::Finalize()
{
    s_isDestroyed_ = true;
    spawnedObjects_.clear();
    objects_.clear();
    lightInspectables_.clear();
    selectedObject_ = nullptr;
    editorCamera_.reset();
    instance_.reset();
}

void EditorManager::SafeUnregister(IEditorInspectable* obj)
{
    if (IsAlive()) {
        instance_->UnregisterObject(obj);
    }
}

void EditorManager::RegisterObject(IEditorInspectable* obj)
{
    if (!obj) return;
    if (std::find(objects_.begin(), objects_.end(), obj) == objects_.end()) {
        objects_.push_back(obj);
    }
}

void EditorManager::UnregisterObject(IEditorInspectable* obj)
{
    if (!obj) return;
    auto it = std::find(objects_.begin(), objects_.end(), obj);
    if (it != objects_.end()) {
        objects_.erase(it);
    }
    if (selectedObject_ == obj) {
        selectedObject_ = nullptr;
    }
}

void EditorManager::ClearObjects()
{
    spawnedObjects_.clear();
    objects_.clear();
    selectedObject_ = nullptr;
    playSnapshot_.clear();
    undoStack_.clear();
    redoStack_.clear();
    wasUsingGizmo_ = false;
    isInspectorEditing_ = false;
    if (editorCamera_) {
        objects_.push_back(editorCamera_.get());
    }
}

void EditorManager::SetActiveViewportTab(EditorViewportTab tab)
{
    if (activeTab_ != tab) {
        activeTab_ = tab;
        pendingTabSelect_ = true;

        // Game -> Scene へ切り替えた瞬間、現在のゲームカメラ位置・角度を引き継ぐ
        if (tab == EditorViewportTab::Scene && editorCamera_) {
            if (Scene* cur = SceneManager::GetInstance()->GetCurrentScene()) {
                if (Camera* gameCam = cur->GetActiveCamera()) {
                    editorCamera_->SetTranslate(gameCam->GetTranslate());
                    editorCamera_->SetRotate(gameCam->GetRotate());
                    editorCamera_->Update();
                }
            }
        }
    }
}

void EditorManager::SetPlayState(EditorPlayState state)
{
    if (playState_ == state) return;

    // Edit -> Play に切り替える直前にスナップショットを保存
    if (playState_ == EditorPlayState::Edit && state == EditorPlayState::Play) {
        CapturePlaySnapshot();
    }
    // Play/Pause/Step -> Edit (Stop) に戻した時にスナップショットを復元
    if (playState_ != EditorPlayState::Edit && state == EditorPlayState::Edit) {
        RestorePlaySnapshot();
    }

    playState_ = state;
    if (state == EditorPlayState::Play) {
        SetActiveViewportTab(EditorViewportTab::Game);
    } else if (state == EditorPlayState::Edit) {
        SetActiveViewportTab(EditorViewportTab::Scene);
    }
}

bool EditorManager::ShouldUpdateGame() const
{
    if (!isEditorEnabled_) {
        return true;
    }
    return playState_ == EditorPlayState::Play || playState_ == EditorPlayState::Step;
}

void EditorManager::ConsumeStepFrame()
{
    if (playState_ == EditorPlayState::Step) {
        playState_ = EditorPlayState::Pause;
    }
}

Camera* EditorManager::GetOverrideCameraIfSceneView() const
{
    // ゲームカメラとデバッグカメラを完全に分離したため、常にゲームカメラを上書きしない
    return nullptr;
}

void EditorManager::Update()
{
    if (!isEditorEnabled_) {
        return;
    }

#ifdef USE_IMGUI
    // ギズモ操作モード・フォーカス・Undo/Redo のショートカット
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        if (ImGui::IsKeyPressed(ImGuiKey_W)) gizmoOperation_ = 0; // Translate
        if (ImGui::IsKeyPressed(ImGuiKey_E)) gizmoOperation_ = 1; // Rotate
        if (ImGui::IsKeyPressed(ImGuiKey_R)) gizmoOperation_ = 2; // Scale
        if (ImGui::IsKeyPressed(ImGuiKey_F)) FocusSelectedObject();
    }
    if (!io.WantTextInput && io.KeyCtrl) {
        if (ImGui::IsKeyPressed(ImGuiKey_Z)) {
            if (io.KeyShift) Redo();
            else Undo();
        } else if (ImGui::IsKeyPressed(ImGuiKey_Y)) {
            Redo();
        } else if (ImGui::IsKeyPressed(ImGuiKey_D)) {
            DuplicateSelectedObject();
        }
    }
    if (!io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        DeleteSelectedObject();
    }

    if (editorCamera_) {
        editorCamera_->SetEditorMode(true);
        editorCamera_->SetAllowViewportInput(isSceneHovered_);
        editorCamera_->Update();
    }
#endif
}

void EditorManager::DrawEditorUI()
{
#ifdef USE_IMGUI
    if (!isEditorEnabled_ || !ImGuiManager::GetInstance()->IsShow()) {
        return;
    }

    // フルスクリーン DockSpace
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("F9EngineEditorHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    DrawMainMenuBar();

    ImGuiID dockspaceId = ImGui::GetID("F9EngineDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    if (isFirstLayout_) {
        isFirstLayout_ = false;
        // 既存の ini にレイアウトが無ければデフォルトの Unity 風レイアウトを構築
        if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr ||
            ImGui::DockBuilderGetNode(dockspaceId)->ChildNodes[0] == nullptr) {
            BuildDefaultDockLayout(dockspaceId);
        }
    }

    ImGui::End();

    // 各ウィンドウ描画
    if (showToolbar_)    DrawToolbarWindow();
    if (showHierarchy_)  DrawHierarchyWindow();
    if (showInspector_)  DrawInspectorWindow();
    if (showSceneDebug_) DrawSceneDebugWindow();
    if (showPostEffect_) DrawPostEffectWindow();
    if (showSceneView_)  DrawSceneWindow();
    if (showGameView_)   DrawGameWindow();
    if (showConsole_)    DrawConsoleWindow();
    if (showProject_)    DrawProjectWindow();
#endif
}

void EditorManager::BuildDefaultDockLayout(unsigned int dockspaceId)
{
#ifdef USE_IMGUI
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

    ImGuiID dockMain = dockspaceId;
    ImGuiID dockTop = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Up, 0.065f, nullptr, &dockMain);
    ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.18f, nullptr, &dockMain);
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.26f, nullptr, &dockMain);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.25f, nullptr, &dockMain);

    // 中央ペインを左右に分割：左 = Scene(デバッグカメラ), 右 = Game(ゲームカメラ)
    ImGuiID dockGame = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.50f, nullptr, &dockMain);

    ImGui::DockBuilderDockWindow("Toolbar", dockTop);
    ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
    ImGui::DockBuilderDockWindow("Inspector", dockRight);
    ImGui::DockBuilderDockWindow("Scene Debug", dockRight);
    ImGui::DockBuilderDockWindow("Post Effect", dockRight);
    ImGui::DockBuilderDockWindow("Scene", dockMain);
    ImGui::DockBuilderDockWindow("Game", dockGame);
    ImGui::DockBuilderDockWindow("Console", dockBottom);
    ImGui::DockBuilderDockWindow("Project", dockBottom);

    ImGui::DockBuilderFinish(dockspaceId);
#else
    (void)dockspaceId;
#endif
}

void EditorManager::DrawMainMenuBar()
{
#ifdef USE_IMGUI
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Scene JSON (edited_stage.json)")) {
                SaveSceneToJson("resources/Stagemap/edited_stage.json");
            }
            if (ImGui::MenuItem("Reset Layout")) {
                isFirstLayout_ = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, CanUndo())) {
                Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, CanRedo())) {
                Redo();
            }
            ImGui::Separator();
            ImGui::MenuItem("Show Scene Grid", nullptr, &showGrid_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("Toolbar", nullptr, &showToolbar_);
            ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy_);
            ImGui::MenuItem("Inspector", nullptr, &showInspector_);
            ImGui::MenuItem("Scene Debug", nullptr, &showSceneDebug_);
            ImGui::MenuItem("Post Effect", nullptr, &showPostEffect_);
            ImGui::MenuItem("Scene", nullptr, &showSceneView_);
            ImGui::MenuItem("Game", nullptr, &showGameView_);
            ImGui::MenuItem("Console", nullptr, &showConsole_);
            ImGui::MenuItem("Project", nullptr, &showProject_);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
#endif
}

void EditorManager::DrawToolbarWindow()
{
#ifdef USE_IMGUI
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    if (ImGui::Begin("Toolbar", &showToolbar_, flags)) {
        // Gizmo ツールボタン
        auto ToolButton = [&](const char* label, int op) {
            bool active = (gizmoOperation_ == op);
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.78f, 1.0f));
            if (ImGui::Button(label)) gizmoOperation_ = op;
            if (active) ImGui::PopStyleColor();
        };

        ToolButton("Move (W)", 0);
        ImGui::SameLine();
        ToolButton("Rotate (E)", 1);
        ImGui::SameLine();
        ToolButton("Scale (R)", 2);
        ImGui::SameLine();
        if (ImGui::Button(gizmoUseLocal_ ? "Local" : "World")) {
            gizmoUseLocal_ = !gizmoUseLocal_;
        }

        // 中央付近に Play / Pause / Step ボタン
        ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.45f);

        bool isPlaying = (playState_ == EditorPlayState::Play);
        bool isPaused = (playState_ == EditorPlayState::Pause);

        if (isPlaying) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.30f, 1.0f));
        if (ImGui::Button(isPlaying ? "Stop" : "Play")) {
            SetPlayState(isPlaying ? EditorPlayState::Edit : EditorPlayState::Play);
        }
        if (isPlaying) ImGui::PopStyleColor();

        ImGui::SameLine();
        if (isPaused) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.50f, 0.15f, 1.0f));
        if (ImGui::Button("Pause")) {
            playState_ = isPaused ? EditorPlayState::Play : EditorPlayState::Pause;
        }
        if (isPaused) ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("Step")) {
            playState_ = EditorPlayState::Step;
        }

        ImGui::SameLine();
        const char* stateStr = "EDIT";
        switch (playState_) {
        case EditorPlayState::Edit:  stateStr = "EDIT"; break;
        case EditorPlayState::Play:  stateStr = "PLAYING"; break;
        case EditorPlayState::Pause: stateStr = "PAUSED"; break;
        case EditorPlayState::Step:  stateStr = "STEP"; break;
        }
        ImGui::Text(" [%s]", stateStr);
    }
    ImGui::End();
#endif
}



void EditorManager::SyncLightInspectables()
{
    auto* lm = LightManager::GetInstance();
    size_t total = lm->GetDirectionalLightCount() + lm->GetPointLightCount() + lm->GetSpotLightCount();
    if (lightInspectables_.size() == total) {
        return;
    }

    // 選択中オブジェクトがライトだった場合はクリア
    for (auto& l : lightInspectables_) {
        if (selectedObject_ == l.get()) {
            selectedObject_ = nullptr;
        }
    }
    lightInspectables_.clear();

    for (size_t i = 0; i < lm->GetDirectionalLightCount(); ++i) {
        lightInspectables_.push_back(std::make_unique<LightInspectable>(LightInspectable::Type::Directional, i));
    }
    for (size_t i = 0; i < lm->GetPointLightCount(); ++i) {
        lightInspectables_.push_back(std::make_unique<LightInspectable>(LightInspectable::Type::Point, i));
    }
    for (size_t i = 0; i < lm->GetSpotLightCount(); ++i) {
        lightInspectables_.push_back(std::make_unique<LightInspectable>(LightInspectable::Type::Spot, i));
    }
}

void EditorManager::DrawHierarchyWindow()
{
#ifdef USE_IMGUI
    if (ImGui::Begin("Hierarchy", &showHierarchy_)) {
        SyncLightInspectables();

        static ImGuiTextFilter filter;
        filter.Draw("Search", -1.0f);

        // 生成・ライト追加ボタン
        if (ImGui::Button("+ Cube")) {
            SpawnCube();
        }
        ImGui::SameLine();
        if (ImGui::Button("+ PointLight")) {
            LightManager::GetInstance()->AddPointLight({ 1, 1, 1, 1 }, { 0, 3, 0 }, 2.0f, 10.0f, 1.0f);
            SyncLightInspectables();
        }
        ImGui::SameLine();
        if (ImGui::Button("+ SpotLight")) {
            LightManager::GetInstance()->AddSpotLight({ 1, 1, 1, 1 }, { 0, 5, 0 }, 2.0f, { 0, -1, 0 }, 15.0f, 1.0f, 0.7f, 0.9f);
            SyncLightInspectables();
        }
        ImGui::Separator();

        if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::MenuItem("Create Cube")) { SpawnCube(); }
            if (ImGui::MenuItem("Create PointLight")) {
                LightManager::GetInstance()->AddPointLight({ 1, 1, 1, 1 }, { 0, 3, 0 }, 2.0f, 10.0f, 1.0f);
                SyncLightInspectables();
            }
            if (ImGui::MenuItem("Create SpotLight")) {
                LightManager::GetInstance()->AddSpotLight({ 1, 1, 1, 1 }, { 0, 5, 0 }, 2.0f, { 0, -1, 0 }, 15.0f, 1.0f, 0.7f, 0.9f);
                SyncLightInspectables();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, selectedObject_ != nullptr)) { DuplicateSelectedObject(); }
            if (ImGui::MenuItem("Delete", "Del", false, selectedObject_ != nullptr)) { DeleteSelectedObject(); }
            ImGui::EndPopup();
        }

        // カテゴリごとにグループ化
        std::map<std::string, std::vector<IEditorInspectable*>> grouped;
        for (IEditorInspectable* obj : objects_) {
            if (!obj) continue;
            std::string name = obj->GetInspectorName();
            if (!filter.PassFilter(name.c_str())) continue;
            grouped[obj->GetInspectorCategory()].push_back(obj);
        }
        for (auto& l : lightInspectables_) {
            std::string name = l->GetInspectorName();
            if (!filter.PassFilter(name.c_str())) continue;
            grouped[l->GetInspectorCategory()].push_back(l.get());
        }

        if (grouped.empty()) {
            ImGui::TextDisabled("No objects in scene.");
        }

        for (auto& [category, list] : grouped) {
            ImGuiTreeNodeFlags catFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
            bool catOpen = ImGui::TreeNodeEx(category.c_str(), catFlags);

            // "Object3d" カテゴリヘッダへのドロップで親を解除してルートに戻す
            if (category == "Object3d" && ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJ3D")) {
                    Object3d* dropped = *static_cast<Object3d* const*>(payload->Data);
                    if (dropped) dropped->SetParent(nullptr);
                }
                ImGui::EndDragDropTarget();
            }

            if (catOpen) {
                std::function<void(IEditorInspectable*)> drawNode = [&](IEditorInspectable* obj) {
                    auto* obj3d = dynamic_cast<Object3d*>(obj);
                    bool hasChildren = (obj3d && !obj3d->GetChildren().empty());
                    bool isSelected = (selectedObject_ == obj);

                    ImGui::PushID(static_cast<int>(reinterpret_cast<uintptr_t>(obj)));

                    bool* vis = obj->GetInspectorVisiblePtr();
                    if (vis) {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                        ImGui::Checkbox("##vis", vis);
                        ImGui::PopStyleVar();
                        ImGui::SameLine();
                    }

                    bool isHidden = (vis && !*vis);
                    if (isHidden) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    }

                    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
                    if (isSelected) nodeFlags |= ImGuiTreeNodeFlags_Selected;
                    if (!hasChildren) nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

                    bool nodeOpen = ImGui::TreeNodeEx(obj->GetInspectorName().c_str(), nodeFlags);
                    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                        selectedObject_ = obj;
                    }

                    // ドラッグ＆ドロップによる親子付け（Object3d 同士）
                    if (obj3d) {
                        if (ImGui::BeginDragDropSource()) {
                            ImGui::SetDragDropPayload("HIERARCHY_OBJ3D", &obj3d, sizeof(Object3d*));
                            ImGui::Text("%s", obj3d->GetInspectorName().c_str());
                            ImGui::EndDragDropSource();
                        }
                        if (ImGui::BeginDragDropTarget()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJ3D")) {
                                Object3d* dropped = *static_cast<Object3d* const*>(payload->Data);
                                if (dropped && dropped != obj3d) {
                                    dropped->SetParent(obj3d);
                                }
                            }
                            ImGui::EndDragDropTarget();
                        }
                    }

                    if (isHidden) {
                        ImGui::PopStyleColor();
                    }

                    if (hasChildren && nodeOpen) {
                        for (Object3d* child : obj3d->GetChildren()) {
                            drawNode(child);
                        }
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                };

                for (size_t i = 0; i < list.size(); ++i) {
                    IEditorInspectable* obj = list[i];
                    if (auto* obj3d = dynamic_cast<Object3d*>(obj)) {
                        // 親がいる子は親ノードの下で再帰描画するためルートではスキップ
                        if (obj3d->GetParent() != nullptr) continue;
                    }
                    drawNode(obj);
                }
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
#endif
}



void EditorManager::DrawInspectorWindow()
{
#ifdef USE_IMGUI
    if (ImGui::Begin("Inspector", &showInspector_)) {
        if (!selectedObject_) {
            ImGui::TextDisabled("Select an object in Hierarchy to inspect.");
            ImGui::End();
            return;
        }

        // ヘッダー情報（表示オンオフチェックボックス付き）
        if (bool* vis = selectedObject_->GetInspectorVisiblePtr()) {
            bool prevVis = *vis;
            if (ImGui::Checkbox("##InspectorVisible", vis)) {
                ObjectSnapshot before = CaptureSingleSnapshot(selectedObject_);
                before.visible = prevVis;
                ObjectSnapshot after = CaptureSingleSnapshot(selectedObject_);
                PushUndoAction({ before, after });
            }
            ImGui::SameLine();
        }
        ImGui::Text("%s", selectedObject_->GetInspectorName().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", selectedObject_->GetInspectorCategory().c_str());
        ImGui::Separator();

        // Transform セクション
        Vector3* pos = selectedObject_->GetInspectorPositionPtr();
        Vector3* rot = selectedObject_->GetInspectorRotationPtr();
        Vector3* scl = selectedObject_->GetInspectorScalePtr();

        if (pos || rot || scl) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto TrackEdit = [&](bool changed) {
                    if (ImGui::IsItemActivated()) {
                        inspectorStartSnap_ = CaptureSingleSnapshot(selectedObject_);
                        isInspectorEditing_ = true;
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit() && isInspectorEditing_) {
                        ObjectSnapshot after = CaptureSingleSnapshot(selectedObject_);
                        PushUndoAction({ inspectorStartSnap_, after });
                        isInspectorEditing_ = false;
                    }
                    return changed;
                };

                bool modified = false;
                if (pos) {
                    modified |= TrackEdit(ImGui::DragFloat3("Position", &pos->x, 0.1f));
                }
                if (rot) {
                    // ラジアン -> 度で表示・編集すると直感的
                    constexpr float kRadToDeg = 180.0f / 3.14159265f;
                    constexpr float kDegToRad = 3.14159265f / 180.0f;
                    float deg[3] = { rot->x * kRadToDeg, rot->y * kRadToDeg, rot->z * kRadToDeg };
                    bool rotChanged = ImGui::DragFloat3("Rotation", deg, 0.5f);
                    if (rotChanged) {
                        rot->x = deg[0] * kDegToRad;
                        rot->y = deg[1] * kDegToRad;
                        rot->z = deg[2] * kDegToRad;
                    }
                    modified |= TrackEdit(rotChanged);
                }
                if (scl) {
                    modified |= TrackEdit(ImGui::DragFloat3("Scale", &scl->x, 0.05f));
                }
                if (ImGui::Button("Reset Transform")) {
                    ObjectSnapshot before = CaptureSingleSnapshot(selectedObject_);
                    if (pos) *pos = { 0.0f, 0.0f, 0.0f };
                    if (rot) *rot = { 0.0f, 0.0f, 0.0f };
                    if (scl) *scl = { 1.0f, 1.0f, 1.0f };
                    modified = true;
                    PushUndoAction({ before, CaptureSingleSnapshot(selectedObject_) });
                }
                if (modified) {
                    selectedObject_->OnTransformModified();
                }
            }
        }

        // オブジェクト固有の Inspector GUI
        selectedObject_->OnInspectorGUI();
    }
    ImGui::End();
#endif
}



void EditorManager::DrawSceneWindow()
{
#ifdef USE_IMGUI
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
    isSceneHovered_ = false;
    if (ImGui::Begin("Scene", &showSceneView_)) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 32.0f && avail.y > 32.0f) {
            // マウスドラッグ完了時に Scene 用レンダーターゲットとカメラアスペクト比を動的リサイズ
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                uint32_t targetW = static_cast<uint32_t>(avail.x);
                uint32_t targetH = static_cast<uint32_t>(avail.y);
                DXCommon::GetInstance()->RequestSceneResize(targetW, targetH);
                if (editorCamera_) {
                    editorCamera_->SetAspectRatio(avail.x / avail.y);
                    editorCamera_->UpdateViewProjection();
                }
            }
        }

        ImVec2 imageSize = avail;
        ImVec2 screenPos = ImGui::GetCursorScreenPos();

        // renderTextures_[2] (デバッグカメラ) の SRV を描画
        uint32_t srvIndex = DXCommon::GetInstance()->GetRenderTextureSrvIndex(2);
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);
        ImGui::Image((ImTextureID)gpuHandle.ptr, imageSize);

        isSceneHovered_ = ImGui::IsItemHovered();

        // Scene ウィンドウ上に ImGuizmo と ViewManipulate をオーバーレイ
        if (editorCamera_) {
            float posArr[2] = { screenPos.x, screenPos.y };
            float sizeArr[2] = { imageSize.x, imageSize.y };

            if (selectedObject_) {
                DrawGizmoOverlay(posArr, sizeArr);
            } else {
                wasUsingGizmo_ = false;
            }

            // 右上に方位ギズモ (ViewManipulate) を描画
            ImGuizmo::SetDrawlist();
            Matrix4x4 viewMat = editorCamera_->GetViewMatrix();
            ImGuizmo::ViewManipulate(
                &viewMat.m[0][0], 8.0f,
                ImVec2(screenPos.x + imageSize.x - 96.0f, screenPos.y + 8.0f),
                ImVec2(88.0f, 88.0f), 0x10101010);

            // 左クリックで 3D ピッキング（ギズモ操作中でない時）
            if (isSceneHovered_ && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                !ImGuizmo::IsOver() && !ImGuizmo::IsUsing()) {
                ImVec2 mp = ImGui::GetMousePos();
                PickObjectInSceneView(mp.x, mp.y, posArr, sizeArr);
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
#endif
}

void EditorManager::DrawGameWindow()
{
#ifdef USE_IMGUI
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
    if (ImGui::Begin("Game", &showGameView_)) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float targetAspect = static_cast<float>(WinApp::kClientWidth) / static_cast<float>(WinApp::kClientHeight);

        ImVec2 imageSize = avail;
        if (avail.x > 0.0f && avail.y > 0.0f) {
            float availAspect = avail.x / avail.y;
            if (availAspect > targetAspect) {
                imageSize.x = avail.y * targetAspect;
                imageSize.y = avail.y;
            } else {
                imageSize.x = avail.x;
                imageSize.y = avail.x / targetAspect;
            }
        }

        ImVec2 cursorPos = ImGui::GetCursorPos();
        cursorPos.x += (avail.x - imageSize.x) * 0.5f;
        cursorPos.y += (avail.y - imageSize.y) * 0.5f;
        ImGui::SetCursorPos(cursorPos);

        // renderTextures_[1] (ゲームカメラ + ポストエフェクト) の SRV を描画
        uint32_t srvIndex = DXCommon::GetInstance()->GetRenderTextureSrvIndex(1);
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = SrvManager::GetInstance()->GetGPUDescriptorHandle(srvIndex);
        ImGui::Image((ImTextureID)gpuHandle.ptr, imageSize);
    }
    ImGui::End();
    ImGui::PopStyleVar();
#endif
}

void EditorManager::DrawGizmoOverlay(const float* viewportPos, const float* viewportSize)
{
#ifdef USE_IMGUI
    Vector3* pos = selectedObject_->GetInspectorPositionPtr();
    Vector3* rot = selectedObject_->GetInspectorRotationPtr();
    Vector3* scl = selectedObject_->GetInspectorScalePtr();
    if (!pos) return;

    Vector3 dummyRot = rot ? *rot : Vector3{ 0.0f, 0.0f, 0.0f };
    Vector3 dummyScl = scl ? *scl : Vector3{ 1.0f, 1.0f, 1.0f };

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportPos[0], viewportPos[1], viewportSize[0], viewportSize[1]);

    ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
    if (gizmoOperation_ == 1) op = ImGuizmo::ROTATE;
    else if (gizmoOperation_ == 2) op = ImGuizmo::SCALE;

    ImGuizmo::MODE mode = gizmoUseLocal_ ? ImGuizmo::LOCAL : ImGuizmo::WORLD;

    constexpr float kRadToDeg = 180.0f / 3.14159265f;
    constexpr float kDegToRad = 3.14159265f / 180.0f;

    float translation[3] = { pos->x, pos->y, pos->z };
    float rotationDeg[3] = { dummyRot.x * kRadToDeg, dummyRot.y * kRadToDeg, dummyRot.z * kRadToDeg };
    float scale[3] = { dummyScl.x, dummyScl.y, dummyScl.z };

    float worldMatrix[16];
    ImGuizmo::RecomposeMatrixFromComponents(translation, rotationDeg, scale, worldMatrix);

    const Matrix4x4& viewMat = editorCamera_->GetViewMatrix();
    const Matrix4x4& projMat = editorCamera_->GetProjectionMatrix();

    bool isUsingNow = ImGuizmo::IsUsing();
    if (isUsingNow && !wasUsingGizmo_) {
        gizmoStartSnap_ = CaptureSingleSnapshot(selectedObject_);
    }

    if (ImGuizmo::Manipulate(&viewMat.m[0][0], &projMat.m[0][0], op, mode, worldMatrix)) {
        ImGuizmo::DecomposeMatrixToComponents(worldMatrix, translation, rotationDeg, scale);
        *pos = { translation[0], translation[1], translation[2] };
        if (rot) {
            *rot = { rotationDeg[0] * kDegToRad, rotationDeg[1] * kDegToRad, rotationDeg[2] * kDegToRad };
        }
        if (scl) {
            *scl = { scale[0], scale[1], scale[2] };
        }
        selectedObject_->OnTransformModified();
    }

    if (!isUsingNow && wasUsingGizmo_) {
        ObjectSnapshot after = CaptureSingleSnapshot(selectedObject_);
        PushUndoAction({ gizmoStartSnap_, after });
    }
    wasUsingGizmo_ = isUsingNow;
#else
    (void)viewportPos;
    (void)viewportSize;
#endif
}

void EditorManager::DrawPostEffectWindow()
{
#ifdef USE_IMGUI
    if (ImGui::Begin("Post Effect", &showPostEffect_)) {
        OffScreen::GetInstance()->DrawImGuiContents();
    }
    ImGui::End();
#endif
}



void EditorManager::DrawSceneDebugWindow()
{
#ifdef USE_IMGUI
    if (ImGui::Begin("Scene Debug", &showSceneDebug_)) {
        if (Scene* cur = SceneManager::GetInstance()->GetCurrentScene()) {
            cur->OnEditorDebugGUI();
        }

        if (ImGui::CollapsingHeader("Camera & World Settings")) {
            float globalCurvature = Object3dCommon::GetInstance()->GetGlobalCurvature();
            if (ImGui::SliderFloat("Global Curvature", &globalCurvature, 0.0f, 0.005f, "%.5f")) {
                Object3dCommon::GetInstance()->SetGlobalCurvature(globalCurvature);
            }
            if (editorCamera_) {
                if (ImGui::Button("Sync Debug Cam to Game Cam")) {
                    if (Scene* cur = SceneManager::GetInstance()->GetCurrentScene()) {
                        if (Camera* gameCam = cur->GetActiveCamera()) {
                            editorCamera_->SetTranslate(gameCam->GetTranslate());
                            editorCamera_->SetRotate(gameCam->GetRotate());
                            editorCamera_->Update();
                        }
                    }
                }
                float speed = editorCamera_->GetMoveSpeed();
                if (ImGui::DragFloat("Editor Cam Speed", &speed, 0.02f, 0.01f, 10.0f)) {
                    editorCamera_->SetMoveSpeed(speed);
                }
                float rotSpeed = editorCamera_->GetRotateSpeed();
                if (ImGui::DragFloat("Editor Cam Rot Speed", &rotSpeed, 0.0002f, 0.0005f, 0.05f)) {
                    editorCamera_->SetRotateSpeed(rotSpeed);
                }
            }
        }
    }
    ImGui::End();
#endif
}



void EditorManager::DrawConsoleWindow()
{
#ifdef USE_IMGUI
    if (ImGui::Begin("Console", &showConsole_)) {
        if (ImGui::Button("Clear")) {
            Logger::ClearLogs();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &consoleAutoScroll_);
        ImGui::SameLine();
        static ImGuiTextFilter filter;
        filter.Draw("Filter", 200.0f);
        ImGui::Separator();

        ImGui::BeginChild("ConsoleScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        const auto& logs = Logger::GetLogs();
        for (const auto& line : logs) {
            if (!filter.PassFilter(line.c_str())) continue;
            ImGui::TextUnformatted(line.c_str());
        }
        if (consoleAutoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
    ImGui::End();
#endif
}



void EditorManager::DrawSceneGrid()
{
    if (!showGrid_ || !isEditorEnabled_) return;
#ifdef USE_LINE
    auto* pd = PrimitiveDrawer::GetInstance();
    constexpr int kHalfLines = 10;
    constexpr float kStep = 5.0f;
    constexpr float kExtent = kHalfLines * kStep;
    const Vector4 gridCol = { 0.35f, 0.35f, 0.38f, 0.55f };

    for (int i = -kHalfLines; i <= kHalfLines; ++i) {
        float offset = i * kStep;
        if (i == 0) {
            pd->DrawLine({ -kExtent, 0.0f, 0.0f }, { kExtent, 0.0f, 0.0f }, { 0.85f, 0.25f, 0.25f, 0.85f });
            pd->DrawLine({ 0.0f, 0.0f, -kExtent }, { 0.0f, 0.0f, kExtent }, { 0.25f, 0.45f, 0.90f, 0.85f });
        } else {
            pd->DrawLine({ -kExtent, 0.0f, offset }, { kExtent, 0.0f, offset }, gridCol);
            pd->DrawLine({ offset, 0.0f, -kExtent }, { offset, 0.0f, kExtent }, gridCol);
        }
    }
#endif
}

void EditorManager::FocusSelectedObject()
{
    if (!selectedObject_ || !editorCamera_) return;
    if (selectedObject_ == editorCamera_.get()) return;
    Vector3* pos = selectedObject_->GetInspectorPositionPtr();
    if (!pos) return;

    const Matrix4x4& mat = editorCamera_->GetWorldMatrix();
    Vector3 forward = { mat.m[2][0], mat.m[2][1], mat.m[2][2] };
    if (Length(forward) > 0.0001f) forward = Normalize(forward);
    else forward = { 0.0f, 0.0f, 1.0f };

    editorCamera_->SetTranslate(*pos - forward * 8.0f);
    editorCamera_->Update();
}

void EditorManager::PickObjectInSceneView(float mouseX, float mouseY, const float* vpPos, const float* vpSize)
{
    if (!editorCamera_ || vpSize[0] <= 0.0f || vpSize[1] <= 0.0f) return;

    float ndcX = ((mouseX - vpPos[0]) / vpSize[0]) * 2.0f - 1.0f;
    float ndcY = -(((mouseY - vpPos[1]) / vpSize[1]) * 2.0f - 1.0f);

    Matrix4x4 invVP = Inverse(editorCamera_->GetViewProtectionMatrix());
    auto unproject = [&](float x, float y, float z) -> Vector3 {
        float rx = x * invVP.m[0][0] + y * invVP.m[1][0] + z * invVP.m[2][0] + invVP.m[3][0];
        float ry = x * invVP.m[0][1] + y * invVP.m[1][1] + z * invVP.m[2][1] + invVP.m[3][1];
        float rz = x * invVP.m[0][2] + y * invVP.m[1][2] + z * invVP.m[2][2] + invVP.m[3][2];
        float rw = x * invVP.m[0][3] + y * invVP.m[1][3] + z * invVP.m[2][3] + invVP.m[3][3];
        if (std::abs(rw) > 1e-6f) {
            rx /= rw; ry /= rw; rz /= rw;
        }
        return { rx, ry, rz };
    };

    Vector3 rayOrigin = unproject(ndcX, ndcY, 0.0f);
    Vector3 rayEnd    = unproject(ndcX, ndcY, 1.0f);
    Vector3 rayDir    = rayEnd - rayOrigin;
    if (Length(rayDir) < 1e-6f) return;
    rayDir = Normalize(rayDir);

    IEditorInspectable* bestObj = nullptr;
    float bestT = 1e9f;

    auto testCandidate = [&](IEditorInspectable* obj) {
        if (!obj || obj == editorCamera_.get()) return;
        Vector3* pos = obj->GetInspectorPositionPtr();
        if (!pos) return;
        float radius = 0.8f;
        if (Vector3* scl = obj->GetInspectorScalePtr()) {
            float maxS = (std::max)({ std::abs(scl->x), std::abs(scl->y), std::abs(scl->z) });
            radius = (std::max)(0.6f, maxS * 0.8f);
        }
        Vector3 oc = rayOrigin - *pos;
        float b = oc.x * rayDir.x + oc.y * rayDir.y + oc.z * rayDir.z;
        float c = (oc.x * oc.x + oc.y * oc.y + oc.z * oc.z) - radius * radius;
        float disc = b * b - c;
        if (disc >= 0.0f) {
            float t = -b - std::sqrt(disc);
            if (t < 0.0f) t = -b + std::sqrt(disc);
            if (t > 0.0f && t < bestT) {
                bestT = t;
                bestObj = obj;
            }
        }
    };

    for (auto* obj : objects_) testCandidate(obj);
    for (auto& l : lightInspectables_) testCandidate(l.get());

    if (bestObj) {
        selectedObject_ = bestObj;
    }
}

EditorManager::ObjectSnapshot EditorManager::CaptureSingleSnapshot(IEditorInspectable* target) const
{
    ObjectSnapshot snap;
    snap.target = target;
    if (!target) return snap;
    if (Vector3* p = target->GetInspectorPositionPtr()) snap.pos = *p;
    if (Vector3* r = target->GetInspectorRotationPtr()) snap.rot = *r;
    if (Vector3* s = target->GetInspectorScalePtr())    snap.scl = *s;
    if (bool* v = target->GetInspectorVisiblePtr())     snap.visible = *v;
    return snap;
}

void EditorManager::ApplySnapshot(const ObjectSnapshot& snap)
{
    if (!snap.target) return;
    bool exists = (std::find(objects_.begin(), objects_.end(), snap.target) != objects_.end());
    if (!exists) {
        for (auto& l : lightInspectables_) {
            if (l.get() == snap.target) { exists = true; break; }
        }
    }
    if (!exists) return;

    if (Vector3* p = snap.target->GetInspectorPositionPtr()) *p = snap.pos;
    if (Vector3* r = snap.target->GetInspectorRotationPtr()) *r = snap.rot;
    if (Vector3* s = snap.target->GetInspectorScalePtr())    *s = snap.scl;
    if (bool* v = snap.target->GetInspectorVisiblePtr())     *v = snap.visible;
    snap.target->OnTransformModified();
}

void EditorManager::CapturePlaySnapshot()
{
    playSnapshot_.clear();
    for (auto* obj : objects_) {
        if (obj && obj != editorCamera_.get()) {
            playSnapshot_.push_back(CaptureSingleSnapshot(obj));
        }
    }
    for (auto& l : lightInspectables_) {
        playSnapshot_.push_back(CaptureSingleSnapshot(l.get()));
    }
}

void EditorManager::RestorePlaySnapshot()
{
    for (const auto& snap : playSnapshot_) {
        ApplySnapshot(snap);
    }
}

void EditorManager::PushUndoAction(const UndoAction& action)
{
    constexpr size_t kMaxUndo = 100;
    if (undoStack_.size() >= kMaxUndo) {
        undoStack_.erase(undoStack_.begin());
    }
    undoStack_.push_back(action);
    redoStack_.clear();
}

void EditorManager::Undo()
{
    if (undoStack_.empty()) return;
    UndoAction act = undoStack_.back();
    undoStack_.pop_back();
    ApplySnapshot(act.before);
    redoStack_.push_back(act);
}

void EditorManager::Redo()
{
    if (redoStack_.empty()) return;
    UndoAction act = redoStack_.back();
    redoStack_.pop_back();
    ApplySnapshot(act.after);
    undoStack_.push_back(act);
}

void EditorManager::DrawProjectWindow()
{
#ifdef USE_IMGUI
    if (ImGui::Begin("Project", &showProject_)) {
        if (!std::filesystem::exists(currentAssetDir_)) {
            currentAssetDir_ = "resources";
        }

        // パスバーと戻るボタン
        if (currentAssetDir_ != std::filesystem::path("resources") && currentAssetDir_.has_parent_path()) {
            if (ImGui::Button(".. (Up)")) {
                currentAssetDir_ = currentAssetDir_.parent_path();
            }
            ImGui::SameLine();
        }
        ImGui::Text("Path: %s", currentAssetDir_.string().c_str());

        // フィルタタブ
        const char* filters[] = { "All", "Models (.obj/.gltf)", "Textures (.png/.dds)", "Stages (.json)" };
        ImGui::SetNextItemWidth(180.0f);
        ImGui::Combo("Filter", &assetFilterMode_, filters, IM_ARRAYSIZE(filters));
        ImGui::SameLine();
        static ImGuiTextFilter searchFilter;
        searchFilter.Draw("Search", 200.0f);
        ImGui::Separator();

        ImGui::BeginChild("ProjectAssetList", ImVec2(0, 0), false);
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(currentAssetDir_, ec)) {
            std::string name = entry.path().filename().string();
            if (!searchFilter.PassFilter(name.c_str())) continue;

            if (entry.is_directory()) {
                if (ImGui::Selectable(("[Dir] " + name).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0) || ImGui::IsItemClicked()) {
                        currentAssetDir_ = entry.path();
                    }
                }
            } else {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                bool isModel = (ext == ".obj" || ext == ".gltf");
                bool isTex   = (ext == ".png" || ext == ".dds" || ext == ".jpg");
                bool isStage = (ext == ".json");

                if (assetFilterMode_ == 1 && !isModel) continue;
                if (assetFilterMode_ == 2 && !isTex) continue;
                if (assetFilterMode_ == 3 && !isStage) continue;

                std::string label = (isModel ? "[Model] " : isTex ? "[Tex] " : isStage ? "[JSON] " : "[File] ") + name;
                if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                    if (ImGui::IsMouseDoubleClicked(0) && isModel) {
                        SpawnObjectFromModel(entry.path());
                    }
                }
                if (isModel && ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Spawn in Scene")) {
                        SpawnObjectFromModel(entry.path());
                    }
                    ImGui::EndPopup();
                }
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
#endif
}

void EditorManager::UpdateSpawnedObjects(Camera* gameCam)
{
    for (auto& obj : spawnedObjects_) {
        if (!obj) continue;
        if (gameCam) obj->SetCamera(gameCam);
        obj->Update();
    }
}

void EditorManager::DrawSpawnedObjects()
{
    for (auto& obj : spawnedObjects_) {
        if (obj) obj->Draw();
    }
}

Object3d* EditorManager::SpawnCube()
{
    ModelManager::GetInstance()->CreateBoxModel("EditorCube");
    auto obj = std::make_unique<Object3d>();
    obj->Initialize();
    obj->SetModel("EditorCube");
    obj->SetEditorName("EditorCube");
    if (editorCamera_) {
        const Matrix4x4& mat = editorCamera_->GetWorldMatrix();
        Vector3 forward = { mat.m[2][0], mat.m[2][1], mat.m[2][2] };
        if (Length(forward) > 0.0001f) forward = Normalize(forward);
        obj->SetTranslate(editorCamera_->GetTranslate() + forward * 5.0f);
    }
    Object3d* ptr = obj.get();
    spawnedObjects_.push_back(std::move(obj));
    selectedObject_ = ptr;
    Logger::Log("[Editor] Spawned EditorCube.\n");
    return ptr;
}

Object3d* EditorManager::SpawnObjectFromModel(const std::filesystem::path& modelPath)
{
    std::string dir = modelPath.parent_path().generic_string();
    std::string file = modelPath.filename().string();
    ModelManager::GetInstance()->LoadModel(dir, file);

    auto obj = std::make_unique<Object3d>();
    obj->Initialize();
    obj->SetModel(file);
    if (editorCamera_) {
        const Matrix4x4& mat = editorCamera_->GetWorldMatrix();
        Vector3 forward = { mat.m[2][0], mat.m[2][1], mat.m[2][2] };
        if (Length(forward) > 0.0001f) forward = Normalize(forward);
        obj->SetTranslate(editorCamera_->GetTranslate() + forward * 5.0f);
    }
    Object3d* ptr = obj.get();
    spawnedObjects_.push_back(std::move(obj));
    selectedObject_ = ptr;
    Logger::Log("[Editor] Spawned model: " + file + "\n");
    return ptr;
}

void EditorManager::DuplicateSelectedObject()
{
    if (!selectedObject_) return;
    if (auto* src = dynamic_cast<Object3d*>(selectedObject_)) {
        auto copy = std::make_unique<Object3d>();
        copy->Initialize();
        if (!src->GetModelFilePath().empty()) {
            copy->SetModel(src->GetModelFilePath());
        }
        copy->SetTranslate(src->GetTranslate() + Vector3{ 1.0f, 0.0f, 0.0f });
        copy->SetRotate(src->GetRotate());
        copy->SetScale(src->GetScale());
        Object3d* ptr = copy.get();
        spawnedObjects_.push_back(std::move(copy));
        selectedObject_ = ptr;
        Logger::Log("[Editor] Duplicated Object3d.\n");
    }
}

void EditorManager::DeleteSelectedObject()
{
    if (!selectedObject_) return;
    for (auto it = spawnedObjects_.begin(); it != spawnedObjects_.end(); ++it) {
        if (it->get() == selectedObject_) {
            selectedObject_ = nullptr;
            spawnedObjects_.erase(it);
            Logger::Log("[Editor] Deleted spawned object.\n");
            return;
        }
    }
}

void EditorManager::SaveSceneToJson(const std::string& outPath)
{
    nlohmann::json root;
    root["name"] = "scene";
    root["objects"] = nlohmann::json::array();

    for (auto* obj : objects_) {
        if (!obj || obj == editorCamera_.get()) continue;
        std::string cat = obj->GetInspectorCategory();
        if (cat == "Camera" || cat == "UI / Sprite") continue;

        nlohmann::json item;
        item["name"] = obj->GetInspectorName();
        item["type"] = (cat == "Stage") ? "BLOCK" : "PROP";

        if (Vector3* p = obj->GetInspectorPositionPtr()) {
            item["transform"]["translation"] = { p->x, p->y, p->z };
        }
        if (Vector3* r = obj->GetInspectorRotationPtr()) {
            item["transform"]["rotation"] = { r->x, r->y, r->z };
        }
        if (Vector3* s = obj->GetInspectorScalePtr()) {
            item["transform"]["scale"] = { s->x, s->y, s->z };
        }
        if (auto* obj3d = dynamic_cast<Object3d*>(obj)) {
            if (!obj3d->GetModelFilePath().empty()) {
                item["model_name"] = obj3d->GetModelFilePath();
            }
        }
        root["objects"].push_back(item);
    }

    JsonManager::GetInstance()->Save(outPath, root);
    Logger::Log("[Editor] Saved scene JSON to: " + outPath + "\n");
}
