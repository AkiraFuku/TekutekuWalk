#include "MathFunction.h"
#include "RotateFunction.h"
#include "Vector3.h"
#include <cmath>
#include <assert.h>
#include "Transform.h"
#include <numbers>



//
// 
// 
// 
// 
Matrix4x4 MakeBillboardMatrix(const Vector3& scale, const Vector3& rotate, Matrix4x4& billboardMatrix, const Vector3& translate)
{
    // 1. 各成分の行列を作成
    Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
    Matrix4x4 rotateMatrix = MakeRotateZMatrix(rotate.z); // Z軸回転のみ使用
    Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

    // 2. 行列を順番に合成 (Scale * RotateZ * Billboard * Translate)

    // Scale * RotateZ
    Matrix4x4 matScaleRot = Multiply(scaleMatrix, rotateMatrix);

    // (Scale * RotateZ) * Billboard
    Matrix4x4 matSRB = Multiply(matScaleRot, billboardMatrix);

    // 全体 * Translate
    Matrix4x4 result_ = Multiply(matSRB, translateMatrix);

    return result_;
}
float ToRadians(float d)
{
    return d * (std::numbers::pi_v<float>/ 180.0f);;
}
Vector3 Bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    Vector3 result_ = uuu * p0;      // (1-t)^3 * P0
    result_ += 3.0f * uu * t * p1;   // 3 * (1-t)^2 * t * P1
    result_ += 3.0f * u * tt * p2;   // 3 * (1-t) * t^2 * P2
    result_ += ttt * p3;             // t^3 * P3

    return result_;
}
Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    Vector3 result_ = 0.5f * (
        (2.0f * p1) +
        (-p0 + p2) * t +
        (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
        (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
        );
    return result_;
}
Vector3 Cross(const Vector3& v1, const Vector3& v2)
{

    return {
    v1.y * v2.z - v1.z * v2.y,
    v1.z * v2.x - v1.x * v2.z,
    v1.x * v2.y - v1.y * v2.x
    };
}
Vector3 Division(const Vector3& v1, const Vector3& v2)
{
    return Vector3(
        v1.x / v2.x,
        v1.y / v2.y,
        v1.z / v2.z

    );
}
Vector3 operator+(const Vector3& v1, const Vector3& v2)
{


    Vector3 result_;
    result_.x = v1.x + v2.x;
    result_.y = v1.y + v2.y;
    result_.z = v1.z + v2.z;
    return result_;
}

Vector3 operator+=(Vector3& v1, const Vector3& v2)
{

    v1.x += v2.x;
    v1.y += v2.y;
    v1.z += v2.z;
    return v1;
}

Vector3 operator-(const Vector3& v1, const Vector3& v2)
{

    Vector3 result_;
    result_.x = v1.x - v2.x;
    result_.y = v1.y - v2.y;
    result_.z = v1.z - v2.z;
    return result_;
}

Vector3 operator-=(Vector3& v1, const Vector3& v2)
{

    v1.x -= v2.x;
    v1.y -= v2.y;
    v1.z -= v2.z;
    return v1;
}

Vector3 operator-(const Vector3& v)
{
        return Vector3(-v.x, -v.y, -v.z);

}

Vector3 operator*(float scalar, const Vector3& v)
{

    Vector3 result_;
    result_.x = v.x * scalar;
    result_.y = v.y * scalar;
    result_.z = v.z * scalar;
    return result_;
}

Vector3 operator*(const Vector3& v, float scalar)
{
    return scalar * v;

}
Vector3 operator/(const Vector3& v, float scalar)
{

    Vector3 result_;
    result_.x = v.x / scalar;
    result_.y = v.y / scalar;
    result_.z = v.z / scalar;
    return result_;
}

Vector3 operator/=(Vector3& v, float scalar)
{
    v.x /= scalar;
    v.y /= scalar;
    v.z /= scalar;
    return v;
}

Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2)
{
    return Multiply(m1,m2);
}


	Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip){
		float cot = 1.0f / std::tan(fovY / 2.0f);
		return Matrix4x4(
			{
				{{cot / aspectRatio,0.0f,0.0f,0.0f},
				{0.0f,cot,0.0f,0.0f},
				{0.0f,0.0f,farClip  / (farClip- nearClip),1.0f},
				{0.0f,0.0f,( -nearClip*farClip) / (farClip- nearClip), 0.0f}}
			
			}
		
		);
	}

	Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip){
		return Matrix4x4(
			{
				{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
				{0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
				{0.0f, 0.0f, 1.0f / (farClip - nearClip), 0.0f},
				{(right + left) / (left-right), (top + bottom) / ( bottom-top),  nearClip / ( nearClip-farClip), 1.0f}
			
			}
		
		);
	}

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth) {
    return Matrix4x4(
        {
            {
                {width / 2.0f, 0.0f, 0.0f, 0.0f},
                {0.0f, -height / 2.0f, 0.0f, 0.0f},
                {0.0f, 0.0f, maxDepth - minDepth, 0.0f},
                {left + width / 2.0f, top + height / 2.0f, minDepth, 1.0f}
            }

        }

    );
}

Matrix4x4 MakeUVTransformMatrix(const UVTransform& uvTransform)
{
    Matrix4x4 scaleMatrix = MakeScaleMatrix(uvTransform.scale);
    Matrix4x4 rotateMatrix = MakeUVRotateMatrix(uvTransform.rotate);
    Matrix4x4 translateMatrix = MakeTranslateMatrix(uvTransform.offset);
    Matrix4x4 result_ = Multiply( translateMatrix,Multiply(scaleMatrix, rotateMatrix));
    return result_;
}

Matrix4x4 MakeTranslateMatrix(const Vector2& translate)
{
    Matrix4x4 result_ = Makeidentity4x4();
    result_.m[3][0] = translate.x;
    result_.m[3][1] = translate.y;
    return result_;
}

Matrix4x4 MakeScaleMatrix(const Vector2& scale)
{
    Matrix4x4 result_ =Makeidentity4x4();
    result_.m[0][0] = scale.x;
    result_.m[1][1] = scale.y;
    return result_ ;
}

Matrix4x4 MakeUVRotateMatrix(const float& rotate)
{
    Matrix4x4 result_ =Makeidentity4x4();
    result_.m[0][0] = std::cos(rotate);
    result_.m[0][1] = -std::sin(rotate);
    result_.m[1][0] = std::sin(rotate);
    result_.m[1][1] = std::cos(rotate);
    return result_;
}


	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate)
	{
		Matrix4x4 scaleMatrix=MakeScaleMatrix(scale);
		Matrix4x4 rotateMatrix=Multiply(MakeRotateXMatrix( rotate.x),Multiply(MakeRotateYMatrix( rotate.y),MakeRotateZMatrix( rotate.z)));
		Matrix4x4 translateMatrix=MakeTranslateMatrix(translate);

	    Matrix4x4 result_=Multiply(Multiply(scaleMatrix,rotateMatrix),translateMatrix);
		return result_ ;
	}

    Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate)
    {
       Matrix4x4 scaleMatrix=MakeScaleMatrix(scale);
       Matrix4x4 rotateMatrix = MakeRotateMatrix(rotate);
		Matrix4x4 translateMatrix=MakeTranslateMatrix(translate);

	    Matrix4x4 result_=Multiply(Multiply(scaleMatrix,rotateMatrix),translateMatrix);
		return result_ ;
    }




/// <summary>
/// 
/// </summary>
/// <param name="translate"></param>
/// <returns></returns>
Matrix4x4 MakeTranslateMatrix(const Vector3& translate){
	return Matrix4x4(
		{
			{
				{1.0f,0.0f,0.0f,0.0f},
				{0.0f,1.0f,0.0f,0.0f},
				{0.0f,0.0f,1.0f,0.0f},
				{translate.x,translate.y,translate.z,1.0f}
			}
		}
	);
}
/// <summary>
/// 
/// </summary>
/// <param name="scale"></param>
/// <returns></returns>
Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
    return Matrix4x4(
        {
            {
                {scale.x,0.0f,0.0f,0.0f},
                {0.0f,scale.y,0.0f,0.0f},
                {0.0f,0.0f,scale.z,0.0f},
                {0.0f,0.0f,0.0f,1.0f}
            }
        }
    );
}
/// <summary>
/// 
/// </summary>
/// <param name="vector"></param>
/// <param name="matrix"></param>
/// <returns></returns>
Vector3 vector3Transform(const Vector3& vector, const Matrix4x4& matrix) {
    Vector3 result_;
    result_.x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + 1.0f * matrix.m[3][0];
    result_.y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + 1.0f * matrix.m[3][1];
    result_.z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + 1.0f * matrix.m[3][2];
    float  w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + 1.0f * matrix.m[3][3];
    assert(w != 0.0f);
    result_.x /= w;
    result_.y /= w;
    result_.z /= w;
    return result_;
}
/// <summary>
/// 
/// </summary>
/// <param name="radian"></param>
/// <returns></returns>
Matrix4x4 MakeRotateXMatrix(float radian)
{
    return Matrix4x4(
        {
            {1.0f,0.0f,0.0f,0.0f,},
            {0.0f,std::cos(radian),std::sin(radian),0.0f},
            {0.0f,-std::sin(radian),std::cos(radian),0.0f},
            {0.0f,0.0f,0.0f,1.0f}
        }
    );
}
Matrix4x4 MakeRotateYMatrix(float radian)
{
    return Matrix4x4(
        {
        {std::cos(radian),0.0f,-std::sin(radian),0.0f,},
        {0.0f,1.0f,0.0f,0.0f},
        {std::sin(radian),0.0f,std::cos(radian),0.0f},
        {0.0f,0.0f,0.0f,1.0f}
        }
    );
}
Matrix4x4 MakeRotateZMatrix(float radian)
{
    return Matrix4x4(
        {

            {std::cos(radian),std::sin(radian),0.0f,0.0f},
            {-std::sin(radian),std::cos(radian),0.0f,0.0f},
            {0.0f,0.0f,1.0f,0.0f,},
            {0.0f,0.0f,0.0f,1.0f}
        }
    );
}
//
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result_ = {};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result_.m[i][j] = m1.m[i][j] + m2.m[i][j];
        }
    }
    return result_;
}
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result_ = {};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result_.m[i][j] = m1.m[i][j] - m2.m[i][j];
        }
    }
    return result_;
}
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
    Matrix4x4 result_ = {};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result_.m[i][j] += m1.m[i][k] * m2.m[k][j];
            }
        }
    }

    return result_;
}
Matrix4x4 Inverse(const Matrix4x4& m) {
    float det =
        (m.m[0][0] * m.m[1][1] * m.m[2][2] * m.m[3][3]) + (m.m[0][0] * m.m[1][2] * m.m[2][3] * m.m[3][1]) + (m.m[0][0] * m.m[1][3] * m.m[2][1] * m.m[3][2])
        - (m.m[0][0] * m.m[1][3] * m.m[2][2] * m.m[3][1]) - (m.m[0][0] * m.m[1][2] * m.m[2][1] * m.m[3][3]) - (m.m[0][0] * m.m[1][1] * m.m[2][3] * m.m[3][2])
        - (m.m[0][1] * m.m[1][0] * m.m[2][2] * m.m[3][3]) - (m.m[0][2] * m.m[1][0] * m.m[2][3] * m.m[3][1]) - (m.m[0][3] * m.m[1][0] * m.m[2][1] * m.m[3][2])
        + (m.m[0][3] * m.m[1][0] * m.m[2][2] * m.m[3][1]) + (m.m[0][2] * m.m[1][0] * m.m[2][1] * m.m[3][3]) + (m.m[0][1] * m.m[1][0] * m.m[2][3] * m.m[3][2])
        + (m.m[0][1] * m.m[1][2] * m.m[2][0] * m.m[3][3]) + (m.m[0][2] * m.m[1][3] * m.m[2][0] * m.m[3][1]) + (m.m[0][3] * m.m[1][1] * m.m[2][0] * m.m[3][2])
        - (m.m[0][3] * m.m[1][2] * m.m[2][0] * m.m[3][1]) - (m.m[0][2] * m.m[1][1] * m.m[2][0] * m.m[3][3]) - (m.m[0][1] * m.m[1][3] * m.m[2][0] * m.m[3][2])
        - (m.m[0][1] * m.m[1][2] * m.m[2][3] * m.m[3][0]) - (m.m[0][2] * m.m[1][3] * m.m[2][1] * m.m[3][0]) - (m.m[0][3] * m.m[1][1] * m.m[2][2] * m.m[3][0])
        + (m.m[0][3] * m.m[1][2] * m.m[2][1] * m.m[3][0]) + (m.m[0][2] * m.m[1][1] * m.m[2][3] * m.m[3][0]) + (m.m[0][1] * m.m[1][3] * m.m[2][2] * m.m[3][0])
        ;
    Matrix4x4 result_ =
    {
        {
            //0
                {///0
                    (1 / det) * (
                         (m.m[1][1] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]))
                        + (m.m[1][2] * (m.m[2][3] * m.m[3][1] - m.m[2][1] * m.m[3][3]))
                        + (m.m[1][3] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]))
                        ),
                        ////

                         ///1
                            -(1 / det) * (
                                 (m.m[2][1] * (m.m[3][2] * m.m[0][3] - m.m[3][3] * m.m[0][2]))
                                + (m.m[2][2] * (m.m[3][3] * m.m[0][1] - m.m[3][1] * m.m[0][3]))
                                + (m.m[2][3] * (m.m[3][1] * m.m[0][2] - m.m[3][2] * m.m[0][1]))
                            ),
                        ////

                        ///2,
                            (1 / det) * (
                                 (m.m[3][1] * (m.m[0][2] * m.m[1][3] - m.m[0][3] * m.m[1][2]))
                                + (m.m[3][2] * (m.m[0][3] * m.m[1][1] - m.m[0][1] * m.m[1][3]))
                                + (m.m[3][3] * (m.m[0][1] * m.m[1][2] - m.m[0][2] * m.m[1][1]))
                                ),
                        ////

                        ///3
                            -(1 / det) * (
                                 (m.m[0][1] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2]))
                                + (m.m[0][2] * (m.m[1][3] * m.m[2][1] - m.m[1][1] * m.m[2][3]))
                                + (m.m[0][3] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]))
                                ),
                        ////
                        },
        ////

        //1
            {
                ///0
                -(1 / det) * (
                         (m.m[1][2] * (m.m[2][3] * m.m[3][0] - m.m[2][0] * m.m[3][3]))
                        + (m.m[1][3] * (m.m[2][0] * m.m[3][2] - m.m[2][2] * m.m[3][0]))
                        + (m.m[1][0] * (m.m[2][2] * m.m[3][3] - m.m[2][3] * m.m[3][2]))
                        ),
                    ////

                    ///1
                    (1 / det) * (
                             (m.m[2][2] * (m.m[3][3] * m.m[0][0] - m.m[3][0] * m.m[0][3]))
                            + (m.m[2][3] * (m.m[3][0] * m.m[0][2] - m.m[3][2] * m.m[0][0]))
                            + (m.m[2][0] * (m.m[3][2] * m.m[0][3] - m.m[3][3] * m.m[0][2]))
                            ),
                    ////

                    ///2
                    -(1 / det) * (
                             (m.m[3][2] * (m.m[0][3] * m.m[1][0] - m.m[0][0] * m.m[1][3]))
                            + (m.m[3][3] * (m.m[0][0] * m.m[1][2] - m.m[0][2] * m.m[1][0]))
                            + (m.m[3][0] * (m.m[0][2] * m.m[1][3] - m.m[0][3] * m.m[1][2]))
                            ),
                    ///3
                    (1 / det) * (
                             (m.m[0][2] * (m.m[1][3] * m.m[2][0] - m.m[1][0] * m.m[2][3]))
                            + (m.m[0][3] * (m.m[1][0] * m.m[2][2] - m.m[1][2] * m.m[2][0]))
                            + (m.m[0][0] * (m.m[1][2] * m.m[2][3] - m.m[1][3] * m.m[2][2]))
                            ),
                    ////

                    },
        ////

        //2
            {
                ///0
                (1 / det) * (
                         (m.m[1][3] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]))
                        + (m.m[1][0] * (m.m[2][1] * m.m[3][3] - m.m[2][3] * m.m[3][1]))
                        + (m.m[1][1] * (m.m[2][3] * m.m[3][0] - m.m[2][0] * m.m[3][3]))
                        ),
                    ////

                    ///1
                    -(1 / det) * (
                             (m.m[2][3] * (m.m[3][0] * m.m[0][1] - m.m[3][1] * m.m[0][0]))
                            + (m.m[2][0] * (m.m[3][1] * m.m[0][3] - m.m[3][3] * m.m[0][1]))
                            + (m.m[2][1] * (m.m[3][3] * m.m[0][0] - m.m[3][0] * m.m[0][3]))
                            ),
                    ////

                    ///2
                    (1 / det) * (
                             (m.m[3][3] * (m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0]))
                            + (m.m[3][0] * (m.m[0][1] * m.m[1][3] - m.m[0][3] * m.m[1][1]))
                            + (m.m[3][1] * (m.m[0][3] * m.m[1][0] - m.m[0][0] * m.m[1][3]))
                            ),
                    ////

                    ///3
                    -(1 / det) * (
                             (m.m[0][3] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]))
                            + (m.m[0][0] * (m.m[1][1] * m.m[2][3] - m.m[1][3] * m.m[2][1]))
                            + (m.m[0][1] * (m.m[1][3] * m.m[2][0] - m.m[1][0] * m.m[2][3]))
                            ),
                    },
        ////

        //3
            {
                ///0
                -(1 / det) * (
                         (m.m[1][0] * (m.m[2][1] * m.m[3][2] - m.m[2][2] * m.m[3][1]))
                        + (m.m[1][1] * (m.m[2][2] * m.m[3][0] - m.m[2][0] * m.m[3][2]))
                        + (m.m[1][2] * (m.m[2][0] * m.m[3][1] - m.m[2][1] * m.m[3][0]))
                        ),
                    ///1
                    (1 / det) * (
                             (m.m[2][0] * (m.m[3][1] * m.m[0][2] - m.m[3][2] * m.m[0][1]))
                            + (m.m[2][1] * (m.m[3][2] * m.m[0][0] - m.m[3][0] * m.m[0][2]))
                            + (m.m[2][2] * (m.m[3][0] * m.m[0][1] - m.m[3][1] * m.m[0][0]))
                            ),
                    ///2
                    -(1 / det) * (
                             (m.m[3][0] * (m.m[0][1] * m.m[1][2] - m.m[0][2] * m.m[1][1]))
                            + (m.m[3][1] * (m.m[0][2] * m.m[1][0] - m.m[0][0] * m.m[1][2]))
                            + (m.m[3][2] * (m.m[0][0] * m.m[1][1] - m.m[0][1] * m.m[1][0]))
                            ),
                    ///3
                    (1 / det) * (
                             (m.m[0][0] * (m.m[1][1] * m.m[2][2] - m.m[1][2] * m.m[2][1]))
                            + (m.m[0][1] * (m.m[1][2] * m.m[2][0] - m.m[1][0] * m.m[2][2]))
                            + (m.m[0][2] * (m.m[1][0] * m.m[2][1] - m.m[1][1] * m.m[2][0]))
                            ),
                    }
                }
    };




		return result_;
	}
	Matrix4x4 Transpose(const Matrix4x4& m)
	{
		
		return Matrix4x4(
		{
			
				{m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0]},
				{m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1]},
				{m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2]},
				{m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3]}
		}
		);
	}
	Matrix4x4 Makeidentity4x4(){
		return Matrix4x4(
			{
				{1,0,0,0},
				{0,1,0,0},
				{0,0,1,0},
				{0,0,0,1}
			}
		
		);
	}

//加算
Vector3 Add(const Vector3& v1, const Vector3& v2) {
    Vector3 result_ = {
        v1.x + v2.x,
        v1.y + v2.y,
        v1.z + v2.z

    };
    return result_;
}
//
Vector3 Subtract(const Vector3& v1, const Vector3& v2) {
    Vector3 result_ = {
        v1.x - v2.x,
        v1.y - v2.y,
        v1.z - v2.z
    };
    return result_;
}
//
Vector3 Multiply(float scalar, const Vector3& v) {
    Vector3 result_ = {
        scalar * v.x,
        scalar * v.y,
        scalar * v.z
    };
    return result_;
}
//
float Dot(const Vector3& v1, const Vector3& v2) {

    float result_ = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    return result_;
}
//
float Length(const Vector3& v) {
    float length = v.x * v.x + v.y * v.y + v.z * v.z;
    if (length > 0.0f) {
        return sqrtf(length);
    }
    return length;
}
//
Vector3 Normalize(const Vector3& v) {
    float length = Length(v);
    if (length > 0.0f) {
        Vector3 result_ = {
            v.x / length,
            v.y / length,
            v.z / length
        };
        return result_;
    }
    Vector3 result_ = { 0.0f, 0.0f, 0.0f };
    return result_;
    //
}

    Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m)
    {
        Vector3 ret={
           ret.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0],
           ret.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1],
           ret.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2]
        };

    return ret;
}

Vector2 Normalize(const Vector2& v)
{

    float length = sqrtf(v.x * v.x + v.y * v.y);
    if (length > 0.0f) {
        Vector2 result_ = {
            v.x / length,
            v.y / length
        };
        return result_;
    }
    Vector2 result_ = { 0.0f, 0.0f };
    return result_;
}

Vector2 operator+(const Vector2& v1, const Vector2& v2)
{
        return Vector2{ v1.x + v2.x, v1.y + v2.y };
}

Vector2 operator-(const Vector2& v1, const Vector2& v2)
{
            return Vector2{ v1.x - v2.x, v1.y - v2.y };
}
