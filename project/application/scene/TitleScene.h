#pragma once
#include"MathFunction.h"
#include "Sprite.h"
#include"Object3D.h"
#include "Model.h"
#include "Camera.h"
#include "ParticleEmitter.h"
#include "Audio.h"
#include "TextureManager.h"

#include "Scene.h"
#include <memory>
#include "Audio.h"

#include "SkyBox.h"

#include "Animation.h"

class Phase;



class TitleScene :public Scene
{
public:
    TitleScene();
    ~TitleScene();

    void Initialize()override;
    void Finalize()override;
    void Update()override;
    void Draw()override;


private:
    std::unique_ptr<Camera> camera;

    std::unique_ptr<Sprite> sprite;
    std::unique_ptr<Object3d> object3d;
    std::unique_ptr<Animation> animation;

    // 道（地面）オブジェクト
    std::unique_ptr<Object3d> roadObject_;

    // オービットカメラ（旋回演出）用パラメータ
    float cameraOrbitAngle_ = 0.0f;       // 現在の旋回角度（ラジアン）
    float cameraOrbitSpeed_ = 0.25f;      // 旋回速度（ラジアン/秒）
    float cameraOrbitRadius_ = 6.0f;      // キャラクターからの旋回距離（半径）
    float cameraHeight_ = 1.8f;           // カメラの高さ
    float cameraTargetHeight_ = 1.0f;     // 注視点の高さオフセット
    Vector3 characterPos_ = { 0.0f, 0.432f, 0.0f }; // キャラクター座標
    Vector3 roadScale_ = { 3.0f, 0.2f, 20.0f };   // 道のスケール

    // 2D PBDクロス（画面全体・四隅固定タイトル幕演出）用パラメータ
    std::unique_ptr<class PbdCloth2D> cloth2D_;
    Vector2 cloth2DPos_ = { 640.0f, 200.0f };    // 画面中央
    Vector2 cloth2DSize_ = { 1280.0f, 720.0f };  // 画面全体サイズ
    Vector2 cloth2DAnchor_ = { 0.5f, 0.5f };     // 中央基準
    float cloth2DRotation_ = 0.0f;
    bool cloth2DWindEnabled_ = true;
    float cloth2DWindStrength_ = 180.0f;         // 2D風の強さ
    int cloth2DPinMode_ = 2;                     // 0: 左右2点, 1: 上辺全体, 2: 四隅固定
    float cloth2DK_ = 0.15f;                     // バネ剛性
    Vector2 cloth2DGravity_ = { 0.0f, 150.0f };  // 四隅固定用の適度な重力

    bool isDebugCamera_ = false;
    std::unique_ptr<SkyBox> skyBox;

    Audio::SoundHandle  handle_ = 0;
};

