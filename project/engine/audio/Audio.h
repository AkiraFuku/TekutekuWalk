#pragma once
#include <wrl.h>
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include "Singleton.h"


class Audio : public Singleton<Audio>
{
public:
     friend class Singleton<Audio>;
     friend struct std::default_delete<Audio>;
    // 型を明確に分離
    using SoundHandle = uint32_t;
    
    // 世代番号つき再生ハンドル (上32bit: 世代, 下32bit: インデックス)
    using VoiceHandle = uint64_t;

    struct SoundData {
        WAVEFORMATEX wfex;
        std::vector<BYTE> buffer;
    };

    enum class VoiceState { Playing, Paused, Stopped };

    struct Voice {
        VoiceHandle handle;
        SoundHandle sourceHandle;
        IXAudio2SourceVoice* sourceVoice = nullptr;
        std::string category = "SE";
        VoiceState state = VoiceState::Stopped;
        float baseVolume = 1.0f;
    };

    void Initialize();
    void Finalize();
    void Update();

    SoundHandle LoadAudio(const std::string& filename);
    void UnloadAudio(SoundHandle soundHandle);

    // PlayAudioは再生用の VoiceHandle を返す
    VoiceHandle PlayAudio(SoundHandle soundHandle, bool loop = false, float volume = 1.0f, const std::string& category = "SE");

    // VoiceHandle を指定して操作
    void StopAudio(VoiceHandle voiceHandle);
    void PauseAudio(VoiceHandle voiceHandle);
    void ResumeAudio(VoiceHandle voiceHandle);
    bool IsPlaying(VoiceHandle voiceHandle);

    // カテゴリ一括操作
    void SetCategoryVolume(const std::string& category, float volume);
    void StopCategory(const std::string& category);

protected:
    Audio() = default;
    ~Audio() override = default;

private:
    VoiceHandle MakeVoiceHandle(uint32_t index, uint32_t generation);

    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    IXAudio2MasteringVoice* masteringVoice_ = nullptr;

    std::unordered_map<SoundHandle, SoundData> soundDatas_;
    SoundHandle nextSoundHandle_ = 1;

    std::vector<Voice> activeVoices_;
    uint32_t nextVoiceIndex_ = 0;
    uint32_t currentGeneration_ = 1;

    std::unordered_map<std::string, float> categoryVolumes_;

    static std::unique_ptr<Audio> instance;
};