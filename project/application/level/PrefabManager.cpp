#include "PrefabManager.h"

PrefabManager* PrefabManager::GetInstance()
{
    static PrefabManager instance;
    return &instance;
}

void PrefabManager::Initialize()
{
    if (isInitialized_) return;
    isInitialized_ = true;

    // ─── 1. 標準ブロック (standard_block / box) ───
    {
        BlockPrefabDefinition def;
        def.prefabId = "standard_block";
        def.modelName = "box";
        def.modelDir = "resources";
        def.texturePath = "resources/checker/checker_gray.png"; // グレーチェック柄
        def.defaultScale = { 3.2f, 2.2f, 3.2f };
        def.isOneway = false;
        def.collisionEnabled = true;
        RegisterPrefab(def);

        // "box" エイリアス
        def.prefabId = "box";
        RegisterPrefab(def);
    }

    // ─── 2. すり抜け足場 (oneway_platform) ───
    {
        BlockPrefabDefinition def;
        def.prefabId = "oneway_platform";
        def.modelName = "box";
        def.modelDir = "resources";
        def.texturePath = "resources/checker/checker_yellow.png"; // 黄色チェック柄
        def.defaultScale = { 3.6f, 0.4f, 2.6f };
        def.isOneway = true;
        def.collisionEnabled = true;
        def.defaultProperties["is_oneway"] = "true";
        RegisterPrefab(def);
    }

    // ─── 3. 浮島 (floating_island) ───
    {
        BlockPrefabDefinition def;
        def.prefabId = "floating_island";
        def.modelName = "box";
        def.modelDir = "resources";
        def.texturePath = "resources/checker/checker_blue.png"; // 水色チェック柄
        def.defaultScale = { 4.8f, 1.5f, 3.6f };
        def.isOneway = false;
        def.collisionEnabled = true;
        RegisterPrefab(def);
    }

    // ─── 4. 階段ステップ (stair_step) ───
    {
        BlockPrefabDefinition def;
        def.prefabId = "stair_step";
        def.modelName = "box";
        def.modelDir = "resources";
        def.texturePath = "resources/checker/checker_green.png"; // 黄緑チェック柄
        def.defaultScale = { 3.4f, 0.8f, 2.6f };
        def.isOneway = false;
        def.collisionEnabled = true;
        RegisterPrefab(def);
    }

    // ─── 5. 背景装飾キューブ (deco_cube / 当たり判定なし) ───
    {
        BlockPrefabDefinition def;
        def.prefabId = "deco_cube";
        def.modelName = "box";
        def.modelDir = "resources";
        def.texturePath = "resources/checker/checker_pink.png"; // ピンクチェック柄
        def.defaultScale = { 3.6f, 3.6f, 3.6f };
        def.isOneway = false;
        def.collisionEnabled = false; // 当たり判定なし
        RegisterPrefab(def);
    }

    // ─── 6. 板メッシュ装飾プレハブ群 (柵・草・花) ───
    auto registerProp = [&](const std::string& id, const std::string& tex, const Vector3& scl) {
        BlockPrefabDefinition def;
        def.prefabId = id;
        def.modelName = "prop_plane";
        def.modelDir = "resources";
        def.texturePath = tex;
        def.defaultScale = scl;
        def.isOneway = false;
        def.collisionEnabled = false;
        RegisterPrefab(def);
    };
    registerProp("prop_fence",         "resources/props/fence.png",         { 3.2f, 1.9f, 1.0f });
    registerProp("prop_grass",         "resources/props/grass_tuft.png",    { 2.1f, 1.6f, 1.0f });
    registerProp("prop_clover",        "resources/props/clover.png",        { 1.8f, 1.8f, 1.0f });
    registerProp("prop_flower_pink",   "resources/props/flower_pink.png",   { 1.8f, 1.8f, 1.0f });
    registerProp("prop_flower_white",  "resources/props/flower_white.png",  { 1.7f, 1.7f, 1.0f });
    registerProp("prop_flower_purple", "resources/props/flower_purple.png", { 1.9f, 1.9f, 1.0f });
}

void PrefabManager::RegisterPrefab(const BlockPrefabDefinition& def)
{
    prefabs_[def.prefabId] = def;
}

const BlockPrefabDefinition* PrefabManager::FindPrefab(const std::string& prefabId) const
{
    auto it = prefabs_.find(prefabId);
    if (it != prefabs_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool PrefabManager::HasPrefab(const std::string& prefabId) const
{
    return prefabs_.find(prefabId) != prefabs_.end();
}
