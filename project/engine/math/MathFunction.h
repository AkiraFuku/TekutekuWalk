#pragma once
///
#define _USE_MATH_DEFINES
#define PI 3.14159265358979323846f
#include "Vector4.h"
#include "Quaternion.h"
#include "Vector3.h"
#include "Vector2.h"

//#include "Transform.h"
struct EulerTransform;
struct QuaternionTransform;
struct UVTransform;

float ToRadians(float d);

Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);
Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);
static const int kColumnWidth = 60;
static const int kRowHeight = 20;
Vector3 Cross(const Vector3& v1, const Vector3& v2);
Vector3 Division(const Vector3& v1, const Vector3& v2);

Vector3 operator+(const Vector3& v1, const Vector3& v2);
Vector3 operator+=(Vector3& v1, const Vector3& v2);
Vector3 operator-(const Vector3& v1, const Vector3& v2);
Vector3 operator-=(Vector3& v1, const Vector3& v2);
Vector3 operator-(const Vector3& v);

Vector3 operator*(float scalar, const Vector3& v);
Vector3 operator*(const Vector3& v, float scalar);
Vector3 operator/(const Vector3& v, float scalar);
Vector3 operator/=(Vector3& v, float scalar);

Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4&m2);

Matrix4x4 MakeBillboardMatrix(const Vector3& scale, const Vector3& rotate, Matrix4x4& billboardMatrix, const Vector3& translate);Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& traslate);

template <typename T>
T Lerp(const T& v1, const T& v2, float t);
///
///透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farCrip);
///正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farCrip);
///ビューポート行列
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

Matrix4x4 MakeUVTransformMatrix(const UVTransform& uvTransform);
Matrix4x4 MakeTranslateMatrix(const Vector2& translate);
Matrix4x4 MakeScaleMatrix(const Vector2& scale);
Matrix4x4 MakeUVRotateMatrix(const float& rotate);

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& traslate);
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& traslate);
Matrix4x4 MakeTranslateMatrix(const Vector3& traslate);
Matrix4x4 MakeScaleMatrix(const Vector3& scale);
/// <summary>
/// 
/// </summary>
/// <param name="vector"></param>
/// <param name="matrix"></param>
/// <returns></returns>
Vector3  vector3Transform(const Vector3& vector, const Matrix4x4& matrix);

Matrix4x4 MakeRotateXMatrix(float radian);
Matrix4x4 MakeRotateYMatrix(float radian);
Matrix4x4 MakeRotateZMatrix(float radian);



/// <summary>
/// matrix4x4
/// </summary>
///　行列のAdd
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
/// <summary>
/// 行列のSubtract
/// </summary>
/// <param name="m1"></param>
/// <param name="m2"></param>
/// <returns></returns>
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
/// <summary>
/// 行列のMultiply
/// </summary>
/// <param name="m1"></param>
/// <param name="m2"></param>
/// <returns></returns>
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
/// <summary>
/// 逆行列
/// </summary>
/// <param name="m"></param>
/// <returns></returns>
Matrix4x4 Inverse(const Matrix4x4& m);
/// <summary>
/// 転置行列
/// </summary>
/// <param name="m"></param>
/// <returns></returns>
Matrix4x4 Transpose(const Matrix4x4& m);
/// <summary>
/// 単位行列の作成
/// </summary>
/// <returns></returns>
Matrix4x4 Makeidentity4x4();
//}

    /// <summary>
    /// ベクトルの値の描画
    /// </summary>
    /// <param name="x">描画位置ｘ</param>
    /// <param name="y">描画位置ｙ</param>
    /// <param name="vector">ベクトル</param>
    /// <param name="label">関数名</param>

    /// <summary>
    /// ADD 
    /// </summary>
    /// <param name="v1">ベクトル１</param>
    /// <param name="v2">ベクトル２</param>
    /// <returns>ベクトル１＋２</returns>
Vector3 Add(const Vector3& v1, const Vector3& v2);
/// <summary>
/// Subtract
/// </summary>
/// <param name="v1">ベクトル１</param>
/// <param name="v2">ベクトル２</param>
/// <returns>ベクトル１－２</returns>
Vector3 Subtract(const Vector3& v1, const Vector3& v2);
/// <summary>
/// スカラー倍
/// </summary>
/// <param name="scalar">スカラー数</param>
/// <param name="v">ベクトル</param>
/// <returns>ベクトル＊スカラー</returns>
Vector3 Multiply(float scalar, const Vector3& v);
/// <summary>
/// DOT
/// </summary>
/// <param name="v1">ベクトル１</param>
/// <param name="v2">ベクトル２</param>
/// <returns></returns>
float Dot(const Vector3& v1, const Vector3& v2);
/// <summary>
/// Length
/// </summary>
/// <param name="v">ベクトル</param>
/// <returns></returns>
float Length(const Vector3& v);
/// <summary>
/// Normalize
/// </summary>
/// <param name="v">ベクトル</param>
/// <returns>正規化数</returns>
Vector3 Normalize(const Vector3& v);

Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

template<typename T>

/// <summary>
/// 線形補間
/// </summary>
/// <param name="v1">ベクトル１ </param>
/// <param name="v2">ベクトル２</param>
/// <param name="t">補間値</param>
/// <returns>補間後のベクトル</returns>
inline T Lerp(const T& v1, const T& v2, float t)
{
    return v1 + (v2 - v1) * t;
}
Vector2 Normalize(const Vector2& v);
Vector2 operator+(const Vector2& v1, const Vector2& v2);
Vector2 operator-(const Vector2& v1, const Vector2& v2);