#pragma once
#include "IInspectable.h"
#include "Camera.h"
#include <memory>
#include <vector>
#include <string>
#include <filesystem>

class Object3d;

/// <summary>
/// エディタの再生状態（Unity の Play / Pause / Step に対応）
/// </summary>
enum class EditorPlayState {
    Edit,   // 編集モード（ゲーム更新停止・Sceneカメラで編集）
    Play,   // プレイモード（通常のゲーム実行）
    Pause,  // 一時停止
    Step    // 1フレームのみ進める
};

/// <summary>
/// ビューポートの表示タブ
/// </summary>
enum class EditorViewportTab {
    Scene,  // エディタカメラ視点（ImGuizmo・自由移動対応）
    Game    // ゲームカメラ視点
};

/// <summary>
/// Unity風エディタUI（DockSpace, Hierarchy, Inspector, Viewport, Console）を統括するクラス
/// </summary>
class EditorManager
{
public:
    static EditorManager* GetInstance();
    static bool IsAlive() { return !s_isDestroyed_ && instance_ != nullptr; }
    static void SafeUnregister(IEditorInspectable* obj);
    friend struct std::default_delete<EditorManager>;

    EditorManager(const EditorManager&) = delete;
    EditorManager& operator=(const EditorManager&) = delete;

    void Initialize();
    void Finalize();

    /// <summary>毎フレームの更新（ショートカット・エディタカメラ等）</summary>
    void Update();

    /// <summary>エディタUI全体の描画（ImGui::NewFrame ～ Render の間で呼ぶ）</summary>
    void DrawEditorUI();

    // ─── オブジェクト登録（Hierarchy / Inspector 用） ───────────────────
    void RegisterObject(IEditorInspectable* obj);
    void UnregisterObject(IEditorInspectable* obj);
    void ClearObjects();

    void SetSelectedObject(IEditorInspectable* obj) { selectedObject_ = obj; }
    IEditorInspectable* GetSelectedObject() const { return selectedObject_; }

    // ─── 再生制御・表示制御 ──────────────────────────────────────────
#ifdef USE_IMGUI
    bool IsEditorEnabled() const { return isEditorEnabled_; }
    void SetEditorEnabled(bool enabled) { isEditorEnabled_ = enabled; }
    void ToggleEditorEnabled() { isEditorEnabled_ = !isEditorEnabled_; }
#else
    bool IsEditorEnabled() const { return false; }
    void SetEditorEnabled(bool) {}
    void ToggleEditorEnabled() {}
#endif

    EditorPlayState GetPlayState() const { return playState_; }
    void SetPlayState(EditorPlayState state);
    bool ShouldUpdateGame() const;
    void ConsumeStepFrame();

    EditorViewportTab GetActiveViewportTab() const { return activeTab_; }
    void SetActiveViewportTab(EditorViewportTab tab);

    Camera* GetEditorCamera() const { return editorCamera_.get(); }

    /// <summary>Sceneビュー用のグリッドを描画する</summary>
    void DrawSceneGrid();

    /// <summary>エディタ上で生成したオブジェクトの更新・描画</summary>
    void UpdateSpawnedObjects(Camera* gameCam);
    void DrawSpawnedObjects();

    /// <summary>オブジェクト生成・複製・削除・保存</summary>
    Object3d* SpawnCube();
    Object3d* SpawnObjectFromModel(const std::filesystem::path& modelPath);
    void DuplicateSelectedObject();
    void DeleteSelectedObject();
    void SaveSceneToJson(const std::string& outPath);

    /// <summary>Undo / Redo</summary>
    void Undo();
    void Redo();
    bool CanUndo() const { return !undoStack_.empty(); }
    bool CanRedo() const { return !redoStack_.empty(); }

    /// <summary>現在のビューポートに応じたアクティブカメラを取得（SceneタブならEditorCamera、Gameタブならnullptr）</summary>
    Camera* GetOverrideCameraIfSceneView() const;

private:
    EditorManager() = default;
    ~EditorManager() = default;

    void BuildDefaultDockLayout(unsigned int dockspaceId);
    void DrawMainMenuBar();
    void DrawToolbarWindow();
    void DrawHierarchyWindow();
    void DrawInspectorWindow();
    void DrawSceneWindow();
    void DrawGameWindow();
    void DrawConsoleWindow();
    void DrawProjectWindow();
    void DrawPostEffectWindow();
    void DrawSceneDebugWindow();
    void DrawGizmoOverlay(const float* viewportPos, const float* viewportSize);
    void SyncLightInspectables();
    void PickObjectInSceneView(float mouseX, float mouseY, const float* vpPos, const float* vpSize);
    void FocusSelectedObject();
    void CapturePlaySnapshot();
    void RestorePlaySnapshot();

    struct ObjectSnapshot {
        IEditorInspectable* target = nullptr;
        Vector3 pos{ 0, 0, 0 };
        Vector3 rot{ 0, 0, 0 };
        Vector3 scl{ 1, 1, 1 };
        bool visible = true;
    };

    struct UndoAction {
        ObjectSnapshot before;
        ObjectSnapshot after;
    };

    void PushUndoAction(const UndoAction& action);
    ObjectSnapshot CaptureSingleSnapshot(IEditorInspectable* target) const;
    void ApplySnapshot(const ObjectSnapshot& snap);

    static std::unique_ptr<EditorManager> instance_;
    static inline bool s_isDestroyed_ = false;

#ifdef USE_IMGUI
    bool isEditorEnabled_ = true;  // エディタビルド時は起動時からエディタUIを表示
#else
    bool isEditorEnabled_ = false; // リリースビルド時はゲーム画面のみ表示
#endif
    bool isFirstLayout_ = true;
    bool pendingTabSelect_ = false;

    EditorPlayState playState_ = EditorPlayState::Play;
    EditorViewportTab activeTab_ = EditorViewportTab::Game;

    // ウィンドウ表示フラグ
    bool showHierarchy_ = true;
    bool showInspector_ = true;
    bool showSceneView_ = true;
    bool showGameView_ = true;
    bool showConsole_ = true;
    bool showProject_ = true;
    bool showToolbar_ = true;
    bool showPostEffect_ = true;
    bool showSceneDebug_ = true;

    // 登録オブジェクトと選択状態
    std::vector<IEditorInspectable*> objects_;
    std::vector<std::unique_ptr<IEditorInspectable>> lightInspectables_;
    IEditorInspectable* selectedObject_ = nullptr;

    // エディタ専用カメラ（デバッグカメラ）
    std::unique_ptr<Camera> editorCamera_;
    bool isSceneHovered_ = false;

    // ImGuizmo 操作モード（0: Translate, 1: Rotate, 2: Scale）
    int gizmoOperation_ = 0;
    bool gizmoUseLocal_ = true;

    // Console 検索・オートスクロール
    bool consoleAutoScroll_ = true;

    // Scene 補助・ピッキング・スナップショット・Undo/Redo
    bool showGrid_ = true;
    bool wasUsingGizmo_ = false;
    ObjectSnapshot gizmoStartSnap_;
    ObjectSnapshot inspectorStartSnap_;
    bool isInspectorEditing_ = false;

    std::vector<ObjectSnapshot> playSnapshot_;
    std::vector<UndoAction> undoStack_;
    std::vector<UndoAction> redoStack_;

    // アセットブラウザ ＆ エディタ生成オブジェクト
    std::filesystem::path currentAssetDir_ = "resources";
    int assetFilterMode_ = 0; // 0: All, 1: Models, 2: Textures, 3: Stages
    std::vector<std::unique_ptr<Object3d>> spawnedObjects_;
};
