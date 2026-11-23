#define NOMINMAX
#include "Terrain.h"
#include "RailPath.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "DXCommon.h"
#include "MathFunction.h"
#include <sstream>
#include <algorithm>
#include <unordered_map>

void Terrain::Initialize(const LevelObjectData& data, Camera* camera)
{
    name_     = data.name;
    camera_   = camera;

    transform_.translate = data.transform.translation;
    transform_.rotate    = data.transform.rotation;
    transform_.scale     = data.transform.scale;

    SetupPSO();
    BuildMesh(data);
    Update();
}

void Terrain::SetupPSO()
{
    // 既に "Terrain" PSO が登録済みなら再登録しない
    static bool s_psoRegistered = false;
    if (s_psoRegistered) {
        return;
    }
    s_psoRegistered = true;

    PsoConfig PSO{};
    PsoConfig::ShaderPath vsPath{ ShaderType::VS, L"resources/shaders/Terrain/Terrain.vs.hlsl", "main", L"vs_6_0" };
    PsoConfig::ShaderPath psPath{ ShaderType::PS, L"resources/shaders/Terrain/Terrain.ps.hlsl", "main", L"ps_6_0" };
    PSO.shaderPaths.push_back(vsPath);
    PSO.shaderPaths.push_back(psPath);

    PSO.rootSignatureGenerator = []() {
        return RootSignatureBuilder()
            // [Param 0] Material (CBV b0, Pixel)
            .AddCBV(0, D3D12_SHADER_VISIBILITY_PIXEL)
            // [Param 1] Transform (CBV b1, Vertex)
            .AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX)
            // [Param 2] Grass Texture (DescriptorTable t0, Pixel)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL)
            // [Param 3] Road Texture (DescriptorTable t1, Pixel)
            .AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, D3D12_SHADER_VISIBILITY_PIXEL)
            // スタティックサンプラー (s0)
            .AddStaticSampler(PSOManager::GetInstance()->StaticSamplers())
            .Build(DXCommon::GetInstance()->GetDevice().Get());
    };

    PSO.inputLayoutGenerator = []() {
        InputLayout inputLayout = {};
        inputLayout.inputElement = {
            { "POSITION",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  1, DXGI_FORMAT_R32_FLOAT,          0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD",  2, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        inputLayout.inputLayout.pInputElementDescs = inputLayout.inputElement.data();
        inputLayout.inputLayout.NumElements = static_cast<UINT>(inputLayout.inputElement.size());
        return inputLayout;
    };

    PSO.cullMode = D3D12_CULL_MODE_NONE; // 両面描画
    PSO.depthEnable = true;
    PSO.depthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    PSO.depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    PSOManager::GetInstance()->RegisterPsoGenerator("Terrain", PSO);
}

void Terrain::BuildMesh(const LevelObjectData& data)
{
    auto getFloat = [&](const std::string& key, float def) -> float {
        auto it = data.properties.find(key);
        if (it != data.properties.end()) {
            try { return std::stof(it->second); } catch (...) {}
        }
        return def;
    };
    auto getInt = [&](const std::string& key, int def) -> int {
        auto it = data.properties.find(key);
        if (it != data.properties.end()) {
            try { return std::stoi(it->second); } catch (...) {}
        }
        return def;
    };

    sizeX_  = getFloat("size_x",      160.0f);
    sizeY_  = getFloat("size_y",      240.0f);
    divX_   = getInt  ("divisions_x", 64);
    divY_   = getInt  ("divisions_y", 80);
    uvTile_ = getFloat("uv_tile",     10.0f);

    // テクスチャ設定（古いデフォルトがJSONに残っている場合も新しい草原・道テクスチャを適用）
    auto itGrass = data.properties.find("grass_texture");
    if (itGrass != data.properties.end() && itGrass->second != "resources/Stagemap/863603.png") {
        grassTexturePath_ = itGrass->second;
    } else {
        grassTexturePath_ = "resources/terrain/grass_vivid.png";
    }

    auto itRoad = data.properties.find("road_texture");
    if (itRoad != data.properties.end() && itRoad->second != "resources/grass.png") {
        roadTexturePath_ = itRoad->second;
    } else {
        roadTexturePath_ = "resources/terrain/road_cobble.png";
    }

    TextureManager::GetInstance()->LoadTexture(grassTexturePath_);
    TextureManager::GetInstance()->LoadTexture(roadTexturePath_);
    grassTexIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(grassTexturePath_);
    roadTexIndex_  = TextureManager::GetInstance()->GetTextureIndexByFilePath(roadTexturePath_);

    // ─── セル属性（cell_types）パース ────────────────────────────────
    int totalCells = divX_ * divY_;
    cellTypes_.assign(totalCells, CellType::kGround);

    auto itCells = data.properties.find("cell_types");
    if (itCells != data.properties.end() && !itCells->second.empty()) {
        std::stringstream ss(itCells->second);
        std::string token;
        int idx = 0;
        while (std::getline(ss, token, ',') && idx < totalCells) {
            try {
                int val = std::stoi(token);
                cellTypes_[idx] = static_cast<CellType>(val);
            } catch (...) {}
            idx++;
        }
    }

    // ─── 頂点・インデックス生成 ─────────────────────────────────────
    const int colCount = divX_ + 1;
    const int rowCount = divY_ + 1;
    const float halfX = sizeX_ * 0.5f;
    const float halfZ = sizeY_ * 0.5f;

    const bool hasCustomIndices = (!data.indices.empty() && !data.vertexPositions.empty());
    const bool hasMatchingGridVertices = (!hasCustomIndices && data.vertexPositions.size() == static_cast<size_t>(colCount * rowCount));

    vertices_.clear();
    indices_.clear();
    triangles_.clear();

    Matrix4x4 worldMat = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    auto addTriangle = [&](uint32_t i0, uint32_t i1, uint32_t i2) {
        indices_.push_back(i0);
        indices_.push_back(i1);
        indices_.push_back(i2);

        Triangle tri;
        Vector3 p0 = { vertices_[i0].position.x, vertices_[i0].position.y, vertices_[i0].position.z };
        Vector3 p1 = { vertices_[i1].position.x, vertices_[i1].position.y, vertices_[i1].position.z };
        Vector3 p2 = { vertices_[i2].position.x, vertices_[i2].position.y, vertices_[i2].position.z };
        tri.vertices[0] = vector3Transform(p0, worldMat);
        tri.vertices[1] = vector3Transform(p1, worldMat);
        tri.vertices[2] = vector3Transform(p2, worldMat);
        triangles_.push_back(tri);
    };

    if (hasCustomIndices) {
        // ── 【Mode A】Blenderから出力されたポリゴン三角形インデックスによる完全再構築 ──
        vertices_.reserve(data.vertexPositions.size());
        for (size_t i = 0; i < data.vertexPositions.size(); ++i) {
            VertexData v;
            const auto& pos = data.vertexPositions[i];
            v.position = { pos.x, pos.y, pos.z, 1.0f };
            float tx = (pos.x + halfX) / (sizeX_ > 1e-4f ? sizeX_ : 1.0f);
            float tz = (pos.z + halfZ) / (sizeY_ > 1e-4f ? sizeY_ : 1.0f);
            v.texCoord   = { tx, tz };
            v.normal     = { 0.0f, 1.0f, 0.0f };
            v.roadAttr   = 0.0f;
            v.extendInfo = { 0.0f, 0.0f, 0.0f };
            vertices_.push_back(v);
        }

        indices_.reserve(data.indices.size());
        for (size_t i = 0; i + 2 < data.indices.size(); i += 3) {
            uint32_t i0 = data.indices[i];
            uint32_t i1 = data.indices[i + 1];
            uint32_t i2 = data.indices[i + 2];
            if (i0 < vertices_.size() && i1 < vertices_.size() && i2 < vertices_.size()) {
                addTriangle(i0, i1, i2);
            }
        }

        // 三角形面からスムーズ法線を自動計算
        std::vector<Vector3> normals(vertices_.size(), { 0.0f, 0.0f, 0.0f });
        for (size_t i = 0; i + 2 < indices_.size(); i += 3) {
            uint32_t i0 = indices_[i], i1 = indices_[i + 1], i2 = indices_[i + 2];
            Vector3 p0 = { vertices_[i0].position.x, vertices_[i0].position.y, vertices_[i0].position.z };
            Vector3 p1 = { vertices_[i1].position.x, vertices_[i1].position.y, vertices_[i1].position.z };
            Vector3 p2 = { vertices_[i2].position.x, vertices_[i2].position.y, vertices_[i2].position.z };
            Vector3 e1 = { p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
            Vector3 e2 = { p2.x - p0.x, p2.y - p0.y, p2.z - p0.z };
            Vector3 fn = { e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x };
            normals[i0] = { normals[i0].x + fn.x, normals[i0].y + fn.y, normals[i0].z + fn.z };
            normals[i1] = { normals[i1].x + fn.x, normals[i1].y + fn.y, normals[i1].z + fn.z };
            normals[i2] = { normals[i2].x + fn.x, normals[i2].y + fn.y, normals[i2].z + fn.z };
        }
        for (size_t i = 0; i < vertices_.size(); ++i) {
            Vector3 n = normals[i];
            float lenSq = n.x * n.x + n.y * n.y + n.z * n.z;
            if (lenSq > 1e-6f) {
                float inv = 1.0f / std::sqrt(lenSq);
                vertices_[i].normal = { n.x * inv, n.y * inv, n.z * inv };
            }
        }
    } else {
        // ── 【Mode B】グリッド格子による生成（カスタムグリッド座標または平坦グリッド） ──
        vertices_.reserve(static_cast<size_t>(colCount) * rowCount);

        for (int row = 0; row < rowCount; ++row) {
            for (int col = 0; col < colCount; ++col) {
                float tx = static_cast<float>(col) / static_cast<float>(divX_);
                float tz = static_cast<float>(row) / static_cast<float>(divY_);
                int vIdx = row * colCount + col;

                VertexData v;
                if (hasMatchingGridVertices && vIdx < static_cast<int>(data.vertexPositions.size())) {
                    v.position = { data.vertexPositions[vIdx].x, data.vertexPositions[vIdx].y, data.vertexPositions[vIdx].z, 1.0f };
                } else {
                    v.position = { -halfX + tx * sizeX_, 0.0f, -halfZ + tz * sizeY_, 1.0f };
                }
                v.texCoord = { tx, tz };
                v.normal   = { 0.0f, 1.0f, 0.0f };

                float roadWeight = 0.0f;
                int roadCount = 0;
                int totalNeighbors = 0;

                for (int dy = -1; dy <= 0; ++dy) {
                    for (int dx = -1; dx <= 0; ++dx) {
                        int c = col + dx;
                        int r = row + dy;
                        if (c >= 0 && c < divX_ && r >= 0 && r < divY_) {
                            int cIdx = r * divX_ + c;
                            CellType ct = cellTypes_[cIdx];
                            if (ct == CellType::kRoad) {
                                roadCount++;
                            }
                            totalNeighbors++;
                        }
                    }
                }
                if (totalNeighbors > 0 && roadCount > 0) {
                    roadWeight = static_cast<float>(roadCount) / static_cast<float>(totalNeighbors);
                    if (roadCount >= 1) {
                        roadWeight = (std::max)(roadWeight, 0.7f);
                    }
                }
                v.roadAttr   = roadWeight;
                v.extendInfo = { 0.0f, 0.0f, 0.0f };

                vertices_.push_back(v);
            }
        }

        if (hasMatchingGridVertices) {
            for (int row = 0; row < rowCount; ++row) {
                for (int col = 0; col < colCount; ++col) {
                    int cL = (std::max)(0, col - 1);
                    int cR = (std::min)(colCount - 1, col + 1);
                    int rD = (std::max)(0, row - 1);
                    int rU = (std::min)(rowCount - 1, row + 1);

                    const auto& pL = vertices_[row * colCount + cL].position;
                    const auto& pR = vertices_[row * colCount + cR].position;
                    const auto& pD = vertices_[rD * colCount + col].position;
                    const auto& pU = vertices_[rU * colCount + col].position;

                    Vector3 tX = { pR.x - pL.x, pR.y - pL.y, pR.z - pL.z };
                    Vector3 tZ = { pU.x - pD.x, pU.y - pD.y, pU.z - pD.z };

                    Vector3 n = {
                        tZ.y * tX.z - tZ.z * tX.y,
                        tZ.z * tX.x - tZ.x * tX.z,
                        tZ.x * tX.y - tZ.y * tX.x
                    };
                    float lenSq = n.x * n.x + n.y * n.y + n.z * n.z;
                    if (lenSq > 1e-6f) {
                        float invLen = 1.0f / std::sqrt(lenSq);
                        n.x *= invLen; n.y *= invLen; n.z *= invLen;
                    } else {
                        n = { 0.0f, 1.0f, 0.0f };
                    }
                    vertices_[row * colCount + col].normal = n;
                }
            }
        }

        indices_.reserve(static_cast<size_t>(totalCells) * 6);
        for (int row = 0; row < divY_; ++row) {
            for (int col = 0; col < divX_; ++col) {
                int cellIdx = row * divX_ + col;
                CellType ctype = (cellIdx < static_cast<int>(cellTypes_.size())) ? cellTypes_[cellIdx] : CellType::kGround;

                if (ctype == CellType::kHole) {
                    continue;
                }

                uint32_t lb = static_cast<uint32_t>(row * colCount + col);
                uint32_t rb = static_cast<uint32_t>(row * colCount + col + 1);
                uint32_t lt = static_cast<uint32_t>((row + 1) * colCount + col);
                uint32_t rt = static_cast<uint32_t>((row + 1) * colCount + col + 1);

                if (ctype == CellType::kEdgeTri0) {
                    addTriangle(lb, lt, rb);
                } else if (ctype == CellType::kEdgeTri1) {
                    addTriangle(rb, lt, rt);
                } else {
                    addTriangle(lb, lt, rb);
                    addTriangle(rb, lt, rt);
                }
            }
        }
    }

    // ─── 段差・穴境界・外周の開放辺に崖スカート壁 / シェーダー地平線延長を追加（見た目専用） ────
    {
        struct EdgeInfo {
            uint32_t a   = 0;
            uint32_t b   = 0;
            uint32_t opp = 0; // 三角形の第3頂点（外向き法線判定用）
            int count    = 0;
        };
        std::unordered_map<uint64_t, EdgeInfo> edgeMap;
        edgeMap.reserve(indices_.size());

        auto recordEdge = [&](uint32_t a, uint32_t b, uint32_t opp) {
            uint32_t lo = (std::min)(a, b);
            uint32_t hi = (std::max)(a, b);
            uint64_t key = (static_cast<uint64_t>(lo) << 32) | static_cast<uint64_t>(hi);
            auto& info = edgeMap[key];
            if (info.count == 0) {
                info.a   = a;
                info.b   = b;
                info.opp = opp;
            }
            info.count++;
        };

        const size_t baseTriCount = indices_.size() / 3;
        for (size_t t = 0; t < baseTriCount; ++t) {
            uint32_t i0 = indices_[t * 3 + 0];
            uint32_t i1 = indices_[t * 3 + 1];
            uint32_t i2 = indices_[t * 3 + 2];
            recordEdge(i0, i1, i2);
            recordEdge(i1, i2, i0);
            recordEdge(i2, i0, i1);
        }

        // 地形全体のXZ・Yバウンディングボックスを算出
        float minBX = 1e9f, maxBX = -1e9f, minBY = 1e9f, minBZ = 1e9f, maxBZ = -1e9f;
        for (const auto& v : vertices_) {
            minBX = (std::min)(minBX, v.position.x);
            maxBX = (std::max)(maxBX, v.position.x);
            minBY = (std::min)(minBY, v.position.y);
            minBZ = (std::min)(minBZ, v.position.z);
            maxBZ = (std::max)(maxBZ, v.position.z);
        }
        float centerBX = (minBX + maxBX) * 0.5f;
        float centerBZ = (minBZ + maxBZ) * 0.5f;
        const float edgeTol = (std::max)(1.0f, (std::max)(maxBX - minBX, maxBZ - minBZ) * 0.015f);

        auto isOnBBoxEdge = [&](const VertexData& a, const VertexData& b) -> bool {
            bool onMinX = (std::abs(a.position.x - minBX) < edgeTol && std::abs(b.position.x - minBX) < edgeTol);
            bool onMaxX = (std::abs(a.position.x - maxBX) < edgeTol && std::abs(b.position.x - maxBX) < edgeTol);
            bool onMinZ = (std::abs(a.position.z - minBZ) < edgeTol && std::abs(b.position.z - minBZ) < edgeTol);
            bool onMaxZ = (std::abs(a.position.z - maxBZ) < edgeTol && std::abs(b.position.z - maxBZ) < edgeTol);
            return onMinX || onMaxX || onMinZ || onMaxZ;
        };

        constexpr float kSkirtBottomY = -10.0f;

        // 開放辺 (count == 1) を収集し、連結成分（境界ループ）ごとに分類する
        struct OpenEdge {
            uint32_t a   = 0;
            uint32_t b   = 0;
            uint32_t opp = 0;
            Vector3  outDir = { 0.0f, 0.0f, 1.0f };
            int      compId = -1;
            bool     isOuter = false;
        };
        std::vector<OpenEdge> openEdges;
        openEdges.reserve(edgeMap.size() / 4);

        for (const auto& [key, info] : edgeMap) {
            if (info.count != 1) continue;
            const auto& pA   = vertices_[info.a].position;
            const auto& pB   = vertices_[info.b].position;
            const auto& pOpp = vertices_[info.opp].position;
            if (pA.y <= kSkirtBottomY + 0.05f && pB.y <= kSkirtBottomY + 0.05f) {
                continue;
            }

            // XZ平面で三角形の内側頂点(opp)と反対向きの垂直ベクトルを外向き法線とする
            float edx = pB.x - pA.x;
            float edz = pB.z - pA.z;
            float perpX = -edz;
            float perpZ =  edx;
            float toOppX = pOpp.x - pA.x;
            float toOppZ = pOpp.z - pA.z;
            if (perpX * toOppX + perpZ * toOppZ > 0.0f) {
                perpX = -perpX;
                perpZ = -perpZ;
            }
            float pLenSq = perpX * perpX + perpZ * perpZ;
            Vector3 outN = { 0.0f, 0.0f, 1.0f };
            if (pLenSq > 1e-6f) {
                float inv = 1.0f / std::sqrt(pLenSq);
                outN = { perpX * inv, 0.0f, perpZ * inv };
            } else {
                float rx = (pA.x + pB.x) * 0.5f - centerBX;
                float rz = (pA.z + pB.z) * 0.5f - centerBZ;
                float rLenSq = rx * rx + rz * rz;
                if (rLenSq > 1e-6f) {
                    float inv = 1.0f / std::sqrt(rLenSq);
                    outN = { rx * inv, 0.0f, rz * inv };
                }
            }

            OpenEdge oe;
            oe.a = info.a;
            oe.b = info.b;
            oe.opp = info.opp;
            oe.outDir = outN;
            openEdges.push_back(oe);
        }

        // 頂点インデックスで開放辺を連結し、最大のバウンディング面積を持つループを外周と判定
        std::unordered_map<uint32_t, std::vector<size_t>> vertToEdges;
        for (size_t i = 0; i < openEdges.size(); ++i) {
            vertToEdges[openEdges[i].a].push_back(i);
            vertToEdges[openEdges[i].b].push_back(i);
        }

        int numComps = 0;
        int largestCompId = -1;
        float largestCompArea = -1.0f;
        std::vector<size_t> queue;
        for (size_t i = 0; i < openEdges.size(); ++i) {
            if (openEdges[i].compId != -1) continue;
            int cid = numComps++;
            float cMinX = 1e9f, cMaxX = -1e9f, cMinZ = 1e9f, cMaxZ = -1e9f;
            queue.clear();
            queue.push_back(i);
            openEdges[i].compId = cid;

            for (size_t qIdx = 0; qIdx < queue.size(); ++qIdx) {
                const auto& e = openEdges[queue[qIdx]];
                const auto& pa = vertices_[e.a].position;
                const auto& pb = vertices_[e.b].position;
                cMinX = (std::min)({ cMinX, pa.x, pb.x });
                cMaxX = (std::max)({ cMaxX, pa.x, pb.x });
                cMinZ = (std::min)({ cMinZ, pa.z, pb.z });
                cMaxZ = (std::max)({ cMaxZ, pa.z, pb.z });

                auto pushNeighbors = [&](uint32_t vIdx) {
                    for (size_t nei : vertToEdges[vIdx]) {
                        if (openEdges[nei].compId == -1) {
                            openEdges[nei].compId = cid;
                            queue.push_back(nei);
                        }
                    }
                };
                pushNeighbors(e.a);
                pushNeighbors(e.b);
            }

            float area = (cMaxX - cMinX) * (cMaxZ - cMinZ);
            if (area > largestCompArea) {
                largestCompArea = area;
                largestCompId = cid;
            }
        }

        // 外周頂点ごとの押し出し方向を計算
        // ステージ内側に入り込んでステージを埋めるのを防ぐため、
        // ステージ中心からの放射方向と同じ側を向いている外周エッジのみ延長対象にする
        std::unordered_map<uint32_t, Vector3> smoothOutDir;
        auto calcRadialOut = [&](float px, float pz) -> Vector3 {
            float rx = px - centerBX;
            float rz = pz - centerBZ;
            float rSq = rx * rx + rz * rz;
            if (rSq > 1e-6f) {
                float inv = 1.0f / std::sqrt(rSq);
                return { rx * inv, 0.0f, rz * inv };
            }
            return { 0.0f, 0.0f, 1.0f };
        };

        for (auto& oe : openEdges) {
            bool candidateOuter = (oe.compId == largestCompId || isOnBBoxEdge(vertices_[oe.a], vertices_[oe.b]));
            if (!candidateOuter) continue;

            const auto& pa = vertices_[oe.a].position;
            const auto& pb = vertices_[oe.b].position;
            Vector3 radMid = calcRadialOut((pa.x + pb.x) * 0.5f, (pa.z + pb.z) * 0.5f);
            float dotRad = oe.outDir.x * radMid.x + oe.outDir.z * radMid.z;

            // 外向き法線が中心からの放射方向と逆向き（凹みの内側など）なら延長しない
            if (dotRad > 0.15f) {
                oe.isOuter = true;
                Vector3 rA = calcRadialOut(pa.x, pa.z);
                Vector3 rB = calcRadialOut(pb.x, pb.z);
                smoothOutDir[oe.a].x += rA.x;
                smoothOutDir[oe.a].z += rA.z;
                smoothOutDir[oe.b].x += rB.x;
                smoothOutDir[oe.b].z += rB.z;
            }
        }
        for (auto& [vIdx, dir] : smoothOutDir) {
            float lSq = dir.x * dir.x + dir.z * dir.z;
            if (lSq > 1e-6f) {
                float inv = 1.0f / std::sqrt(lSq);
                dir = { dir.x * inv, 0.0f, dir.z * inv };
            } else {
                const auto& vp = vertices_[vIdx].position;
                dir = calcRadialOut(vp.x, vp.z);
            }
        }

        for (const auto& oe : openEdges) {
            // vertices_.push_back による再確保で参照が無効化されないよう値コピーする
            const VertexData vA = vertices_[oe.a];
            const VertexData vB = vertices_[oe.b];

            // ── 開放辺：崖スカート壁を生成（ステージ上を突き抜ける延長クワッドは生成しない） ──
            Vector3 edgeVec = {
                vB.position.x - vA.position.x,
                vB.position.y - vA.position.y,
                vB.position.z - vA.position.z
            };
            Vector3 outN = oe.outDir;

            float vSpanA = (std::max)(0.2f, (vA.position.y - kSkirtBottomY) * 0.15f);
            float vSpanB = (std::max)(0.2f, (vB.position.y - kSkirtBottomY) * 0.15f);
            float uEdge = std::sqrt(edgeVec.x * edgeVec.x + edgeVec.z * edgeVec.z) * 0.15f;

            uint32_t baseV = static_cast<uint32_t>(vertices_.size());
            VertexData topA = vA;
            topA.normal = outN;
            topA.texCoord = { 0.0f, 0.0f };
            topA.roadAttr = 0.0f;
            topA.extendInfo = { 0.0f, 0.0f, 0.0f };

            VertexData topB = vB;
            topB.normal = outN;
            topB.texCoord = { uEdge, 0.0f };
            topB.roadAttr = 0.0f;
            topB.extendInfo = { 0.0f, 0.0f, 0.0f };

            VertexData botA = topA;
            botA.position.y = kSkirtBottomY;
            botA.texCoord = { 0.0f, vSpanA };

            VertexData botB = topB;
            botB.position.y = kSkirtBottomY;
            botB.texCoord = { uEdge, vSpanB };

            vertices_.push_back(topA);
            vertices_.push_back(topB);
            vertices_.push_back(botA);
            vertices_.push_back(botB);

            indices_.push_back(baseV + 0);
            indices_.push_back(baseV + 2);
            indices_.push_back(baseV + 1);

            indices_.push_back(baseV + 1);
            indices_.push_back(baseV + 2);
            indices_.push_back(baseV + 3);
        }

        // ── 谷底・穴の下から地平線まで途切れないよう、地形最低点のすぐ下にベース草原フロアを追加 ──
        // （頂点シェーダーで horizonExtendDist だけ4隅へ押し出し、ステージ自体は埋めずに地平線まで草原で覆う）
        {
            const float floorY = minBY - 0.1f;
            const float invSqrt2 = 0.70710678f;
            const Vector3 cornerDirs[4] = {
                { -invSqrt2, 0.0f, -invSqrt2 },
                {  invSqrt2, 0.0f, -invSqrt2 },
                { -invSqrt2, 0.0f,  invSqrt2 },
                {  invSqrt2, 0.0f,  invSqrt2 },
            };
            const float cornerX[4] = { minBX, maxBX, minBX, maxBX };
            const float cornerZ[4] = { minBZ, minBZ, maxBZ, maxBZ };

            uint32_t floorBase = static_cast<uint32_t>(vertices_.size());
            for (int c = 0; c < 4; ++c) {
                VertexData fv{};
                fv.position   = { cornerX[c], floorY, cornerZ[c], 1.0f };
                fv.texCoord   = { (cornerX[c] + halfX) / (std::max)(1.0f, sizeX_), (cornerZ[c] + halfZ) / (std::max)(1.0f, sizeY_) };
                fv.normal     = { 0.0f, 1.0f, 0.0f };
                fv.roadAttr   = 0.0f;
                fv.extendInfo = { cornerDirs[c].x, cornerDirs[c].z, 1.0f };
                vertices_.push_back(fv);
            }

            indices_.push_back(floorBase + 0);
            indices_.push_back(floorBase + 2);
            indices_.push_back(floorBase + 1);

            indices_.push_back(floorBase + 1);
            indices_.push_back(floorBase + 2);
            indices_.push_back(floorBase + 3);
        }
    }

    // ─── DirectX12 バッファリソース生成 ───────────────────────────────
    auto device = DXCommon::GetInstance()->GetDevice().Get();

    // 頂点バッファ
    UINT vbSize = static_cast<UINT>(sizeof(VertexData) * vertices_.size());
    vertexResource_ = DXCommon::GetInstance()->CreateBufferResource(vbSize);
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes    = vbSize;
    vertexBufferView_.StrideInBytes  = sizeof(VertexData);

    void* vbData = nullptr;
    vertexResource_->Map(0, nullptr, &vbData);
    memcpy(vbData, vertices_.data(), vbSize);
    vertexResource_->Unmap(0, nullptr);

    // インデックスバッファ
    if (!indices_.empty()) {
        UINT ibSize = static_cast<UINT>(sizeof(uint32_t) * indices_.size());
        indexResource_ = DXCommon::GetInstance()->CreateBufferResource(ibSize);
        indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
        indexBufferView_.SizeInBytes    = ibSize;
        indexBufferView_.Format         = DXGI_FORMAT_R32_UINT;

        void* ibData = nullptr;
        indexResource_->Map(0, nullptr, &ibData);
        memcpy(ibData, indices_.data(), ibSize);
        indexResource_->Unmap(0, nullptr);
    }

    // マテリアルバッファ
    materialResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(MaterialData));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
    materialData_->color     = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->uvTile    = uvTile_;
    materialData_->cameraPos = { 0.0f, 0.0f, 0.0f };

    // WVP 定数バッファ
    wvpResource_ = DXCommon::GetInstance()->CreateBufferResource(sizeof(TransformationMatrix));
    wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
}

void Terrain::Update()
{
    Matrix4x4 world = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    Matrix4x4 wvp   = {};

    if (camera_) {
        wvp = Multiply(world, camera_->GetViewProtectionMatrix());
    } else {
        wvp = Multiply(world, Makeidentity4x4());
    }

    if (materialData_ && camera_) {
        materialData_->cameraPos = camera_->GetTranslate();
    }

    if (wvpData_) {
        wvpData_->WVP   = wvp;
        wvpData_->World = world;
        wvpData_->WorldInverseTranspose = Transpose(Inverse(world));
        wvpData_->horizonExtendDist = 75.0f;
        wvpData_->horizonDropY      = 0.0f;
        wvpData_->sizeX             = sizeX_;
        wvpData_->sizeY             = sizeY_;
    }
}

void Terrain::Draw()
{
    if (indices_.empty()) return;

    auto cmdList = DXCommon::GetInstance()->GetCommandList();
    auto psoSet  = PSOManager::GetInstance()->GetPso("Terrain", BlendMode::Normal, FillMode::kSolid);

    cmdList->SetPipelineState(psoSet.pipelineState.Get());
    cmdList->SetGraphicsRootSignature(psoSet.rootSignature.Get());

    cmdList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    cmdList->IASetIndexBuffer(&indexBufferView_);
    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // [Param 0] Material (CBV b0)
    cmdList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // [Param 1] Transform (CBV b1)
    cmdList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());

    // [Param 2] Grass Texture (DescriptorTable t0)
    cmdList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(grassTexIndex_));

    // [Param 3] Road Texture (DescriptorTable t1)
    cmdList->SetGraphicsRootDescriptorTable(3, TextureManager::GetInstance()->GetSrvHandleGPU(roadTexIndex_));

    cmdList->DrawIndexedInstanced(static_cast<UINT>(indices_.size()), 1, 0, 0, 0);
}

Terrain::CellType Terrain::GetSurfaceType(const Vector3& worldPos) const
{
    // ワールド座標からローカル座標へ逆変換
    float lx = worldPos.x - transform_.translate.x;
    float lz = worldPos.z - transform_.translate.z;

    float halfX = sizeX_ * 0.5f;
    float halfZ = sizeY_ * 0.5f;

    if (lx < -halfX || lx > halfX || lz < -halfZ || lz > halfZ) {
        return CellType::kGround;
    }

    float tx = (lx + halfX) / sizeX_;
    float tz = (lz + halfZ) / sizeY_;

    int col = std::clamp(static_cast<int>(tx * divX_), 0, divX_ - 1);
    int row = std::clamp(static_cast<int>(tz * divY_), 0, divY_ - 1);
    int idx = row * divX_ + col;

    if (idx >= 0 && idx < static_cast<int>(cellTypes_.size())) {
        return cellTypes_[idx];
    }

    return CellType::kGround;
}

void Terrain::ApplyRailRoad(const RailPath* rail, float roadHalfWidth)
{
    if (!rail || vertices_.empty() || !vertexResource_) {
        return;
    }

    float totalLen = rail->GetTotalLength();
    if (totalLen <= 0.001f) {
        return;
    }

    // レールを 0.5m 間隔でサンプリング（XZ平面）
    std::vector<Vector3> railPts;
    const float step = 0.5f;
    for (float d = 0.0f; d <= totalLen; d += step) {
        railPts.push_back(rail->GetPosition(rail->GetTFromDistance(d)));
    }
    railPts.push_back(rail->GetPosition(rail->GetTFromDistance(totalLen)));

    Matrix4x4 worldMat = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
    const float innerR = roadHalfWidth * 0.72f;
    const float outerR = roadHalfWidth * 1.35f;
    const float blendSpan = (std::max)(0.05f, outerR - innerR);

    for (auto& v : vertices_) {
        Vector3 localPos = { v.position.x, v.position.y, v.position.z };
        Vector3 wPos = vector3Transform(localPos, worldMat);

        // スカート底部や地平線延長頂点などには道テクスチャを適用しない
        if (v.extendInfo.z > 0.5f || wPos.y < -3.0f) {
            continue;
        }

        float minDistSq = 1e12f;
        for (const auto& rp : railPts) {
            float dx = wPos.x - rp.x;
            float dz = wPos.z - rp.z;
            float dSq = dx * dx + dz * dz;
            if (dSq < minDistSq) {
                minDistSq = dSq;
            }
        }

        float dist = std::sqrt(minDistSq);
        if (dist <= outerR) {
            float attr = std::clamp(1.0f - (dist - innerR) / blendSpan, 0.0f, 1.0f);
            v.roadAttr = (std::max)(v.roadAttr, attr);
        }
    }

    // 頂点バッファをGPUへ再アップロード
    void* vbData = nullptr;
    vertexResource_->Map(0, nullptr, &vbData);
    memcpy(vbData, vertices_.data(), sizeof(VertexData) * vertices_.size());
    vertexResource_->Unmap(0, nullptr);
}
