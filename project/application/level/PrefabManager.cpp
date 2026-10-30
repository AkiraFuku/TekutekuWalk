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
        def.texturePath = "resources/grass.png"; // 草地テクスチャプレハブ
        def.defaultScale = { 3.0f, 1.0f, 3.6f };
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
        def.texturePath = "resources/grass.png"; // すり抜けプレート
        def.defaultScale = { 3.5f, 0.25f, 2.0f };
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
        def.texturePath = "resources/grass.png";
        def.defaultScale = { 6.5f, 1.5f, 4.5f };
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
        def.texturePath = "resources/grass.png";
        def.defaultScale = { 4.0f, 0.5f, 2.0f };
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
        def.texturePath = "resources/grass.png";
        def.defaultScale = { 2.0f, 2.0f, 2.0f };
        def.isOneway = false;
        def.collisionEnabled = false; // 当たり判定なし
        RegisterPrefab(def);
    }
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
