#pragma once

#include "Vector4.h"
#include "Vector3.h"
#include <cstdint>

// 共通レンダリング用定数バッファ構造体
struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Matrix4x4 WorldInverseTranspose;
    Matrix4x4 View;
    Matrix4x4 Projection;
    float curvature;       // 空間湾曲率 (0.0fで湾曲なし, 0.001f等で奥に向かって下がる)
    float padding[3];      // 16バイトアライメント用パディング
};

struct CameraForGPU {
    Vector3 worldPosition;
    float farClip;
    Vector3 cameraForward;
    float padding; // 16バイト整列
};

struct DirectionalLight {
    Vector4 color;    // ライトの色
    Vector3 direction; // ライトの向き
    float intensity;   // 明るさ
};

struct PointLight {
    Vector4 color;     // ライトの色
    Vector3 position;  // ライトの位置
    float intensity;   // 明るさ
    float radius;      // 影響半径
    float decay;       // 減衰率
    float padding[2];  // 16バイト整列
};

struct SpotLight {
    Vector4 color;          // ライトの色
    Vector3 position;       // ライトの位置
    float intensity;        // 明るさ
    Vector3 direction;      // ライトの向き
    float distance;         // 照射距離
    float decay;            // 減衰率
    float cosAngle;         // スポット角度
    float cosFalloffStart;  // フォールオフ開始角度
    float padding;          // 16バイト整列
};

// DrawIndirect 用間接描画引数 / ライトカウント構造体
struct IndirectArguments {
    uint32_t numDirectionalLights;
    uint32_t numPointLights;
    uint32_t numSpotLights;
    uint32_t padding;
};

// 汎用マテリアル定数バッファ（Sprite 等で使われているレイアウトに合わせる）
struct MaterialCB {
    Vector4 color;          // RGBA
    int32_t enableLighting; // 0 or 1
    float padding[3];       // 16バイト境界を保つためのパディング
    Matrix4x4 uvTransform;  // UV変換行列（Sprite 用）
};
