#include "Physics.h"
#include "Collider.h"
#include "GameObject.h"
#include <cmath>
#include <algorithm>

const std::vector<Triangle>* Physics::s_triangles_ = nullptr;
const std::vector<Collider*>* Physics::s_colliders_ = nullptr;

void Physics::SetTriangles(const std::vector<Triangle>* triangles) {
    s_triangles_ = triangles;
}

void Physics::SetColliders(const std::vector<Collider*>* colliders) {
    s_colliders_ = colliders;
}

void Physics::Clear() {
    s_triangles_ = nullptr;
    s_colliders_ = nullptr;
}

uint32_t Physics::GetTriangleLayer(const Triangle& tri, const Vector3& normal) {
    if (tri.isOneway) {
        return CollisionLayer::OneWay;
    }
    // 緩やかな上面（歩行可能面、cos約45度以上）はGround
    if (normal.y >= 0.7f) {
        return CollisionLayer::Ground;
    }
    // 垂直壁または急斜面はWall
    return CollisionLayer::Wall;
}

// Christer Ericson「Real-Time Collision Detection」に基づく点と三角形の最近接点
Vector3 Physics::ClosestPointOnTriangle(const Vector3& p, const Triangle& tri) {
    const Vector3& a = tri.vertices[0];
    const Vector3& b = tri.vertices[1];
    const Vector3& c = tri.vertices[2];

    Vector3 ab = Subtract(b, a);
    Vector3 ac = Subtract(c, a);
    Vector3 ap = Subtract(p, a);

    float d1 = Dot(ab, ap);
    float d2 = Dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) return a; // 頂点Aのボロノイ領域

    Vector3 bp = Subtract(p, b);
    float d3 = Dot(ab, bp);
    float d4 = Dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) return b; // 頂点Bのボロノイ領域

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v = d1 / (d1 - d3);
        return Add(a, Multiply(v, ab)); // 辺ABのボロノイ領域
    }

    Vector3 cp = Subtract(p, c);
    float d5 = Dot(ab, cp);
    float d6 = Dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) return c; // 頂点Cのボロノイ領域

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w = d2 / (d2 - d6);
        return Add(a, Multiply(w, ac)); // 辺ACのボロノイ領域
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return Add(b, Multiply(w, Subtract(c, b))); // 辺BCのボロノイ領域
    }

    // 三角形面領域内
    float denom = 1.0f / (va + vb + vc);
    float v = vb * denom;
    float w = vc * denom;
    return Add(a, Add(Multiply(v, ab), Multiply(w, ac)));
}

// ─── 幾何形状交差判定 ──────────────────────────────────────────

bool Physics::Intersect(const Sphere& a, const Sphere& b) {
    Vector3 diff = Subtract(a.center, b.center);
    float distSq = Dot(diff, diff);
    float r = a.radius + b.radius;
    return distSq <= (r * r);
}

bool Physics::Intersect(const AABB& a, const AABB& b) {
    if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
    if (a.max.z < b.min.z || a.min.z > b.max.z) return false;
    return true;
}

bool Physics::Intersect(const Sphere& sphere, const AABB& aabb) {
    Vector3 closestPoint = {
        (std::max)(aabb.min.x, (std::min)(sphere.center.x, aabb.max.x)),
        (std::max)(aabb.min.y, (std::min)(sphere.center.y, aabb.max.y)),
        (std::max)(aabb.min.z, (std::min)(sphere.center.z, aabb.max.z))
    };
    Vector3 diff = Subtract(sphere.center, closestPoint);
    return Dot(diff, diff) <= (sphere.radius * sphere.radius);
}

bool Physics::Intersect(const AABB& aabb, const Sphere& sphere) {
    return Intersect(sphere, aabb);
}

bool Physics::Intersect(const Sphere& sphere, const Triangle& triangle, Vector3* outClosestPoint) {
    Vector3 closest = ClosestPointOnTriangle(sphere.center, triangle);
    if (outClosestPoint) *outClosestPoint = closest;
    Vector3 diff = Subtract(sphere.center, closest);
    return Dot(diff, diff) <= (sphere.radius * sphere.radius);
}

// 分離軸定理 (SAT) による AABB vs Triangle 判定
bool Physics::Intersect(const AABB& aabb, const Triangle& triangle) {
    Vector3 boxCenter = Multiply(0.5f, Add(aabb.min, aabb.max));
    Vector3 boxExtents = Multiply(0.5f, Subtract(aabb.max, aabb.min));

    // AABB中心を原点に平行移動
    Vector3 v0 = Subtract(triangle.vertices[0], boxCenter);
    Vector3 v1 = Subtract(triangle.vertices[1], boxCenter);
    Vector3 v2 = Subtract(triangle.vertices[2], boxCenter);

    Vector3 f0 = Subtract(v1, v0);
    Vector3 f1 = Subtract(v2, v1);
    Vector3 f2 = Subtract(v0, v2);

    // 1. AABBの3軸に対する三角形AABBとの重なり判定
    Vector3 triMin = {
        (std::min)({v0.x, v1.x, v2.x}),
        (std::min)({v0.y, v1.y, v2.y}),
        (std::min)({v0.z, v1.z, v2.z})
    };
    Vector3 triMax = {
        (std::max)({v0.x, v1.x, v2.x}),
        (std::max)({v0.y, v1.y, v2.y}),
        (std::max)({v0.z, v1.z, v2.z})
    };
    if (triMin.x > boxExtents.x || triMax.x < -boxExtents.x) return false;
    if (triMin.y > boxExtents.y || triMax.y < -boxExtents.y) return false;
    if (triMin.z > boxExtents.z || triMax.z < -boxExtents.z) return false;

    // 2. 三角形法線軸に対する判定
    Vector3 normal = Normalize(Cross(f0, f1));
    if (Length(normal) > 0.0001f) {
        float r = boxExtents.x * std::abs(normal.x) +
                  boxExtents.y * std::abs(normal.y) +
                  boxExtents.z * std::abs(normal.z);
        float s = Dot(normal, v0);
        if (std::abs(s) > r) return false;
    }

    // 3. AABB各軸と三角形各辺の外積軸 (9軸)
    auto TestAxis = [&](float a, float b, float fa, float fb, float p0, float p1, float rad) {
        float minP = (std::min)(p0, p1);
        float maxP = (std::max)(p0, p1);
        return !(minP > rad || maxP < -rad);
    };

    // Axis e0 x f0
    float rad = boxExtents.y * std::abs(f0.z) + boxExtents.z * std::abs(f0.y);
    float p0 = v0.z * f0.y - v0.y * f0.z;
    float p2 = v2.z * f0.y - v2.y * f0.z;
    if ((std::min)(p0, p2) > rad || (std::max)(p0, p2) < -rad) return false;

    // Axis e0 x f1
    rad = boxExtents.y * std::abs(f1.z) + boxExtents.z * std::abs(f1.y);
    p0 = v0.z * f1.y - v0.y * f1.z;
    float p1 = v1.z * f1.y - v1.y * f1.z;
    if ((std::min)(p0, p1) > rad || (std::max)(p0, p1) < -rad) return false;

    // Axis e0 x f2
    rad = boxExtents.y * std::abs(f2.z) + boxExtents.z * std::abs(f2.y);
    p0 = v0.z * f2.y - v0.y * f2.z;
    p1 = v1.z * f2.y - v1.y * f2.z;
    if ((std::min)(p0, p1) > rad || (std::max)(p0, p1) < -rad) return false;

    // Axis e1 x f0
    rad = boxExtents.x * std::abs(f0.z) + boxExtents.z * std::abs(f0.x);
    p0 = -v0.z * f0.x + v0.x * f0.z;
    p2 = -v2.z * f0.x + v2.x * f0.z;
    if ((std::min)(p0, p2) > rad || (std::max)(p0, p2) < -rad) return false;

    // Axis e1 x f1
    rad = boxExtents.x * std::abs(f1.z) + boxExtents.z * std::abs(f1.x);
    p0 = -v0.z * f1.x + v0.x * f1.z;
    p1 = -v1.z * f1.x + v1.x * f1.z;
    if ((std::min)(p0, p1) > rad || (std::max)(p0, p1) < -rad) return false;

    // Axis e1 x f2
    rad = boxExtents.x * std::abs(f2.z) + boxExtents.z * std::abs(f2.x);
    p0 = -v0.z * f2.x + v0.x * f2.z;
    p1 = -v1.z * f2.x + v1.x * f2.z;
    if ((std::min)(p0, p1) > rad || (std::max)(p0, p1) < -rad) return false;

    // Axis e2 x f0
    rad = boxExtents.x * std::abs(f0.y) + boxExtents.y * std::abs(f0.x);
    p0 = v0.y * f0.x - v0.x * f0.y;
    p2 = v2.y * f0.x - v2.x * f0.y;
    if ((std::min)(p0, p2) > rad || (std::max)(p0, p2) < -rad) return false;

    // Axis e2 x f1
    rad = boxExtents.x * std::abs(f1.y) + boxExtents.y * std::abs(f1.x);
    p0 = v0.y * f1.x - v0.x * f1.y;
    p1 = v1.y * f1.x - v1.x * f1.y;
    if ((std::min)(p0, p1) > rad || (std::max)(p0, p1) < -rad) return false;

    // Axis e2 x f2
    rad = boxExtents.x * std::abs(f2.y) + boxExtents.y * std::abs(f2.x);
    p0 = v0.y * f2.x - v0.x * f2.y;
    p1 = v1.y * f2.x - v1.x * f2.y;
    if ((std::min)(p0, p1) > rad || (std::max)(p0, p1) < -rad) return false;

    return true;
}

bool Physics::Intersect(const Ray& ray, const Sphere& sphere, float* outDistance, Vector3* outHitPoint) {
    Vector3 m = Subtract(ray.origin, sphere.center);
    Vector3 dir = (Length(ray.diff) > 0.0001f) ? Normalize(ray.diff) : Vector3{ 0,0,1 };
    float b = Dot(m, dir);
    float c = Dot(m, m) - (sphere.radius * sphere.radius);

    if (c > 0.0f && b > 0.0f) return false; // レイの起点が球の外側にあり、レイが球から離れる向き
    float discr = b * b - c;
    if (discr < 0.0f) return false; // 虚数解（交差なし）

    float t = -b - std::sqrt(discr);
    if (t < 0.0f) t = 0.0f; // 球の内側から発射された場合

    if (outDistance) *outDistance = t;
    if (outHitPoint) *outHitPoint = Add(ray.origin, Multiply(t, dir));
    return true;
}

bool Physics::Intersect(const Ray& ray, const AABB& aabb, float* outDistance, Vector3* outHitPoint) {
    Vector3 dir = (Length(ray.diff) > 0.0001f) ? Normalize(ray.diff) : Vector3{ 0,0,1 };
    float tmin = 0.0f;
    float tmax = FLT_MAX;

    // X軸スラブ
    if (std::abs(dir.x) < 1e-6f) {
        if (ray.origin.x < aabb.min.x || ray.origin.x > aabb.max.x) return false;
    } else {
        float invD = 1.0f / dir.x;
        float t1 = (aabb.min.x - ray.origin.x) * invD;
        float t2 = (aabb.max.x - ray.origin.x) * invD;
        if (t1 > t2) std::swap(t1, t2);
        tmin = (std::max)(tmin, t1);
        tmax = (std::min)(tmax, t2);
        if (tmin > tmax) return false;
    }

    // Y軸スラブ
    if (std::abs(dir.y) < 1e-6f) {
        if (ray.origin.y < aabb.min.y || ray.origin.y > aabb.max.y) return false;
    } else {
        float invD = 1.0f / dir.y;
        float t1 = (aabb.min.y - ray.origin.y) * invD;
        float t2 = (aabb.max.y - ray.origin.y) * invD;
        if (t1 > t2) std::swap(t1, t2);
        tmin = (std::max)(tmin, t1);
        tmax = (std::min)(tmax, t2);
        if (tmin > tmax) return false;
    }

    // Z軸スラブ
    if (std::abs(dir.z) < 1e-6f) {
        if (ray.origin.z < aabb.min.z || ray.origin.z > aabb.max.z) return false;
    } else {
        float invD = 1.0f / dir.z;
        float t1 = (aabb.min.z - ray.origin.z) * invD;
        float t2 = (aabb.max.z - ray.origin.z) * invD;
        if (t1 > t2) std::swap(t1, t2);
        tmin = (std::max)(tmin, t1);
        tmax = (std::min)(tmax, t2);
        if (tmin > tmax) return false;
    }

    if (outDistance) *outDistance = tmin;
    if (outHitPoint) *outHitPoint = Add(ray.origin, Multiply(tmin, dir));
    return true;
}

namespace {
    // 内部計算用の正規化済みレイとAABBの高速スラブ交差判定
    inline bool FastRayAABBIntersect(const Ray& normRay, const AABB& aabb, float maxDist, float* outTmin = nullptr) {
        float tmin = 0.0f;
        float tmax = maxDist;

        // X軸
        if (std::abs(normRay.diff.x) < 1e-6f) {
            if (normRay.origin.x < aabb.min.x || normRay.origin.x > aabb.max.x) return false;
        } else {
            float invD = 1.0f / normRay.diff.x;
            float t1 = (aabb.min.x - normRay.origin.x) * invD;
            float t2 = (aabb.max.x - normRay.origin.x) * invD;
            if (t1 > t2) std::swap(t1, t2);
            tmin = (std::max)(tmin, t1);
            tmax = (std::min)(tmax, t2);
            if (tmin > tmax) return false;
        }

        // Y軸
        if (std::abs(normRay.diff.y) < 1e-6f) {
            if (normRay.origin.y < aabb.min.y || normRay.origin.y > aabb.max.y) return false;
        } else {
            float invD = 1.0f / normRay.diff.y;
            float t1 = (aabb.min.y - normRay.origin.y) * invD;
            float t2 = (aabb.max.y - normRay.origin.y) * invD;
            if (t1 > t2) std::swap(t1, t2);
            tmin = (std::max)(tmin, t1);
            tmax = (std::min)(tmax, t2);
            if (tmin > tmax) return false;
        }

        // Z軸
        if (std::abs(normRay.diff.z) < 1e-6f) {
            if (normRay.origin.z < aabb.min.z || normRay.origin.z > aabb.max.z) return false;
        } else {
            float invD = 1.0f / normRay.diff.z;
            float t1 = (aabb.min.z - normRay.origin.z) * invD;
            float t2 = (aabb.max.z - normRay.origin.z) * invD;
            if (t1 > t2) std::swap(t1, t2);
            tmin = (std::max)(tmin, t1);
            tmax = (std::min)(tmax, t2);
            if (tmin > tmax) return false;
        }

        if (outTmin) *outTmin = tmin;
        return true;
    }
}

// ─── 1. レイキャスト判定 (Raycast) ───────────────────────────

bool Physics::Raycast(const Ray& ray, RaycastHit* hitInfo, float maxDistance, uint32_t layerMask) {
    if (!s_triangles_) return false;
    return Raycast(ray, *s_triangles_, hitInfo, maxDistance, layerMask);
}

bool Physics::Raycast(const Vector3& origin, const Vector3& direction, RaycastHit* hitInfo, float maxDistance, uint32_t layerMask) {
    Vector3 dir = (Length(direction) > 0.0001f) ? Normalize(direction) : Vector3{ 0,0,1 };
    Ray ray;
    ray.origin = origin;
    ray.diff = dir;
    return Raycast(ray, hitInfo, maxDistance, layerMask);
}

bool Physics::Raycast(const Ray& ray, const std::vector<Triangle>& triangles, RaycastHit* hitInfo, float maxDistance, uint32_t layerMask) {
    if (hitInfo) {
        hitInfo->hit = false;
        hitInfo->distance = FLT_MAX;
    }

    float rayLen = Length(ray.diff);
    Vector3 dir = (rayLen > 0.0001f) ? Normalize(ray.diff) : Vector3{ 0,0,1 };

    // 内部計算用のレイ（長さ1に正規化）
    Ray normRay;
    normRay.origin = ray.origin;
    normRay.diff = dir;

    bool found = false;
    float closestDist = maxDistance;
    RaycastHit bestHit;

    // レイの到達範囲を表す外接AABB（探索ボリューム）を計算するラムダ
    auto CalcRayAABB = [&](float maxDist) -> AABB {
        Vector3 endPoint = Add(normRay.origin, Multiply(maxDist, normRay.diff));
        AABB box;
        box.min.x = (std::min)(normRay.origin.x, endPoint.x);
        box.min.y = (std::min)(normRay.origin.y, endPoint.y);
        box.min.z = (std::min)(normRay.origin.z, endPoint.z);
        box.max.x = (std::max)(normRay.origin.x, endPoint.x);
        box.max.y = (std::max)(normRay.origin.y, endPoint.y);
        box.max.z = (std::max)(normRay.origin.z, endPoint.z);
        return box;
    };

    AABB rayAABB = CalcRayAABB(closestDist);

    for (const auto& tri : triangles) {
        // 【AABBカリング Step 1】三角形のAABBを取得
        AABB triAABB = GetTriangleAABB(tri);

        // 【AABBカリング Step 2】レイ探索AABBと三角形AABBが重なっていなければ即破棄（float大小比較のみ）
        if (!IsOverlapAABB(rayAABB, triAABB)) {
            continue;
        }

        // 【AABBカリング Step 3】レイと三角形AABBのスラブ判定（ボックス自体を外れるものは除外）
        float boxDist = 0.0f;
        if (!FastRayAABBIntersect(normRay, triAABB, closestDist, &boxDist)) {
            continue;
        }
        if (boxDist > closestDist) {
            continue;
        }

        // 【詳細判定】AABBを通過した三角形のみ高コストな三角形交差計算を実行
        float t = 0.0f;
        Vector3 hitPoint = {};
        RayTriangleCollisionResult result = RayTriangleCollisionResult::NoCollision;

        if (CheckRayTriangle(normRay, tri, &t, &hitPoint, &result)) {
            if (t >= 0.0f && t <= closestDist) {
                Vector3 v01 = Subtract(tri.vertices[1], tri.vertices[0]);
                Vector3 v12 = Subtract(tri.vertices[2], tri.vertices[1]);
                Vector3 normal = Normalize(Cross(v01, v12));
                if (result == RayTriangleCollisionResult::BackFace) {
                    normal = Multiply(-1.0f, normal);
                }

                uint32_t triLayer = GetTriangleLayer(tri, normal);
                if ((triLayer & layerMask) == 0) {
                    continue; // レイヤーマスク対象外
                }

                closestDist = t;
                found = true;

                // 【動的枝刈り】より近い面が見つかったので、レイAABBの探索範囲を縮小
                rayAABB = CalcRayAABB(closestDist);

                bestHit.hit = true;
                bestHit.distance = t;
                bestHit.point = hitPoint;
                bestHit.normal = normal;
                bestHit.triangle = tri;
                bestHit.layer = triLayer;
                bestHit.faceResult = result;
                bestHit.collider = nullptr;
                bestHit.gameObject = nullptr;
            }
        }
    }

    // コライダーリストが登録されている場合はコライダーとの交差もチェック
    if (s_colliders_) {
        for (auto* col : *s_colliders_) {
            if (!col || !col->IsCollide()) continue;

            uint32_t colLayer = CollisionLayer::Default;
            if (col->GetCategory() == CollisionCategory::Player) colLayer = CollisionLayer::Player;
            else if (col->GetCategory() == CollisionCategory::Enemy) colLayer = CollisionLayer::Enemy;
            else if (col->GetCategory() == CollisionCategory::PlayerProjectile || col->GetCategory() == CollisionCategory::EnemyProjectile) colLayer = CollisionLayer::Projectile;

            if ((colLayer & layerMask) == 0) continue;

            auto spheres = col->GetWorldSpheres();
            for (const auto& sphere : spheres) {
                float dist = 0.0f;
                Vector3 hitPoint = {};
                if (Intersect(normRay, sphere, &dist, &hitPoint)) {
                    if (dist >= 0.0f && dist < closestDist) {
                        closestDist = dist;
                        found = true;

                        bestHit.hit = true;
                        bestHit.distance = dist;
                        bestHit.point = hitPoint;
                        bestHit.normal = (dist > 0.0001f) ? Normalize(Subtract(hitPoint, sphere.center)) : Vector3{ 0,1,0 };
                        bestHit.collider = col;
                        bestHit.gameObject = col->GetOwner();
                        bestHit.layer = colLayer;
                        bestHit.faceResult = RayTriangleCollisionResult::FrontFace;
                    }
                }
            }
        }
    }

    if (found && hitInfo) {
        *hitInfo = bestHit;
    }
    return found;
}

std::vector<RaycastHit> Physics::RaycastAll(const Ray& ray, float maxDistance, uint32_t layerMask) {
    if (!s_triangles_) return {};
    return RaycastAll(ray, *s_triangles_, maxDistance, layerMask);
}

std::vector<RaycastHit> Physics::RaycastAll(const Vector3& origin, const Vector3& direction, float maxDistance, uint32_t layerMask) {
    Vector3 dir = (Length(direction) > 0.0001f) ? Normalize(direction) : Vector3{ 0,0,1 };
    Ray ray;
    ray.origin = origin;
    ray.diff = dir;
    return RaycastAll(ray, maxDistance, layerMask);
}

std::vector<RaycastHit> Physics::RaycastAll(const Ray& ray, const std::vector<Triangle>& triangles, float maxDistance, uint32_t layerMask) {
    std::vector<RaycastHit> hits;

    Vector3 dir = (Length(ray.diff) > 0.0001f) ? Normalize(ray.diff) : Vector3{ 0,0,1 };
    Ray normRay;
    normRay.origin = ray.origin;
    normRay.diff = dir;

    // レイ全体の探索AABBを事前計算
    Vector3 endPoint = Add(normRay.origin, Multiply(maxDistance, normRay.diff));
    AABB rayAABB;
    rayAABB.min.x = (std::min)(normRay.origin.x, endPoint.x);
    rayAABB.min.y = (std::min)(normRay.origin.y, endPoint.y);
    rayAABB.min.z = (std::min)(normRay.origin.z, endPoint.z);
    rayAABB.max.x = (std::max)(normRay.origin.x, endPoint.x);
    rayAABB.max.y = (std::max)(normRay.origin.y, endPoint.y);
    rayAABB.max.z = (std::max)(normRay.origin.z, endPoint.z);

    for (const auto& tri : triangles) {
        // 【AABBカリング Step 1】三角形のAABBを取得
        AABB triAABB = GetTriangleAABB(tri);

        // 【AABBカリング Step 2】レイ探索AABBと三角形AABBのオーバーラップ判定
        if (!IsOverlapAABB(rayAABB, triAABB)) {
            continue;
        }

        // 【AABBカリング Step 3】レイと三角形AABBのスラブ判定
        float boxDist = 0.0f;
        if (!FastRayAABBIntersect(normRay, triAABB, maxDistance, &boxDist)) {
            continue;
        }
        if (boxDist > maxDistance) {
            continue;
        }

        // 【詳細判定】AABB通過三角形のみ交差判定を実行
        float t = 0.0f;
        Vector3 hitPoint = {};
        RayTriangleCollisionResult result = RayTriangleCollisionResult::NoCollision;

        if (CheckRayTriangle(normRay, tri, &t, &hitPoint, &result)) {
            if (t >= 0.0f && t <= maxDistance) {
                Vector3 v01 = Subtract(tri.vertices[1], tri.vertices[0]);
                Vector3 v12 = Subtract(tri.vertices[2], tri.vertices[1]);
                Vector3 normal = Normalize(Cross(v01, v12));
                if (result == RayTriangleCollisionResult::BackFace) {
                    normal = Multiply(-1.0f, normal);
                }

                uint32_t triLayer = GetTriangleLayer(tri, normal);
                if ((triLayer & layerMask) == 0) continue;

                RaycastHit hit;
                hit.hit = true;
                hit.distance = t;
                hit.point = hitPoint;
                hit.normal = normal;
                hit.triangle = tri;
                hit.layer = triLayer;
                hit.faceResult = result;
                hits.push_back(hit);
            }
        }
    }

    if (s_colliders_) {
        for (auto* col : *s_colliders_) {
            if (!col || !col->IsCollide()) continue;

            uint32_t colLayer = CollisionLayer::Default;
            if (col->GetCategory() == CollisionCategory::Player) colLayer = CollisionLayer::Player;
            else if (col->GetCategory() == CollisionCategory::Enemy) colLayer = CollisionLayer::Enemy;
            else if (col->GetCategory() == CollisionCategory::PlayerProjectile || col->GetCategory() == CollisionCategory::EnemyProjectile) colLayer = CollisionLayer::Projectile;

            if ((colLayer & layerMask) == 0) continue;

            auto spheres = col->GetWorldSpheres();
            for (const auto& sphere : spheres) {
                float dist = 0.0f;
                Vector3 hitPoint = {};
                if (Intersect(normRay, sphere, &dist, &hitPoint)) {
                    if (dist >= 0.0f && dist <= maxDistance) {
                        RaycastHit hit;
                        hit.hit = true;
                        hit.distance = dist;
                        hit.point = hitPoint;
                        hit.normal = (dist > 0.0001f) ? Normalize(Subtract(hitPoint, sphere.center)) : Vector3{ 0,1,0 };
                        hit.collider = col;
                        hit.gameObject = col->GetOwner();
                        hit.layer = colLayer;
                        hits.push_back(hit);
                    }
                }
            }
        }
    }

    // 距離昇順にソート
    std::sort(hits.begin(), hits.end(), [](const RaycastHit& a, const RaycastHit& b) {
        return a.distance < b.distance;
    });

    return hits;
}

// ─── 2. 球体判定 (Sphere) ───────────────────────────────────

bool Physics::SphereCast(const Vector3& origin, float radius, const Vector3& direction, RaycastHit* hitInfo, float maxDistance, uint32_t layerMask) {
    if (!s_triangles_) return false;
    return SphereCast(origin, radius, direction, *s_triangles_, hitInfo, maxDistance, layerMask);
}

bool Physics::SphereCast(const Vector3& origin, float radius, const Vector3& direction, const std::vector<Triangle>& triangles, RaycastHit* hitInfo, float maxDistance, uint32_t layerMask) {
    if (hitInfo) {
        hitInfo->hit = false;
        hitInfo->distance = FLT_MAX;
    }

    Vector3 dir = (Length(direction) > 0.0001f) ? Normalize(direction) : Vector3{ 0,0,1 };
    bool found = false;
    float closestDist = maxDistance;
    RaycastHit bestHit;

    // 球面と三角形の掃引判定:
    // 三角形の面を法線方向に radius だけオフセットした面へのレイキャスト ＋ 辺や頂点への最近接クランプ
    for (const auto& tri : triangles) {
        Vector3 v01 = Subtract(tri.vertices[1], tri.vertices[0]);
        Vector3 v12 = Subtract(tri.vertices[2], tri.vertices[1]);
        Vector3 normal = Normalize(Cross(v01, v12));
        if (Length(normal) < 0.0001f) continue;

        // 法線との内積
        float dot = Dot(dir, normal);
        if (dot >= 0.0f) {
            // 裏面の場合は法線を反転
            normal = Multiply(-1.0f, normal);
            dot = Dot(dir, normal);
            if (dot >= 0.0f) continue;
        }

        uint32_t triLayer = GetTriangleLayer(tri, normal);
        if ((triLayer & layerMask) == 0) continue;

        // 面を半径分だけ手前に押し出した平面に対する交差距離
        float planeD = Dot(tri.vertices[0], normal) + radius;
        float t = (planeD - Dot(origin, normal)) / dot;

        if (t >= 0.0f && t < closestDist) {
            Vector3 sphereCenterAtT = Add(origin, Multiply(t, dir));
            Vector3 closest = ClosestPointOnTriangle(sphereCenterAtT, tri);
            Vector3 diff = Subtract(sphereCenterAtT, closest);

            if (Dot(diff, diff) <= (radius * radius * 1.05f)) {
                closestDist = t;
                found = true;

                bestHit.hit = true;
                bestHit.distance = t;
                bestHit.point = closest;
                bestHit.normal = (Length(diff) > 0.0001f) ? Normalize(diff) : normal;
                bestHit.triangle = tri;
                bestHit.layer = triLayer;
            }
        }
    }

    if (found && hitInfo) {
        *hitInfo = bestHit;
    }
    return found;
}

bool Physics::CheckSphere(const Vector3& position, float radius, uint32_t layerMask) {
    Sphere s = { position, radius };

    if (s_triangles_) {
        for (const auto& tri : *s_triangles_) {
            Vector3 v01 = Subtract(tri.vertices[1], tri.vertices[0]);
            Vector3 v12 = Subtract(tri.vertices[2], tri.vertices[1]);
            Vector3 normal = Normalize(Cross(v01, v12));
            uint32_t triLayer = GetTriangleLayer(tri, normal);
            if ((triLayer & layerMask) == 0) continue;

            if (Intersect(s, tri)) return true;
        }
    }

    if (s_colliders_) {
        for (auto* col : *s_colliders_) {
            if (!col || !col->IsCollide()) continue;
            uint32_t colLayer = CollisionLayer::Default;
            if (col->GetCategory() == CollisionCategory::Player) colLayer = CollisionLayer::Player;
            else if (col->GetCategory() == CollisionCategory::Enemy) colLayer = CollisionLayer::Enemy;
            if ((colLayer & layerMask) == 0) continue;

            auto spheres = col->GetWorldSpheres();
            for (const auto& cs : spheres) {
                if (Intersect(s, cs)) return true;
            }
        }
    }

    return false;
}

std::vector<Collider*> Physics::OverlapSphere(const Vector3& position, float radius, uint32_t layerMask) {
    std::vector<Collider*> result;
    if (!s_colliders_) return result;

    Sphere s = { position, radius };
    for (auto* col : *s_colliders_) {
        if (!col || !col->IsCollide()) continue;

        uint32_t colLayer = CollisionLayer::Default;
        if (col->GetCategory() == CollisionCategory::Player) colLayer = CollisionLayer::Player;
        else if (col->GetCategory() == CollisionCategory::Enemy) colLayer = CollisionLayer::Enemy;
        if ((colLayer & layerMask) == 0) continue;

        auto spheres = col->GetWorldSpheres();
        for (const auto& cs : spheres) {
            if (Intersect(s, cs)) {
                result.push_back(col);
                break;
            }
        }
    }
    return result;
}

// ─── 3. ボックス・AABB判定 (Box / AABB) ──────────────────────

bool Physics::CheckBox(const Vector3& center, const Vector3& halfExtents, uint32_t layerMask) {
    AABB aabb = {
        Subtract(center, halfExtents),
        Add(center, halfExtents)
    };
    return CheckAABB(aabb, layerMask);
}

bool Physics::CheckAABB(const AABB& aabb, uint32_t layerMask) {
    if (s_triangles_) {
        for (const auto& tri : *s_triangles_) {
            Vector3 v01 = Subtract(tri.vertices[1], tri.vertices[0]);
            Vector3 v12 = Subtract(tri.vertices[2], tri.vertices[1]);
            Vector3 normal = Normalize(Cross(v01, v12));
            uint32_t triLayer = GetTriangleLayer(tri, normal);
            if ((triLayer & layerMask) == 0) continue;

            if (Intersect(aabb, tri)) return true;
        }
    }

    if (s_colliders_) {
        for (auto* col : *s_colliders_) {
            if (!col || !col->IsCollide()) continue;
            uint32_t colLayer = CollisionLayer::Default;
            if (col->GetCategory() == CollisionCategory::Player) colLayer = CollisionLayer::Player;
            else if (col->GetCategory() == CollisionCategory::Enemy) colLayer = CollisionLayer::Enemy;
            if ((colLayer & layerMask) == 0) continue;

            Sphere colSphere = { col->GetWorldPosition(), col->GetRadius() };
            if (Intersect(aabb, colSphere)) return true;
        }
    }

    return false;
}

std::vector<Collider*> Physics::OverlapBox(const Vector3& center, const Vector3& halfExtents, uint32_t layerMask) {
    AABB aabb = {
        Subtract(center, halfExtents),
        Add(center, halfExtents)
    };
    return OverlapAABB(aabb, layerMask);
}

std::vector<Collider*> Physics::OverlapAABB(const AABB& aabb, uint32_t layerMask) {
    std::vector<Collider*> result;
    if (!s_colliders_) return result;

    for (auto* col : *s_colliders_) {
        if (!col || !col->IsCollide()) continue;

        uint32_t colLayer = CollisionLayer::Default;
        if (col->GetCategory() == CollisionCategory::Player) colLayer = CollisionLayer::Player;
        else if (col->GetCategory() == CollisionCategory::Enemy) colLayer = CollisionLayer::Enemy;
        if ((colLayer & layerMask) == 0) continue;

        Sphere colSphere = { col->GetWorldPosition(), col->GetRadius() };
        if (Intersect(aabb, colSphere)) {
            result.push_back(col);
        }
    }
    return result;
}
