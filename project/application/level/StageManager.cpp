#include "StageManager.h"
#include "Enemy.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "PrimitiveDrawer.h"
#include "PrefabManager.h"
#include "InstancedBlockRenderer.h"
#include <Windows.h>   // OutputDebugStringA

// ─────────────────────────────────────────────────────────────────────
// Load / Hot Reload
// ─────────────────────────────────────────────────────────────────────
void StageManager::Load(const std::string& jsonPath, Camera* camera, SpawnEnemyFunc spawnEnemy)
{
    // プレハブマネージャー & インスタンシングレンダラーの初期化
    PrefabManager::GetInstance()->Initialize();
    InstancedBlockRenderer::GetInstance()->Initialize();

    // 既存リストをクリア（Hot Reload）
    blocks_.clear();
    terrain_.reset();
    stageBackground_.reset();
    backgroundConfig_ = StageBackground::CreateDefaultConfig();
    hasCustomBackgroundConfig_ = false;
    props_.clear();
    triggers_.clear();
    ropes_.clear();
    cloths_.clear();
    //coins_.clear();
    stageRail_.reset();
    cameraRail_.reset();
    allTriangles_.clear();
    playerSpawnPos_     = { 0.0f, 0.0f, 0.0f };
    playerSpawnRailPos_ = { 0.0f, 0.0f };
    goalPos_            = { 0.0f, 0.0f, 0.0f };
    goalRailPos_        = { 0.0f, 0.0f };
    stageCameraDrawDistance_ = 500.0f;
    stageCameraDistance_ = 9.5f;
    stageCameraHeight_   = 3.5f;

    camera_     = camera;
    loadedPath_ = jsonPath;

    // JSONを読み込む
    LevelData levelData = LevelLoader::Load(jsonPath);

    // 全オブジェクトを再帰的に処理
    for (const auto& obj : levelData.objects) {
        ProcessObject(obj, camera, spawnEnemy);
    }

    // ステージレールがあれば道テクスチャを適用＆奥側背景ポリゴンメッシュを生成
    if (stageRail_) {
        if (terrain_) {
            terrain_->ApplyRailRoad(stageRail_.get(), 1.65f);
        }
        stageBackground_ = std::make_unique<StageBackground>();
        stageBackground_->Initialize(stageRail_.get(), camera, backgroundConfig_);
        PopulateDefaultProps(camera);
    }

    // 三角形リストの初期構築
    RebuildTriangles();

    OutputDebugStringA(("[StageManager] Stage loaded: " + jsonPath + "\n").c_str());
}

// ─────────────────────────────────────────────────────────────────────
// 個別オブジェクト処理（再帰）
// ─────────────────────────────────────────────────────────────────────
void StageManager::ProcessObject(const LevelObjectData& data, Camera* camera, SpawnEnemyFunc& spawnEnemy)
{
    // disabled オブジェクトはスキップ
    if (data.disabled) return;

    switch (data.type) {

    // ─── 地形 ────────────────────────────────────────────────────
    case LevelObjectType::kTerrain:
    {
        terrain_ = std::make_unique<Terrain>();
        terrain_->Initialize(data, camera);
        break;
    }

    // ─── ブロック ──────────────────────────────────────────────────
    case LevelObjectType::kBlock:
    {
        auto block = std::make_unique<StageBlock>();
        block->Initialize(data, camera);
        blocks_.push_back(std::move(block));
        break;
    }

    // ─── 背景・装飾（当たり判定なし）──────────────────────────────
    case LevelObjectType::kProp:
    {
        auto prop = std::make_unique<StageBlock>();
        prop->Initialize(data, camera);
        prop->SetCollisionEnabled(false);
        props_.push_back(std::move(prop));
        break;
    }

    // ─── プレイヤー開始位置 ───────────────────────────────────────
    case LevelObjectType::kPlayerSpawn:
        playerSpawnPos_     = data.transform.translation;
        playerSpawnRailPos_ = data.railPos;
        break;

    // ─── ゴール ───────────────────────────────────────────────────
    case LevelObjectType::kGoal:
        goalPos_     = data.transform.translation;
        goalRailPos_ = data.railPos;
        break;

    // ─── コイン ───────────────────────────────────────────────────
    //case LevelObjectType::kCoin:
    //{
    //    auto coin = std::make_unique<Coin>();
    //    coin->Initialize(data.modelName);
    //    coin->SetCamera(camera);

    //    // レール配置指定がある場合
    //    if (data.railPos.x != 0.0f || data.railPos.y != 0.0f) {
    //        if (stageRail_) {
    //            coin->SetRail(stageRail_.get());
    //            coin->SetRailPosition(data.railPos);
    //        } else {
    //            coin->SetPosition(data.transform.translation);
    //        }
    //    } else {
    //        coin->SetPosition(data.transform.translation);
    //    }

    //    // プロパティ設定
    //    auto itScore = data.properties.find("score");
    //    if (itScore != data.properties.end()) {
    //        try { coin->SetScoreValue(std::stoi(itScore->second)); } catch (...) {}
    //    }
    //    auto itRadius = data.properties.find("radius");
    //    if (itRadius != data.properties.end()) {
    //        try { coin->SetRadius(std::stof(itRadius->second)); } catch (...) {}
    //    }

    //    coins_.push_back(std::move(coin));
    //    break;
    //}

    // ─── 敵 ───────────────────────────────────────────────────────
    case LevelObjectType::kEnemy:
        if (spawnEnemy) {
            spawnEnemy(data);
        }
        break;

    // ─── イベントトリガー ─────────────────────────────────────────
    case LevelObjectType::kTrigger:
    {
        auto trigger = std::make_unique<EventTrigger>();
        trigger->Initialize(data);
        triggers_.push_back(std::move(trigger));
        break;
    }

    // ─── PBD ロープ ───────────────────────────────────────────────
    case LevelObjectType::kPbdRope:
    {
        auto rope = std::make_unique<PbdRope>();
        rope->Initialize(
            data.pbd.startPos,
            data.pbd.endPos,
            data.pbd.numPoints,
            data.pbd.stiffness,
            1.0f / 60.0f,
            data.pbd.damping,
            { 0.0f, data.pbd.gravityY, 0.0f }
        );
        // 固定端の設定
        if (data.pbd.fixStart) rope->SetFixed(0, true);
        if (data.pbd.fixEnd)   rope->SetFixed(data.pbd.numPoints - 1, true);

        ropes_.push_back(std::move(rope));
        break;
    }

    // ─── PBD 布 ───────────────────────────────────────────────────
    case LevelObjectType::kPbdCloth:
    {
        auto cloth = std::make_unique<PbdCloth>();
        cloth->Initialize(
            data.pbd.startPos,
            data.pbd.endPos,
            data.pbd.widthPoints,
            data.pbd.heightPoints,
            data.pbd.stiffness,
            1.0f / 60.0f,
            data.pbd.damping,
            { 0.0f, data.pbd.gravityY, 0.0f }
        );
        cloth->SetCamera(camera);
        if (!data.pbd.texturePath.empty()) {
            TextureManager::GetInstance()->LoadTexture(data.pbd.texturePath);
            cloth->SetTexture(data.pbd.texturePath);
        }
        cloths_.push_back(std::move(cloth));
        break;
    }

    // ─── ステージレール ───────────────────────────────────────────
    case LevelObjectType::kRailStage:
    {
        stageRail_ = std::make_unique<RailPath>();
        stageRail_->SetLoop(data.railLoop);
        for (const auto& rp : data.railPoints) {
            // handleOut/handleIn は "このポイントから出るオフセット"
            Vector3 offsetOut = { rp.handleOut.x - rp.position.x,
                                  rp.handleOut.y - rp.position.y,
                                  rp.handleOut.z - rp.position.z };
            Vector3 offsetIn  = { rp.handleIn.x - rp.position.x,
                                  rp.handleIn.y - rp.position.y,
                                  rp.handleIn.z - rp.position.z };
            stageRail_->AddBezierPoint(rp.position, offsetIn, offsetOut, rp.type);
        }
        stageRail_->Update();
        break;
    }

    // ─── カメラレール ─────────────────────────────────────────────
    case LevelObjectType::kRailCamera:
    {
        cameraRail_ = std::make_unique<RailPath>();
        cameraRail_->SetLoop(data.railLoop);
        for (const auto& rp : data.railPoints) {
            Vector3 offsetOut = { rp.handleOut.x - rp.position.x,
                                  rp.handleOut.y - rp.position.y,
                                  rp.handleOut.z - rp.position.z };
            Vector3 offsetIn  = { rp.handleIn.x - rp.position.x,
                                  rp.handleIn.y - rp.position.y,
                                  rp.handleIn.z - rp.position.z };
            cameraRail_->AddBezierPoint(rp.position, offsetIn, offsetOut, rp.type);
        }
        cameraRail_->Update();
        break;
    }

    // ─── カメラ設定 ───────────────────────────────────────────────
    case LevelObjectType::kCamera:
    {
        auto getFloat = [&](const std::string& key, float def) -> float {
            auto it = data.properties.find(key);
            if (it != data.properties.end()) {
                try { return std::stof(it->second); } catch (...) {}
            }
            return def;
        };
        stageCameraDrawDistance_ = getFloat("draw_distance", 500.0f);
        stageCameraDistance_     = getFloat("camera_distance", 9.5f);
        stageCameraHeight_       = getFloat("camera_height", 3.5f);
        auto itMode = data.properties.find("camera_mode");
        if (itMode != data.properties.end()) {
            stageCameraMode_ = itMode->second;
        }
        if (camera) {
            camera->SetDrawDistance(stageCameraDrawDistance_);
        }
        break;
    }

    // ─── ステージ奥側背景設定 ─────────────────────────────────────
    case LevelObjectType::kStageBackground:
    {
        backgroundConfig_ = StageBackground::ParseConfigFromProperties(data.properties);
        hasCustomBackgroundConfig_ = true;
        break;
    }

    default:
        break;
    }

    // 子オブジェクトの再帰処理
    for (const auto& child : data.children) {
        ProcessObject(child, camera, spawnEnemy);
    }
}

// ─────────────────────────────────────────────────────────────────────
// 三角形リスト再構築（全ブロック/地形から集約）
// ─────────────────────────────────────────────────────────────────────
void StageManager::RebuildTriangles()
{
    allTriangles_.clear();
    if (terrain_) {
        const auto& tris = terrain_->GetWorldTriangles();
        allTriangles_.insert(allTriangles_.end(), tris.begin(), tris.end());
    }
    for (const auto& block : blocks_) {
        const auto& tris = block->GetWorldTriangles();
        allTriangles_.insert(allTriangles_.end(), tris.begin(), tris.end());
    }
}

// ─────────────────────────────────────────────────────────────────────
// Update
// ─────────────────────────────────────────────────────────────────────
void StageManager::Update(const Vector3& playerPos)
{
    // 地形更新
    if (terrain_) {
        terrain_->Update();
    }
    if (stageBackground_) {
        stageBackground_->Update();
    }

    // ブロック更新
    for (auto& block : blocks_)   block->Update();
    for (auto& prop  : props_)    prop->Update();

    // 三角形リストを毎フレーム再構築（動的ブロック対応）
    RebuildTriangles();

    // イベントトリガー更新
    for (auto& trigger : triggers_) {
        trigger->Update(playerPos);
    }

    // PBD ロープ更新
    for (auto& rope : ropes_) {
        rope->Update();
    }

    // PBD 布更新
    for (auto& cloth : cloths_) {
        cloth->Update();
    }

    //// コイン更新
    //for (auto& coin : coins_) {
    //    coin->Update();
    //}
}

// ─────────────────────────────────────────────────────────────────────
// Draw
// ─────────────────────────────────────────────────────────────────────
void StageManager::Draw()
{
    // 地形描画（専用PSOによる道/地面テクスチャ割り振り＆穴抜き）
    if (terrain_) {
        terrain_->Draw();
    }
    // レール沿い奥側背景ポリゴン描画（遠景のため距離カリングなし）
    if (stageBackground_) {
        stageBackground_->Draw();
    }

    // カメラの描画距離範囲による距離カリング
    float maxDistSq = 1000000.0f;
    Vector3 camPos = { 0.0f, 0.0f, 0.0f };
    if (camera_) {
        camPos = camera_->GetTranslate();
        float drawDist = camera_->GetDrawDistance() + 5.0f; // 境界マージン
        maxDistSq = drawDist * drawDist;
    }

    // ブロック＆装飾オブジェクトの個別描画（距離カリング適用）
    for (auto& block : blocks_) {
        if (block->IsDisabled()) continue;
        Vector3 bPos = block->GetPosition();
        Vector3 diff = { bPos.x - camPos.x, bPos.y - camPos.y, bPos.z - camPos.z };
        if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= maxDistSq) {
            block->Draw();
        }
    }
    for (auto& prop : props_) {
        if (prop->IsDisabled()) continue;
        Vector3 pPos = prop->GetPosition();
        Vector3 diff = { pPos.x - camPos.x, pPos.y - camPos.y, pPos.z - camPos.z };
        if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= maxDistSq) {
            prop->Draw();
        }
    }

    for (auto& rope  : ropes_)   rope->Draw();
    for (auto& cloth : cloths_)  cloth->Draw();
    //for (auto& coin  : coins_) {
    //    coin->Draw();
    //}
}

// ─────────────────────────────────────────────────────────────────────
// DebugDraw
// ─────────────────────────────────────────────────────────────────────
void StageManager::DebugDraw()
{
    if (!isDebugDrawEnabled_) return;

    // トリガー領域のワイヤーフレーム
    for (auto& trigger : triggers_) {
        trigger->DebugDraw();
    }

    // レールのデバッグ描画
    if (stageRail_)  stageRail_->DebugDraw();
    if (cameraRail_) cameraRail_->DebugDraw();

    // 接地判定ポリゴンのワイヤーフレーム
    auto* pd = PrimitiveDrawer::GetInstance();
    Vector4 triColor = { 0.0f, 0.8f, 0.0f, 0.6f };
    for (const auto& tri : allTriangles_) {
        pd->DrawLine(tri.vertices[0], tri.vertices[1], triColor);
        pd->DrawLine(tri.vertices[1], tri.vertices[2], triColor);
        pd->DrawLine(tri.vertices[2], tri.vertices[0], triColor);
    }
}

// ─────────────────────────────────────────────────────────────────────
// レール周辺の仮置き装飾（柵・草・花・チェック柄キューブ）自動配置
// ─────────────────────────────────────────────────────────────────────
void StageManager::PopulateDefaultProps(Camera* camera)
{
    if (!stageRail_) return;
    float totalLen = stageRail_->GetTotalLength();
    if (totalLen < 4.0f) return;

    // 既に多数のPropがJSONで配置されている場合は追加しない
    if (props_.size() >= 10) return;

    float baseGroundY = terrain_ ? terrain_->GetTranslate().y : -0.5f;
    float flipSign = backgroundConfig_.flipSide ? -1.0f : 1.0f;

    // 地形三角形から正確な地面高さと法線Yを取得（穴や急斜面は false を返す）
    auto sampleTerrainSurface = [&](float x, float z, float& outY, float& outNormalY) -> bool {
        if (!terrain_) {
            outY = baseGroundY;
            outNormalY = 1.0f;
            return true;
        }
        if (terrain_->GetSurfaceType({ x, 0.0f, z }) == Terrain::CellType::kHole) {
            return false;
        }

        const auto& tris = terrain_->GetWorldTriangles();
        bool found = false;
        float bestY = -1e9f;
        float bestNY = 0.0f;

        for (const auto& tri : tris) {
            const Vector3& a = tri.vertices[0];
            const Vector3& b = tri.vertices[1];
            const Vector3& c = tri.vertices[2];

            float minX = (std::min)({ a.x, b.x, c.x });
            float maxX = (std::max)({ a.x, b.x, c.x });
            float minZ = (std::min)({ a.z, b.z, c.z });
            float maxZ = (std::max)({ a.z, b.z, c.z });
            if (x < minX || x > maxX || z < minZ || z > maxZ) continue;

            float v0x = c.x - a.x, v0z = c.z - a.z;
            float v1x = b.x - a.x, v1z = b.z - a.z;
            float v2x = x   - a.x, v2z = z   - a.z;

            float dot00 = v0x * v0x + v0z * v0z;
            float dot01 = v0x * v1x + v0z * v1z;
            float dot02 = v0x * v2x + v0z * v2z;
            float dot11 = v1x * v1x + v1z * v1z;
            float dot12 = v1x * v2x + v1z * v2z;

            float denom = dot00 * dot11 - dot01 * dot01;
            if (std::abs(denom) < 1e-6f) continue;
            float invDenom = 1.0f / denom;
            float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
            float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
            if (u >= -0.01f && v >= -0.01f && (u + v) <= 1.01f) {
                float hitY = a.y + u * (c.y - a.y) + v * (b.y - a.y);
                Vector3 e1 = { b.x - a.x, b.y - a.y, b.z - a.z };
                Vector3 e2 = { c.x - a.x, c.y - a.y, c.z - a.z };
                Vector3 n = Cross(e1, e2);
                float nLen = Length(n);
                float ny = (nLen > 1e-5f) ? std::abs(n.y / nLen) : 0.0f;
                if (!found || hitY > bestY) {
                    bestY = hitY;
                    bestNY = ny;
                    found = true;
                }
            }
        }

        if (!found) return false;
        // 崖スカート底や急斜面には配置しない
        if (bestY < baseGroundY - 1.8f || bestNY < 0.85f) return false;

        outY = bestY;
        outNormalY = bestNY;
        return true;
    };

    auto hash01 = [](uint32_t seed) -> float {
        uint32_t x = seed * 747796405u + 2891336453u;
        x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
        x = (x >> 22u) ^ x;
        return static_cast<float>(x & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
    };

    auto spawnProp = [&](const std::string& prefabId, const Vector3& pos, float rotY, float scaleMul = 1.0f, const std::string& checkerColor = "") {
        LevelObjectData d;
        d.name = "AutoProp_" + prefabId;
        d.type = LevelObjectType::kProp;
        d.transform.translation = pos;
        d.transform.rotation = { 0.0f, rotY, 0.0f };
        d.transform.scale = { scaleMul, scaleMul, scaleMul };
        d.properties["prefab_id"] = prefabId;
        if (!checkerColor.empty()) {
            d.properties["checker_color"] = checkerColor;
        }
        auto prop = std::make_unique<StageBlock>();
        prop->Initialize(d, camera);
        prop->SetCollisionEnabled(false);
        props_.push_back(std::move(prop));
    };

    const char* flowerIds[] = { "prop_flower_pink", "prop_flower_white", "prop_flower_purple" };
    const char* checkerCols[] = { "pink", "yellow", "blue" };

    uint32_t idx = 0;
    for (float dist = 2.0f; dist < totalLen - 2.0f; dist += 3.8f) {
        float t = stageRail_->GetTFromDistance(dist);
        Vector3 railPos = stageRail_->GetPosition(t);
        Vector3 fwd = stageRail_->GetDirection(t);
        fwd.y = 0.0f;
        float fLen = Length(fwd);
        if (fLen > 1e-4f) fwd = fwd * (1.0f / fLen);
        else fwd = { 0.0f, 0.0f, 1.0f };

        // カメラ側から見た「奥側」方向ベクトル
        Vector3 right = Normalize(Cross({ 0.0f, 1.0f, 0.0f }, fwd));
        Vector3 farDir = right * (-1.0f * flipSign);
        float faceRotY = std::atan2(-farDir.x, -farDir.z);

        float hitY = 0.0f;
        float ny = 1.0f;

        // 1. コース奥側と手前側に白い柵を仮置き（穴や急斜面を避け、地形高さに接地）
        if (hash01(idx * 13 + 1) > 0.25f) {
            float fx = railPos.x + farDir.x * 2.5f;
            float fz = railPos.z + farDir.z * 2.5f;
            if (sampleTerrainSurface(fx, fz, hitY, ny)) {
                spawnProp("prop_fence", { fx, hitY, fz }, faceRotY);
            }
        }
        if (hash01(idx * 13 + 2) > 0.45f) {
            float fx = railPos.x - farDir.x * 2.4f;
            float fz = railPos.z - farDir.z * 2.4f;
            if (sampleTerrainSurface(fx, fz, hitY, ny)) {
                spawnProp("prop_fence", { fx, hitY, fz }, faceRotY);
            }
        }

        // 2. ギザギザ草・クローバー・花を奥側と手前側に散りばめる
        float rGrass = hash01(idx * 13 + 3);
        float offsetFar = 3.2f + rGrass * 8.0f;
        {
            float gx = railPos.x + farDir.x * offsetFar;
            float gz = railPos.z + farDir.z * offsetFar;
            if (sampleTerrainSurface(gx, gz, hitY, ny)) {
                spawnProp((idx % 2 == 0) ? "prop_grass" : "prop_clover", { gx, hitY, gz }, faceRotY, 0.85f + rGrass * 0.45f);
            }
        }

        float rFlower = hash01(idx * 13 + 4);
        if (rFlower > 0.30f) {
            float fDist = (rFlower > 0.65f) ? -(1.8f + rFlower * 1.4f) : (2.8f + rFlower * 5.0f);
            float flx = railPos.x + farDir.x * fDist;
            float flz = railPos.z + farDir.z * fDist;
            if (sampleTerrainSurface(flx, flz, hitY, ny)) {
                spawnProp(flowerIds[idx % 3], { flx, hitY, flz }, faceRotY, 0.8f + rFlower * 0.4f);
            }
        }

        // 3. 奥側の草原にチェック柄装飾キューブを点在させる
        if (idx % 3 == 1) {
            float cubeDist = 6.5f + hash01(idx * 13 + 5) * 7.5f;
            float cubeScale = 1.6f + hash01(idx * 13 + 6) * 1.0f;
            float cx = railPos.x + farDir.x * cubeDist;
            float cz = railPos.z + farDir.z * cubeDist;
            if (sampleTerrainSurface(cx, cz, hitY, ny)) {
                spawnProp("deco_cube", { cx, hitY + cubeScale * 0.5f, cz }, faceRotY, cubeScale, checkerCols[idx % 3]);
            }
        }

        idx++;
    }
}
