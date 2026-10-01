#include "LevelLoader.h"
#include "JsonManager.h"
#include <numbers>
#include <cassert>
#include <Windows.h>   // OutputDebugStringA

// nlohmann JSON を使用
#include "externals/Json/json.hpp"
using json = nlohmann::json;

namespace {

// ─────────────────────────────────────────────────────────────────────
// 座標系変換ヘルパー
// ─────────────────────────────────────────────────────────────────────
Vector3 ConvertTranslation(float bx, float by, float bz)
{
    // Blender: X右 Y奥 Z上  →  DX: X右 Y上 Z奥
    return { bx, bz, by };
}

Vector3 ConvertRotation(float bx, float by, float bz)
{
    // Blenderアドオンは degree で出力（export_scene.py: rot.x = math.degrees(...)）
    constexpr float deg2rad = static_cast<float>(std::numbers::pi) / 180.0f;
    return {
        -bx * deg2rad,
         bz * deg2rad,
         by * deg2rad
    };
}

Vector3 ConvertScale(float bx, float by, float bz)
{
    return { bx, bz, by };
}

// ─────────────────────────────────────────────────────────────────────
// オブジェクトタイプ文字列 → enum
// ─────────────────────────────────────────────────────────────────────
LevelObjectType ParseObjectType(const std::string& blenderType, const std::string& objectTypeTag)
{
    if (objectTypeTag == "TERRAIN")       return LevelObjectType::kTerrain;
    if (objectTypeTag == "BLOCK")         return LevelObjectType::kBlock;
    if (objectTypeTag == "ENEMY")         return LevelObjectType::kEnemy;
    if (objectTypeTag == "PLAYER_SPAWN")  return LevelObjectType::kPlayerSpawn;
    if (objectTypeTag == "GOAL")          return LevelObjectType::kGoal;
    if (objectTypeTag == "COIN" || objectTypeTag == "COLLECTIBLE") return LevelObjectType::kCoin;
    if (objectTypeTag == "TRIGGER")       return LevelObjectType::kTrigger;
    if (objectTypeTag == "PBD_ROPE")      return LevelObjectType::kPbdRope;
    if (objectTypeTag == "PBD_CLOTH")     return LevelObjectType::kPbdCloth;
    if (objectTypeTag == "STAGE_RAIL")    return LevelObjectType::kRailStage;
    if (objectTypeTag == "CAMERA_RAIL")   return LevelObjectType::kRailCamera;
    if (objectTypeTag == "PROP")          return LevelObjectType::kProp;

    if (blenderType == "MESH")   return LevelObjectType::kProp;
    if (blenderType == "EMPTY")  return LevelObjectType::kPlayerSpawn;
    if (blenderType == "CURVE")  return LevelObjectType::kRailStage;
    if (blenderType == "CAMERA") return LevelObjectType::kCamera;

    return LevelObjectType::kUnknown;
}

// 前方宣言
LevelObjectData ParseObject(const json& node, bool isGameCoords);

// ─────────────────────────────────────────────────────────────────────
// JSON ノードを 1 オブジェクトにパース（再帰）
// ─────────────────────────────────────────────────────────────────────
LevelObjectData ParseObject(const json& node, bool isGameCoords)
{
    LevelObjectData data;

    // ── 名前 ──
    data.name = node.value("name", "");

    // ── Blender オブジェクトタイプ（MESH, EMPTY, CURVE 等）──
    std::string blenderType = node.value("type", "MESH");

    // ── カスタムプロパティ object_type ──
    std::string objectTypeTag = node.value("object_type", "");
    data.type = ParseObjectType(blenderType, objectTypeTag);

    // ── disabled フラグ ──
    data.disabled = node.value("disabled", false);

    // ── Transform ──
    if (node.contains("transform")) {
        const auto& t = node["transform"];

        auto getArr3 = [](const json& j, const std::string& key) -> std::array<float, 3> {
            if (j.contains(key) && j[key].is_array() && j[key].size() >= 3) {
                return { j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>() };
            }
            return { 0.f, 0.f, 0.f };
        };

        auto tr = getArr3(t, "translation");
        auto ro = getArr3(t, "rotation");
        auto sc = getArr3(t, "scaling");

        if (isGameCoords) {
            constexpr float deg2rad = static_cast<float>(std::numbers::pi) / 180.0f;
            data.transform.translation = { tr[0], tr[1], tr[2] };
            data.transform.rotation    = { ro[0] * deg2rad, ro[1] * deg2rad, ro[2] * deg2rad };
            data.transform.scale       = { sc[0], sc[1], sc[2] };
        } else {
            data.transform.translation = ConvertTranslation(tr[0], tr[1], tr[2]);
            data.transform.rotation    = ConvertRotation(ro[0], ro[1], ro[2]);
            data.transform.scale       = ConvertScale(sc[0], sc[1], sc[2]);
        }
    }

    // ── モデルファイル名 & ディレクトリ & テクスチャ ──
    if (node.contains("file_name")) {
        std::string fn = node["file_name"].get<std::string>();
        // "box", "terrain_grid" 等の組み込みモデル名は拡張子を補完しない
        static const std::vector<std::string> kBuiltinNames = { "box", "terrain_grid" };
        bool isBuiltin = false;
        for (const auto& bn : kBuiltinNames) {
            if (fn == bn) { isBuiltin = true; break; }
        }
        if (!isBuiltin && fn.find('.') == std::string::npos) {
            fn += ".obj";
        }
        data.modelName = fn;
        data.modelDir  = node.value("model_dir", "resources/Stagemap");
    }

    if (node.contains("texture")) {
        data.texturePath = node["texture"].get<std::string>();
    } else if (node.contains("texture_path")) {
        data.texturePath = node["texture_path"].get<std::string>();
    }

    // ── Collider ──
    if (node.contains("collider")) {
        const auto& col = node["collider"];
        std::string colType = col.value("type", "BOX");
        data.collider.shape = (colType == "SPHERE")
            ? LevelColliderData::Shape::Sphere
            : LevelColliderData::Shape::Box;

        if (col.contains("center") && col["center"].is_array() && col["center"].size() >= 3) {
            float cx = col["center"][0].get<float>();
            float cy = col["center"][1].get<float>();
            float cz = col["center"][2].get<float>();
            data.collider.center = isGameCoords ? Vector3{ cx, cy, cz } : ConvertTranslation(cx, cy, cz);
        }
        if (col.contains("size") && col["size"].is_array() && col["size"].size() >= 3) {
            float sx = col["size"][0].get<float>();
            float sy = col["size"][1].get<float>();
            float sz = col["size"][2].get<float>();
            data.collider.size = isGameCoords ? Vector3{ sx, sy, sz } : ConvertScale(sx, sy, sz);
        }
    }

    // ── Trigger ──
    if (node.contains("trigger") || data.type == LevelObjectType::kTrigger) {
        const auto& tr = node.contains("trigger") ? node["trigger"] : json{};
        data.trigger.eventName = tr.value("event_name", data.name);
        std::string fm = tr.value("fire_mode", "once");
        if (fm == "continuous")  data.trigger.fireMode = LevelTriggerData::FireMode::Continuous;
        else if (fm == "enter_exit") data.trigger.fireMode = LevelTriggerData::FireMode::EnterExit;
        else                     data.trigger.fireMode = LevelTriggerData::FireMode::Once;

        // カスタムパラメータ
        if (tr.contains("params") && tr["params"].is_object()) {
            for (auto& [k, v] : tr["params"].items()) {
                data.trigger.params[k] = v.dump();
            }
        }
    }

    // ── PBD ──
    if (data.type == LevelObjectType::kPbdRope || data.type == LevelObjectType::kPbdCloth) {
        if (node.contains("pbd")) {
            const auto& pbd = node["pbd"];

            auto getPos = [&](const std::string& key) -> Vector3 {
                if (pbd.contains(key) && pbd[key].is_array() && pbd[key].size() >= 3) {
                    float px = pbd[key][0].get<float>();
                    float py = pbd[key][1].get<float>();
                    float pz = pbd[key][2].get<float>();
                    return isGameCoords ? Vector3{ px, py, pz } : ConvertTranslation(px, py, pz);
                }
                return data.transform.translation;
            };

            data.pbd.startPos     = getPos("start_pos");
            data.pbd.endPos       = getPos("end_pos");
            data.pbd.numPoints    = pbd.value("num_points", 10);
            data.pbd.widthPoints  = pbd.value("width_points", 8);
            data.pbd.heightPoints = pbd.value("height_points", 8);
            data.pbd.stiffness    = pbd.value("stiffness", 0.2f);
            data.pbd.damping      = pbd.value("damping", 0.05f);
            data.pbd.gravityY     = pbd.value("gravity_y", -9.8f);
            data.pbd.fixStart     = pbd.value("fix_start", true);
            data.pbd.fixEnd       = pbd.value("fix_end", false);
            data.pbd.texturePath  = pbd.value("texture", "");
        }
    }

    // ── Rail (Bezier Curve) ──
    if (data.type == LevelObjectType::kRailStage || data.type == LevelObjectType::kRailCamera) {
        data.railLoop = node.value("loop", false);
        if (node.contains("rail_points") && node["rail_points"].is_array()) {
            for (const auto& rp : node["rail_points"]) {
                LevelRailPoint pt;
                auto getV3 = [&](const json& j, const std::string& key) -> Vector3 {
                    if (j.contains(key) && j[key].is_array() && j[key].size() >= 3) {
                        float rx = j[key][0].get<float>();
                        float ry = j[key][1].get<float>();
                        float rz = j[key][2].get<float>();
                        return isGameCoords ? Vector3{ rx, ry, rz } : ConvertTranslation(rx, ry, rz);
                    }
                    return { 0,0,0 };
                };
                pt.position  = getV3(rp, "co");
                pt.handleIn  = getV3(rp, "handle_left");
                pt.handleOut = getV3(rp, "handle_right");

                std::string typeStr = rp.value("type", "BEZIER");
                if (typeStr == "LINEAR" || typeStr == "Linear") {
                    pt.type = RailPath::InterpolationType::Linear;
                } else if (typeStr == "CATMULL_ROM" || typeStr == "CatmullRom") {
                    pt.type = RailPath::InterpolationType::CatmullRom;
                } else {
                    pt.type = RailPath::InterpolationType::Bezier;
                }

                data.railPoints.push_back(pt);
            }
        }
    }

    // ── 敵種別 & レール位置 ──
    if (data.type == LevelObjectType::kEnemy) {
        data.enemyType = node.value("enemy_type", "Normal");
    }

    if (node.contains("rail_pos") && node["rail_pos"].is_array() && node["rail_pos"].size() >= 2) {
        data.railPos = { node["rail_pos"][0].get<float>(), node["rail_pos"][1].get<float>() };
    }

    // ── カスタムプロパティ（汎用）──
    if (node.contains("properties") && node["properties"].is_object()) {
        for (auto& [k, v] : node["properties"].items()) {
            data.properties[k] = v.is_string() ? v.get<std::string>() : v.dump();
        }
    }

    // ── 地形等の頂点位置データ ──
    if (node.contains("vertex_positions") && node["vertex_positions"].is_array()) {
        const auto& arr = node["vertex_positions"];
        data.vertexPositions.reserve(arr.size());
        for (const auto& item : arr) {
            if (item.is_array() && item.size() >= 3) {
                float vx = item[0].get<float>();
                float vy = item[1].get<float>();
                float vz = item[2].get<float>();
                data.vertexPositions.push_back(isGameCoords ? Vector3{ vx, vy, vz } : ConvertTranslation(vx, vy, vz));
            }
        }
    }

    // ── 地形等のポリゴン三角形インデックスデータ ──
    if (node.contains("indices") && node["indices"].is_array()) {
        const auto& arr = node["indices"];
        data.indices.reserve(arr.size());
        for (const auto& item : arr) {
            if (item.is_number_integer() || item.is_number_unsigned()) {
                data.indices.push_back(item.get<uint32_t>());
            }
        }
    }

    // ── 子オブジェクト（再帰） ──
    if (node.contains("children") && node["children"].is_array()) {
        for (const auto& child : node["children"]) {
            data.children.push_back(ParseObject(child, isGameCoords));
        }
    }

    return data;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────
// エントリポイント
// ─────────────────────────────────────────────────────────────────────
LevelData LevelLoader::Load(const std::string& filePath)
{
    LevelData result;

    bool success = false;
    json root = JsonManager::GetInstance()->LoadDirect(filePath, &success);
    if (!success) {
        OutputDebugStringA(("[LevelLoader] Failed to load: " + filePath + "\n").c_str());
        return result;
    }

    result.sceneName = root.value("name", "unnamed");
    bool isGameCoords = (root.value("coordinate_system", "BLENDER") == "GAME");

    if (root.contains("objects") && root["objects"].is_array()) {
        for (const auto& obj : root["objects"]) {
            LevelObjectData data = ParseObject(obj, isGameCoords);
            if (data.type != LevelObjectType::kUnknown) {
                result.objects.push_back(std::move(data));
            }
        }
    }

    OutputDebugStringA(("[LevelLoader] Loaded scene: " + result.sceneName +
                        " (" + std::to_string(result.objects.size()) + " objects)\n").c_str());
    return result;
}
