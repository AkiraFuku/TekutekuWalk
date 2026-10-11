#pragma once

#pragma once
#include "MathFunction.h"
#include "Vector4.h"
#include "Quaternion.h"
struct Sphere{
	Vector3 center;//中心
	float radius;
    Quaternion rotate={ 0.0f,0.0f,0.0f,1.0f }; // 回転を追加
};
struct Line{
	Vector3 origin;//起点
	Vector3 diff;//方向

};
struct Ray{
	Vector3 origin;//起点
	Vector3 diff;//方向
};
struct  Segment{
	Vector3 origin;//起点
	Vector3 diff;//方向
};
struct Plane
{
	Vector3 normal;//法向量
	float distance;//距離
};
struct Triangle
{
	Vector3 vertices[3];//頂点1
	bool isOneway = false; // すり抜け足場フラグ
};
#include <algorithm>
struct AABB{
	Vector3 min;
	Vector3 max;
};

// 三角形の3頂点から外接AABBを取得する関数
inline AABB GetTriangleAABB(const Triangle& tri) {
	AABB box;
	box.min.x = (std::min)({ tri.vertices[0].x, tri.vertices[1].x, tri.vertices[2].x });
	box.min.y = (std::min)({ tri.vertices[0].y, tri.vertices[1].y, tri.vertices[2].y });
	box.min.z = (std::min)({ tri.vertices[0].z, tri.vertices[1].z, tri.vertices[2].z });

	box.max.x = (std::max)({ tri.vertices[0].x, tri.vertices[1].x, tri.vertices[2].x });
	box.max.y = (std::max)({ tri.vertices[0].y, tri.vertices[1].y, tri.vertices[2].y });
	box.max.z = (std::max)({ tri.vertices[0].z, tri.vertices[1].z, tri.vertices[2].z });
	return box;
}

// 2つのAABBが重なっているかの高速判定
inline bool IsOverlapAABB(const AABB& a, const AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
	       (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
	       (a.min.z <= b.max.z && a.max.z >= b.min.z);
}
// レイと三角形の交差判定結果を表す列挙型
enum class RayTriangleCollisionResult
{
    NoCollision,
    FrontFace,
    BackFace
};



Vector3 Project(const Vector3& v1,const Vector3& v2);
Vector3 ClosestPoint(const Vector3& point,const Segment& segment);
float DistanceSqPointToSegment(const Vector3& p, const Vector3& a, const Vector3& b);

bool IsCollision(const Sphere& si,const Sphere& s2);
bool IsCollision(const Sphere& sphere,const Plane& plane);
bool isCollision(const Segment& segment, const Plane& plane);
bool isCollision(const Line& line, const Plane& plane);
bool isCollision(const Ray& ray, const Plane& plane);
bool isCollision(const Ray& ray, const Triangle& triangle);
bool isCollision(const Triangle triangle , const  Segment& segment );
bool IsCollision(const AABB& a, const AABB& b);
bool IsCollision(const AABB& aabb,const Sphere& sphere);
bool IsCollision(const AABB& aabb,const Segment& segment);

// レイと三角形の交差判定 (交点と距離を取得)
bool CheckRayTriangle(const Ray& ray, const Triangle& triangle, float* outDistance, Vector3* outHitPoint, RayTriangleCollisionResult* outResult=nullptr);


