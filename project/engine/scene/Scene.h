#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "Audio.h"
#include "DrawFunction.h"

class SceneManager;
class Camera;
class Phase;
class Player;

class Scene
{
public:
    virtual ~Scene() = default;
    virtual void Initialize() = 0;
    virtual void Finalize() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;

    virtual void SetSceneManager(SceneManager* sceneManager) {
        sceneManager_ = sceneManager;
    }
    SceneManager* GetSceneManager() {
        return sceneManager_;
    }

    void ChangeActiveCamera(Camera* targetCamera);

    Camera* GetActiveCamera() const {
        return activeCamera_;
    }

   void ChangePhase(std::unique_ptr<Phase> nextPhase);

   Audio::VoiceHandle getBGMPlayHundle(){
       return BGMHandle_;
   };

   void PrayBGM(Audio::SoundHandle bgmHundle);

   // --- ゲームプレイ共通の仮想関数（ポリモーフィズム） ---
   virtual void TriggerHitStop(float duration = 0.1f) {}
   virtual void RequestCameraShake(float duration = 0.1f, float power = 1.0f) {}
   virtual const std::vector<Triangle>& GetTriangle() const {
       static const std::vector<Triangle> empty;
       return empty;
   }
   virtual Player* GetPlayer() { return nullptr; }

private:
    SceneManager* sceneManager_ = nullptr;

protected:
    Audio::VoiceHandle BGMHandle_ = 0;
    Camera* activeCamera_ = nullptr;
    std::map<std::string, std::unique_ptr<Camera>> cameraMap_;

    std::unique_ptr<Phase> currentPhase_;

};

