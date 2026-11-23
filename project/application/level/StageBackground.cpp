#define NOMINMAX
#include "StageBackground.h"
#include "Model.h"
#include "ModelManager.h"
#include "MathFunction.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <sstream>

namespace {

/// <summary>指定した表向き方向と整合するCW四角形を追加するヘルパー</summary>
void AddOrientedQuad(
    std::vector<Model::VertexData>& vertices,
    std::vector<uint32_t>& indices,
    const Vector3& bl, const Vector3& tl,
    const Vector3& br, const Vector3& tr,
    const Vector2& uvBL, const Vector2& uvTL,
    const Vector2& uvBR, const Vector2& uvTR,
    const Vector3& expectedFrontDir,
    bool useUpNormal = false)
{
    Vector3 e1 = tl - bl;
    Vector3 e2 = br - bl;
    Vector3 crossN = Cross(e1, e2);
    float nLen = Length(crossN);
    Vector3 faceN = (nLen > 1e-5f) ? (crossN * (1.0f / nLen)) : Normalize(expectedFrontDir);

    bool flipWinding = (Dot(faceN, expectedFrontDir) < 0.0f);
    if (flipWinding) {
        faceN = faceN * -1.0f;
    }

    // 遠景の山・空では法線を上向き({0,1,0})に揃えることで、角度による暗い影や輪郭線を抑える
    Vector3 vertN = useUpNormal ? Vector3{ 0.0f, 1.0f, 0.0f } : faceN;

    uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.push_back({ { bl.x, bl.y, bl.z, 1.0f }, uvBL, vertN });
    vertices.push_back({ { tl.x, tl.y, tl.z, 1.0f }, uvTL, vertN });
    vertices.push_back({ { br.x, br.y, br.z, 1.0f }, uvBR, vertN });
    vertices.push_back({ { tr.x, tr.y, tr.z, 1.0f }, uvTR, vertN });

    if (!flipWinding) {
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);

        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 3);
    } else {
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 1);

        indices.push_back(base + 2);
        indices.push_back(base + 3);
        indices.push_back(base + 1);
    }
}

/// <summary>決定論的ハッシュ (0.0f ～ 1.0f)</summary>
float Hash01(uint32_t seed, uint32_t index)
{
    uint32_t x = seed ^ (index * 747796405u + 2891336453u);
    x = ((x >> ((x >> 28u) + 4u)) ^ x) * 277803737u;
    x = (x >> 22u) ^ x;
    return static_cast<float>(x & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

StageBackground::Style ParseStyle(const std::string& str, StageBackground::Style def)
{
    if (str == "MountainBand"    || str == "mountain"         || str == "MOUNTAIN")         return StageBackground::Style::MountainBand;
    if (str == "PanoramaWall"    || str == "panorama"         || str == "PANORAMA")         return StageBackground::Style::PanoramaWall;
    if (str == "PropLine"        || str == "propline"         || str == "PROPLINE")         return StageBackground::Style::PropLine;
    if (str == "HorizonMountain" || str == "horizon_mountain" || str == "HORIZON_MOUNTAIN") return StageBackground::Style::HorizonMountain;
    if (str == "HorizonSky"      || str == "horizon_sky"      || str == "HORIZON_SKY")      return StageBackground::Style::HorizonSky;
    if (str == "HorizonGround"   || str == "horizon_ground"   || str == "HORIZON_GROUND")   return StageBackground::Style::HorizonGround;
    return def;
}

Vector4 ParseColorRGBA(const std::string& str, const Vector4& def)
{
    std::stringstream ss(str);
    std::string token;
    float c[4] = { def.x, def.y, def.z, def.w };
    int idx = 0;
    while (std::getline(ss, token, ',') && idx < 4) {
        try { c[idx] = std::stof(token); } catch (...) {}
        idx++;
    }
    return { c[0], c[1], c[2], c[3] };
}

} // namespace

StageBackground::Config StageBackground::CreateDefaultConfig()
{
    Config cfg;
    cfg.enabled      = true;
    cfg.flipSide     = false;
    cfg.stepDistance = 2.0f;
    cfg.marginStart  = 14.0f;
    cfg.marginEnd    = 14.0f;

    // Layer 0: 遠景の草原地面リング（プレイヤーや手前ステージに被らないよう45m以遠・低位置に配置）
    LayerConfig horizonGround;
    horizonGround.style       = Style::HorizonGround;
    horizonGround.skirtDepth  = 45.0f;
    horizonGround.distance    = 170.0f;
    horizonGround.bottomY     = -4.5f;
    horizonGround.uvScaleU    = 18.0f;
    horizonGround.texturePath = "resources/terrain/grass_vivid.png";
    horizonGround.color       = { 0.45f, 0.80f, 0.45f, 1.0f };
    cfg.layers.push_back(horizonGround);

    // Layer 1: 手前の緑の丘（横視点で地形端の向こう側を隠す）
    LayerConfig nearHill;
    nearHill.style       = Style::HorizonMountain;
    nearHill.distance    = 68.0f;
    nearHill.bottomY     = -10.0f;
    nearHill.baseHeight  = 18.0f;
    nearHill.amplitude   = 3.5f;
    nearHill.frequency   = 8.0f;  // 1周あたりの山の数
    nearHill.phase       = 0.5f;
    nearHill.uvScaleU    = 12.0f;
    nearHill.texturePath = "resources/terrain/grass_vivid.png";
    nearHill.color       = { 0.36f, 0.78f, 0.32f, 1.0f };
    cfg.layers.push_back(nearHill);

    // Layer 2: 遠景の青緑・淡い水色の山並み（空気遠近法・外周リング）
    LayerConfig farMountain;
    farMountain.style       = Style::HorizonMountain;
    farMountain.distance    = 120.0f;
    farMountain.bottomY     = -18.0f;
    farMountain.baseHeight  = 32.0f;
    farMountain.amplitude   = 6.0f;
    farMountain.frequency   = 5.0f;
    farMountain.phase       = 2.1f;
    farMountain.uvScaleU    = 12.0f;
    farMountain.texturePath = "resources/white.png";
    farMountain.color       = { 0.52f, 0.84f, 0.88f, 1.0f };
    cfg.layers.push_back(farMountain);

    // Layer 3: 青空＋白い雲パノラマ帯（山並みの奥を囲む明るい水色の空）
    LayerConfig sky;
    sky.style       = Style::HorizonSky;
    sky.distance    = 180.0f;
    sky.bottomY     = -25.0f;
    sky.baseHeight  = 130.0f;
    sky.uvScaleU    = 6.0f;
    sky.texturePath = "resources/terrain/sky_clouds.png";
    sky.color       = { 1.00f, 1.00f, 1.00f, 1.0f };
    cfg.layers.push_back(sky);

    return cfg;
}

StageBackground::Config StageBackground::ParseConfigFromProperties(
    const std::unordered_map<std::string, std::string>& props)
{
    Config cfg = CreateDefaultConfig();

    auto getStr = [&](const std::string& key, const std::string& def) -> std::string {
        auto it = props.find(key);
        return (it != props.end()) ? it->second : def;
    };
    auto getFloat = [&](const std::string& key, float def) -> float {
        auto it = props.find(key);
        if (it != props.end()) {
            try { return std::stof(it->second); } catch (...) {}
        }
        return def;
    };
    auto getBool = [&](const std::string& key, bool def) -> bool {
        auto it = props.find(key);
        if (it != props.end()) {
            return (it->second == "true" || it->second == "1" || it->second == "True");
        }
        return def;
    };

    cfg.enabled      = getBool("enabled", cfg.enabled);
    cfg.flipSide     = getBool("flip_side", cfg.flipSide);
    cfg.stepDistance = std::max(0.5f, getFloat("step_distance", cfg.stepDistance));
    cfg.marginStart  = std::max(0.0f, getFloat("margin_start", cfg.marginStart));
    cfg.marginEnd    = std::max(0.0f, getFloat("margin_end", cfg.marginEnd));

    bool hasCustomLayer = false;
    for (int i = 0; i < 6; ++i) {
        std::string prefix = "layer" + std::to_string(i) + "_";
        if (props.contains(prefix + "style") || props.contains(prefix + "distance")) {
            hasCustomLayer = true;
            break;
        }
    }

    if (hasCustomLayer) {
        std::vector<LayerConfig> customLayers;
        for (int i = 0; i < 6; ++i) {
            std::string prefix = "layer" + std::to_string(i) + "_";
            if (!props.contains(prefix + "style") && !props.contains(prefix + "distance")) {
                continue;
            }
            LayerConfig l;
            l.style        = ParseStyle(getStr(prefix + "style", "HorizonMountain"), Style::HorizonMountain);
            l.distance     = getFloat(prefix + "distance",     l.distance);
            l.bottomY      = getFloat(prefix + "bottom_y",     l.bottomY);
            l.baseHeight   = getFloat(prefix + "base_height",  l.baseHeight);
            l.amplitude    = getFloat(prefix + "amplitude",    l.amplitude);
            l.frequency    = getFloat(prefix + "frequency",    l.frequency);
            l.phase        = getFloat(prefix + "phase",        l.phase);
            l.skirtDepth   = getFloat(prefix + "skirt_depth",  l.skirtDepth);
            l.uvScaleU     = std::max(1.0f, getFloat(prefix + "uv_scale_u", l.uvScaleU));
            l.propInterval = std::max(1.0f, getFloat(prefix + "prop_interval", l.propInterval));
            l.propWidth    = std::max(0.5f, getFloat(prefix + "prop_width", l.propWidth));
            l.propDepth    = std::max(0.5f, getFloat(prefix + "prop_depth", l.propDepth));
            l.seed         = static_cast<uint32_t>(std::max(0.0f, getFloat(prefix + "seed", static_cast<float>(l.seed))));
            l.texturePath  = getStr(prefix + "texture", l.texturePath);
            l.color        = ParseColorRGBA(getStr(prefix + "color", ""), l.color);
            customLayers.push_back(std::move(l));
        }
        if (!customLayers.empty()) {
            cfg.layers = std::move(customLayers);
        }
    }

    return cfg;
}

void StageBackground::Initialize(const RailPath* rail, Camera* camera, const Config& config)
{
    config_ = config;
    camera_ = camera;
    layerObjects_.clear();
    followCameraXZ_.clear();

    if (!config_.enabled || !rail || rail->GetPointCount() < 2) {
        return;
    }

    std::vector<RailSample> samples = SampleRail(
        rail, config_.stepDistance, config_.marginStart, config_.marginEnd, config_.flipSide);
    if (samples.size() < 2) {
        return;
    }

    const bool isLoop = rail->IsLoop();

    for (size_t i = 0; i < config_.layers.size(); ++i) {
        const LayerConfig& layer = config_.layers[i];
        std::vector<Model::VertexData> vertices;
        std::vector<uint32_t> indices;
        bool followCam = false;

        switch (layer.style) {
        case Style::MountainBand:
            BuildMountainBand(samples, layer, isLoop, vertices, indices);
            break;
        case Style::PanoramaWall:
            BuildPanoramaWall(samples, layer, isLoop, vertices, indices);
            break;
        case Style::PropLine:
            BuildPropLine(samples, layer, vertices, indices);
            break;
        case Style::HorizonMountain:
            BuildHorizonMountain(layer, vertices, indices);
            followCam = true;
            break;
        case Style::HorizonSky:
            BuildHorizonSky(layer, vertices, indices);
            followCam = true;
            break;
        case Style::HorizonGround:
            BuildHorizonGround(layer, vertices, indices);
            followCam = true;
            break;
        }

        if (vertices.empty() || indices.empty()) {
            continue;
        }

        std::string modelName = "__StageBackgroundLayer_" + std::to_string(i);
        std::shared_ptr<Model> model = Model::CreateFromMeshData(vertices, indices, layer.texturePath);
        ModelManager::GetInstance()->RegisterModel(modelName, model);

        auto obj = std::make_unique<Object3d>();
        obj->Initialize();
        obj->SetCamera(camera);
        obj->SetModel(modelName);
        obj->SetModelColor(layer.color);
        if (followCam && camera_) {
            Vector3 camPos = camera_->GetTranslate();
            obj->SetTranslate({ camPos.x, 0.0f, camPos.z });
        }
        obj->Update();
        layerObjects_.push_back(std::move(obj));
        followCameraXZ_.push_back(followCam);
    }
}

void StageBackground::Update()
{
    if (!config_.enabled) return;
    Vector3 camPos = camera_ ? camera_->GetTranslate() : Vector3{ 0.0f, 0.0f, 0.0f };
    for (size_t i = 0; i < layerObjects_.size(); ++i) {
        if (i < followCameraXZ_.size() && followCameraXZ_[i] && camera_) {
            layerObjects_[i]->SetTranslate({ camPos.x, 0.0f, camPos.z });
        }
        layerObjects_[i]->Update();
    }
}

void StageBackground::Draw()
{
    if (!config_.enabled) return;
    for (auto& obj : layerObjects_) {
        obj->Draw();
    }
}

void StageBackground::SetCamera(Camera* camera)
{
    camera_ = camera;
    for (auto& obj : layerObjects_) {
        obj->SetCamera(camera);
    }
}

std::vector<StageBackground::RailSample> StageBackground::SampleRail(
    const RailPath* rail,
    float stepDistance,
    float marginStart,
    float marginEnd,
    bool flipSide)
{
    std::vector<RailSample> result;
    const float totalLen = rail->GetTotalLength();
    if (totalLen <= 1e-3f) return result;

    const float step = std::max(0.5f, stepDistance);
    const int numSamples = std::max(2, static_cast<int>(std::ceil(totalLen / step)) + 1);
    const Vector3 up = { 0.0f, 1.0f, 0.0f };

    auto makeSample = [&](const Vector3& pos, const Vector3& rawFwd, float dist) -> RailSample {
        Vector3 fwd = { rawFwd.x, 0.0f, rawFwd.z };
        float fLen = Length(fwd);
        if (fLen > 1e-4f) {
            fwd = fwd * (1.0f / fLen);
        } else {
            fwd = { 0.0f, 0.0f, 1.0f };
        }
        Vector3 right = Cross(up, fwd);
        float rLen = Length(right);
        if (rLen > 1e-4f) {
            right = right * (1.0f / rLen);
        } else {
            right = { 1.0f, 0.0f, 0.0f };
        }
        if (flipSide) {
            right = right * -1.0f;
        }
        return { pos, fwd, right * -1.0f, dist };
    };

    std::vector<RailSample> core;
    core.reserve(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        float d = (static_cast<float>(i) / static_cast<float>(numSamples - 1)) * totalLen;
        float t = rail->GetTFromDistance(d);
        core.push_back(makeSample(rail->GetPosition(t), rail->GetDirection(t), d));
    }

    if (!rail->IsLoop() && marginStart > 0.0f) {
        const auto& first = core.front();
        int extCount = std::max(1, static_cast<int>(std::ceil(marginStart / step)));
        for (int k = extCount; k >= 1; --k) {
            float offset = (static_cast<float>(k) / static_cast<float>(extCount)) * marginStart;
            RailSample s = first;
            s.pos = first.pos - first.fwd * offset;
            s.dist = -offset;
            result.push_back(s);
        }
    }

    result.insert(result.end(), core.begin(), core.end());

    if (!rail->IsLoop() && marginEnd > 0.0f) {
        const auto& last = core.back();
        int extCount = std::max(1, static_cast<int>(std::ceil(marginEnd / step)));
        for (int k = 1; k <= extCount; ++k) {
            float offset = (static_cast<float>(k) / static_cast<float>(extCount)) * marginEnd;
            RailSample s = last;
            s.pos = last.pos + last.fwd * offset;
            s.dist = totalLen + offset;
            result.push_back(s);
        }
    }

    // 急カーブでの折り返し・自己交差を防ぐため farDir を移動平均で平滑化する
    const int n = static_cast<int>(result.size());
    const int halfWin = 8;
    std::vector<Vector3> smoothedFar(n);
    for (int i = 0; i < n; ++i) {
        Vector3 sum = { 0.0f, 0.0f, 0.0f };
        for (int w = -halfWin; w <= halfWin; ++w) {
            int idx = rail->IsLoop() ? ((i + w + n) % n) : std::clamp(i + w, 0, n - 1);
            sum = sum + result[idx].farDir;
        }
        float len = Length(sum);
        smoothedFar[i] = (len > 1e-4f) ? (sum * (1.0f / len)) : result[i].farDir;
    }
    for (int i = 0; i < n; ++i) {
        result[i].farDir = smoothedFar[i];
    }

    return result;
}

void StageBackground::BuildMountainBand(
    const std::vector<RailSample>& samples,
    const LayerConfig& layer,
    bool isLoop,
    std::vector<Model::VertexData>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    const size_t count = samples.size();
    if (count < 2) return;

    std::vector<Vector3> topPts(count);
    std::vector<Vector3> frontBotPts(count);
    std::vector<Vector3> backBotPts(count);
    std::vector<float> uCoords(count);

    const float taperSamples = 6.0f;
    for (size_t i = 0; i < count; ++i) {
        const auto& s = samples[i];
        float wave = 0.65f * std::sinf(s.dist * layer.frequency + layer.phase)
                   + 0.35f * std::sinf(s.dist * layer.frequency * 2.1f + layer.phase * 1.7f);
        float edgeFade = 1.0f;
        if (!isLoop && count > 2) {
            float dEdge = static_cast<float>(std::min(i, count - 1 - i));
            edgeFade = std::clamp(dEdge / taperSamples, 0.0f, 1.0f);
        }
        float h = (layer.baseHeight + layer.amplitude * wave) * edgeFade;

        float frontDist = std::max(1.0f, layer.distance - layer.skirtDepth);
        float backDist  = layer.distance + layer.skirtDepth;
        frontBotPts[i] = s.pos + s.farDir * frontDist + Vector3{ 0.0f, layer.bottomY, 0.0f };
        topPts[i]      = s.pos + s.farDir * layer.distance + Vector3{ 0.0f, layer.bottomY + h, 0.0f };
        backBotPts[i]  = s.pos + s.farDir * backDist  + Vector3{ 0.0f, layer.bottomY, 0.0f };
        uCoords[i]     = s.dist / layer.uvScaleU;
    }

    const size_t segCount = isLoop ? count : (count - 1);
    for (size_t i = 0; i < segCount; ++i) {
        size_t next = (i + 1) % count;
        Vector3 avgFar = Normalize((samples[i].farDir + samples[next].farDir) * 0.5f);
        Vector3 frontDir = avgFar * -1.0f;
        // 手前斜面
        AddOrientedQuad(
            outVertices, outIndices,
            frontBotPts[i], topPts[i], frontBotPts[next], topPts[next],
            { uCoords[i], 1.0f }, { uCoords[i], 0.0f },
            { uCoords[next], 1.0f }, { uCoords[next], 0.0f },
            frontDir, true);
        // 奥側背面（曲がり角の横・後ろから見ても空洞を見せない）
        AddOrientedQuad(
            outVertices, outIndices,
            backBotPts[next], topPts[next], backBotPts[i], topPts[i],
            { uCoords[next], 1.0f }, { uCoords[next], 0.0f },
            { uCoords[i], 1.0f }, { uCoords[i], 0.0f },
            avgFar, true);
    }
}

void StageBackground::BuildPanoramaWall(
    const std::vector<RailSample>& samples,
    const LayerConfig& layer,
    bool isLoop,
    std::vector<Model::VertexData>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    const size_t count = samples.size();
    if (count < 2) return;

    std::vector<Vector3> topPts(count);
    std::vector<Vector3> botPts(count);
    std::vector<float> uCoords(count);

    for (size_t i = 0; i < count; ++i) {
        const auto& s = samples[i];
        botPts[i] = s.pos + s.farDir * layer.distance + Vector3{ 0.0f, layer.bottomY, 0.0f };
        topPts[i] = s.pos + s.farDir * layer.distance + Vector3{ 0.0f, layer.bottomY + layer.baseHeight, 0.0f };
        uCoords[i] = s.dist / layer.uvScaleU;
    }

    const size_t segCount = isLoop ? count : (count - 1);
    for (size_t i = 0; i < segCount; ++i) {
        size_t next = (i + 1) % count;
        Vector3 frontDir = Normalize((samples[i].farDir + samples[next].farDir) * -0.5f);
        AddOrientedQuad(
            outVertices, outIndices,
            botPts[i], topPts[i], botPts[next], topPts[next],
            { uCoords[i], 1.0f }, { uCoords[i], 0.0f },
            { uCoords[next], 1.0f }, { uCoords[next], 0.0f },
            frontDir, true);
    }
}

void StageBackground::BuildPropLine(
    const std::vector<RailSample>& samples,
    const LayerConfig& layer,
    std::vector<Model::VertexData>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    if (samples.empty()) return;

    float nextSpawnDist = samples.front().dist;
    uint32_t propIdx = 0;
    const Vector3 up = { 0.0f, 1.0f, 0.0f };

    for (const auto& s : samples) {
        if (s.dist < nextSpawnDist) {
            continue;
        }
        nextSpawnDist = s.dist + layer.propInterval;

        float rW = 0.7f + 0.6f * Hash01(layer.seed, propIdx * 4 + 0);
        float rH = Hash01(layer.seed, propIdx * 4 + 1);
        float rD = 0.7f + 0.6f * Hash01(layer.seed, propIdx * 4 + 2);
        float rZ = (Hash01(layer.seed, propIdx * 4 + 3) - 0.5f) * 2.5f;

        float hw = layer.propWidth * 0.5f * rW;
        float hd = layer.propDepth * 0.5f * rD;
        float h  = std::max(1.5f, layer.baseHeight + (rH - 0.5f) * 2.0f * layer.amplitude);

        Vector3 center = s.pos + s.farDir * (layer.distance + rZ) + Vector3{ 0.0f, layer.bottomY, 0.0f };
        Vector3 fwd = s.fwd;
        Vector3 farDir = s.farDir;

        Vector3 bFL = center - fwd * hw - farDir * hd;
        Vector3 bFR = center + fwd * hw - farDir * hd;
        Vector3 bBL = center - fwd * hw + farDir * hd;
        Vector3 bBR = center + fwd * hw + farDir * hd;

        Vector3 tFL = bFL + up * h;
        Vector3 tFR = bFR + up * h;
        Vector3 tBL = bBL + up * h;
        Vector3 tBR = bBR + up * h;

        AddOrientedQuad(outVertices, outIndices, bFL, tFL, bFR, tFR, { 0,1 }, { 0,0 }, { 1,1 }, { 1,0 }, farDir * -1.0f);
        AddOrientedQuad(outVertices, outIndices, bBR, tBR, bBL, tBL, { 0,1 }, { 0,0 }, { 1,1 }, { 1,0 }, farDir);
        AddOrientedQuad(outVertices, outIndices, bBL, tBL, bFL, tFL, { 0,1 }, { 0,0 }, { 1,1 }, { 1,0 }, fwd * -1.0f);
        AddOrientedQuad(outVertices, outIndices, bFR, tFR, bBR, tBR, { 0,1 }, { 0,0 }, { 1,1 }, { 1,0 }, fwd);
        AddOrientedQuad(outVertices, outIndices, tFL, tBL, tFR, tBR, { 0,1 }, { 0,0 }, { 1,1 }, { 1,0 }, up);

        propIdx++;
    }
}

void StageBackground::BuildHorizonMountain(
    const LayerConfig& layer,
    std::vector<Model::VertexData>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    constexpr int kSegments = 128;
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    const float radius = std::max(10.0f, layer.distance);
    const float lobes = std::max(1.0f, std::round(layer.frequency));

    std::vector<Vector3> botPts(kSegments);
    std::vector<Vector3> topPts(kSegments);
    std::vector<Vector3> dirs(kSegments);

    for (int i = 0; i < kSegments; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(kSegments)) * kTwoPi;
        Vector3 dir = { std::cosf(angle), 0.0f, std::sinf(angle) };
        dirs[i] = dir;

        // 複数の滑らかな波を重ねて、カービィ64風の丸みのある山並みを作る
        float wave = 0.60f * std::sinf(angle * lobes + layer.phase)
                   + 0.28f * std::sinf(angle * (lobes * 2.0f) + layer.phase * 1.7f)
                   + 0.12f * std::cosf(angle * (lobes * 3.0f) - layer.phase * 0.8f);
        float h = std::max(2.0f, layer.baseHeight + layer.amplitude * wave);

        botPts[i] = { dir.x * radius, layer.bottomY, dir.z * radius };
        topPts[i] = { dir.x * radius, layer.bottomY + h, dir.z * radius };
    }

    for (int i = 0; i < kSegments; ++i) {
        int next = (i + 1) % kSegments;
        float u0 = (static_cast<float>(i) / static_cast<float>(kSegments)) * layer.uvScaleU;
        float u1 = (static_cast<float>(i + 1) / static_cast<float>(kSegments)) * layer.uvScaleU;
        Vector3 frontDir = Normalize((dirs[i] + dirs[next]) * -0.5f);
        AddOrientedQuad(
            outVertices, outIndices,
            botPts[i], topPts[i], botPts[next], topPts[next],
            { u0, 1.0f }, { u0, 0.0f },
            { u1, 1.0f }, { u1, 0.0f },
            frontDir, true);
    }
}

void StageBackground::BuildHorizonSky(
    const LayerConfig& layer,
    std::vector<Model::VertexData>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    constexpr int kSegments = 64;
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    const float radius = std::max(15.0f, layer.distance);

    std::vector<Vector3> botPts(kSegments);
    std::vector<Vector3> topPts(kSegments);
    std::vector<Vector3> dirs(kSegments);

    for (int i = 0; i < kSegments; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(kSegments)) * kTwoPi;
        Vector3 dir = { std::cosf(angle), 0.0f, std::sinf(angle) };
        dirs[i] = dir;
        botPts[i] = { dir.x * radius, layer.bottomY, dir.z * radius };
        topPts[i] = { dir.x * radius, layer.bottomY + layer.baseHeight, dir.z * radius };
    }

    for (int i = 0; i < kSegments; ++i) {
        int next = (i + 1) % kSegments;
        float u0 = (static_cast<float>(i) / static_cast<float>(kSegments)) * layer.uvScaleU;
        float u1 = (static_cast<float>(i + 1) / static_cast<float>(kSegments)) * layer.uvScaleU;
        Vector3 frontDir = Normalize((dirs[i] + dirs[next]) * -0.5f);
        AddOrientedQuad(
            outVertices, outIndices,
            botPts[i], topPts[i], botPts[next], topPts[next],
            { u0, 1.0f }, { u0, 0.0f },
            { u1, 1.0f }, { u1, 0.0f },
            frontDir, true);
    }
}

void StageBackground::BuildHorizonGround(
    const LayerConfig& layer,
    std::vector<Model::VertexData>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    constexpr int kSegments = 64;
    constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;
    const float innerR = std::max(4.0f, layer.skirtDepth);
    const float outerR = std::max(innerR + 5.0f, layer.distance);
    const float y = layer.bottomY;
    const Vector3 up = { 0.0f, 1.0f, 0.0f };

    std::vector<Vector3> innerPts(kSegments);
    std::vector<Vector3> outerPts(kSegments);

    for (int i = 0; i < kSegments; ++i) {
        float angle = (static_cast<float>(i) / static_cast<float>(kSegments)) * kTwoPi;
        Vector3 dir = { std::cosf(angle), 0.0f, std::sinf(angle) };
        innerPts[i] = { dir.x * innerR, y, dir.z * innerR };
        outerPts[i] = { dir.x * outerR, y, dir.z * outerR };
    }

    for (int i = 0; i < kSegments; ++i) {
        int next = (i + 1) % kSegments;
        float u0 = (static_cast<float>(i) / static_cast<float>(kSegments)) * layer.uvScaleU;
        float u1 = (static_cast<float>(i + 1) / static_cast<float>(kSegments)) * layer.uvScaleU;
        AddOrientedQuad(
            outVertices, outIndices,
            innerPts[i], outerPts[i], innerPts[next], outerPts[next],
            { u0, 0.0f }, { u0, 12.0f },
            { u1, 0.0f }, { u1, 12.0f },
            up, true);
    }
}
