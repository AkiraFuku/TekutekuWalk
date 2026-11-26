#include "GameEngine.h"
#include "PrimitiveDrawer.h"
#include "OffScreen.h"
#include "Fade.h"
#include "GameMath.h"
#include <cstdint>

#ifdef USE_IMGUI
#include "imgui.h"

namespace {
struct DebugVertex {
    float x, y, z;
    uint32_t color;
};

void DrawGameMathDebugUI() {
    ImGui::Begin("PG4 GameMath Debug");

    // 1. Clamp (int32_t / float)
    ImGui::Text("1. Clamp<T>");
    static int clampIntVal = 120;
    static int clampIntMin = 0;
    static int clampIntMax = 100;
    ImGui::InputInt("Int Value", &clampIntVal);
    ImGui::InputInt("Int Min", &clampIntMin);
    ImGui::InputInt("Int Max", &clampIntMax);
    int32_t clampedInt = Clamp(
        static_cast<int32_t>(clampIntVal),
        static_cast<int32_t>(clampIntMin),
        static_cast<int32_t>(clampIntMax)
    );
    ImGui::Text("Clamp(int32_t) Result: %d", clampedInt);

    ImGui::Separator();

    static float clampFloatVal = 15.0f;
    static float clampFloatMin = 0.0f;
    static float clampFloatMax = 12.5f;
    ImGui::DragFloat("Float Value", &clampFloatVal, 0.1f);
    ImGui::DragFloat("Float Min", &clampFloatMin, 0.1f);
    ImGui::DragFloat("Float Max", &clampFloatMax, 0.1f);
    float clampedFloat = Clamp<float>(clampFloatVal, clampFloatMin, clampFloatMax);
    ImGui::Text("Clamp<float> Result: %.2f", clampedFloat);

    ImGui::Separator();

    // 2. Swap
    ImGui::Text("2. Swap<T>");
    static int swapA = 10;
    static int swapB = 20;
    ImGui::InputInt("Slot A", &swapA);
    ImGui::InputInt("Slot B", &swapB);
    if (ImGui::Button("Swap(Slot A, Slot B)")) {
        Swap(swapA, swapB);
    }

    ImGui::Separator();

    // 3. CalcArrayBytes
    ImGui::Text("3. CalcArrayBytes<DebugVertex>");
    static int vertexCount = 10;
    ImGui::InputInt("Vertex Count", &vertexCount);
    if (vertexCount < 0) {
        vertexCount = 0;
    }
    size_t totalBytes = CalcArrayBytes<DebugVertex>(static_cast<size_t>(vertexCount));
    ImGui::Text("sizeof(DebugVertex)=%zu * %d = %zu bytes", sizeof(DebugVertex), vertexCount, totalBytes);

    ImGui::Separator();

    // 4. Min / Max / IsInRange
    ImGui::Text("4. Min<T> / Max<T> / IsInRange<T>");
    static float minMaxA = 10.5f;
    static float minMaxB = 3.2f;
    ImGui::DragFloat("A (float)", &minMaxA, 0.1f);
    ImGui::DragFloat("B (float)", &minMaxB, 0.1f);
    ImGui::Text("Min: %.2f | Max: %.2f", Min(minMaxA, minMaxB), Max(minMaxA, minMaxB));

    static int rangeVal = 50;
    static int rangeMin = 0;
    static int rangeMax = 100;
    ImGui::InputInt("Range Value", &rangeVal);
    ImGui::InputInt("Range Min", &rangeMin);
    ImGui::InputInt("Range Max", &rangeMax);
    bool inRange = IsInRange(rangeVal, rangeMin, rangeMax);
    ImGui::Text("IsInRange: %s", inRange ? "true" : "false");

    ImGui::End();
}
} // namespace
#endif
void GameEngine::Initialize() {

    Framework::Initialize();



    sceneFactory_ = std::make_unique< SceneFactory>();


    SceneManager::GetInstance()->SetSceneFactory(sceneFactory_.get());

    Fade::GetInstance()->Initialize();
    //   SceneManager::GetInstance()->ChangeScene("GameScene");
    SceneManager::GetInstance()->ChangeScene("TitleScene");
    PrimitiveDrawer::GetInstance()->Initialize();
    OffScreen::GetInstance()->SetEffectFlags(
        PostEffectFlag::DepthOutline
    );
};
void GameEngine::Finalize() {
    SceneManager::GetInstance()->Finalize();

    Framework::Finalize();
};
void GameEngine::Update() {
    Framework::Update();
    SceneManager::GetInstance()->Update();
    Fade::GetInstance()->Update();
#ifdef USE_IMGUI
    DrawGameMathDebugUI();
#endif
}
void GameEngine::PreDraw()
{
    Framework::PreDraw();
}
;
void GameEngine::Draw() {

    SceneManager::GetInstance()->Draw();

    Framework::Draw();
    ///


}