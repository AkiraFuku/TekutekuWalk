#include "Audio.h"
#include <algorithm>
#include <cassert>
#include <windows.h>
#include "StringUtility.h" // 既存の変換クラス

// Media Foundation 関連
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib") 
#pragma comment(lib, "mfreadwrite.lib")

using namespace Microsoft::WRL;

void Audio::Initialize()
{
    HRESULT result;
    // MF初期化
    result = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    assert(SUCCEEDED(result));

    // XAudio2初期化
    result = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(result));

    // マスターボイス作成
    result = xAudio2_->CreateMasteringVoice(&masteringVoice_);
    assert(SUCCEEDED(result));
}

void Audio::Finalize()
{
    // 再生中のボイスをすべて破棄
    for (auto& voice : activeVoices_)
    {
        if (voice.sourceVoice)
        {
            voice.sourceVoice->Stop();
            voice.sourceVoice->FlushSourceBuffers();
            voice.sourceVoice->DestroyVoice();
            voice.sourceVoice = nullptr;
        }
    }
    activeVoices_.clear();

    // マスターボイス破棄
    if (masteringVoice_) {
        masteringVoice_->DestroyVoice();
        masteringVoice_ = nullptr;
    }

    xAudio2_.Reset();
    soundDatas_.clear();
    categoryVolumes_.clear();
    MFShutdown();
    Singleton<Audio>::Finalize();
}

void Audio::Update()
{
    for (auto it = activeVoices_.begin(); it != activeVoices_.end(); )
    {
        if (it->sourceVoice == nullptr) {
            it = activeVoices_.erase(it);
            continue;
        }

        XAUDIO2_VOICE_STATE state{};
        it->sourceVoice->GetState(&state);

        // 再生が完了し、キューが空になったボイスを自動破棄
        if (state.BuffersQueued == 0 && it->state == VoiceState::Playing)
        {
            it->sourceVoice->Stop();
            it->sourceVoice->FlushSourceBuffers();
            it->sourceVoice->DestroyVoice();
            it = activeVoices_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

Audio::VoiceHandle Audio::MakeVoiceHandle(uint32_t index, uint32_t generation) {
    return (static_cast<uint64_t>(generation) << 32) | index;
}

Audio::SoundHandle Audio::LoadAudio(const std::string& filename)
{
    std::wstring filePathW = StringUtility::ConvertString(filename);
    HRESULT result;

    // 1. ソースリーダー作成
    ComPtr<IMFSourceReader> pReader;
    result = MFCreateSourceReaderFromURL(filePathW.c_str(), nullptr, &pReader);
    if (FAILED(result)) {
        assert(false && "Failed to load audio file.");
        return 0;
    }

    // 2. メディアタイプ設定 (PCM)
    ComPtr<IMFMediaType> pPCMType;
    MFCreateMediaType(&pPCMType);
    pPCMType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    pPCMType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    result = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPCMType.Get());
    assert(SUCCEEDED(result));

    // 3. フォーマット取得
    ComPtr<IMFMediaType> pOutType;
    pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutType);
    WAVEFORMATEX* waveFormat = nullptr;
    MFCreateWaveFormatExFromMFMediaType(pOutType.Get(), &waveFormat, nullptr);

    SoundData soundData = {};
    soundData.wfex = *waveFormat;
    CoTaskMemFree(waveFormat);

    // 4. データ読み込みループ
    while (true)
    {
        ComPtr<IMFSample> pSample;
        DWORD flags = 0;
        result = pReader->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
            0, nullptr, &flags, nullptr, &pSample);

        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;

        if (pSample)
        {
            ComPtr<IMFMediaBuffer> pBuffer;
            pSample->ConvertToContiguousBuffer(&pBuffer);
            BYTE* pData = nullptr;
            DWORD currentLength = 0;
            pBuffer->Lock(&pData, nullptr, &currentLength);

            soundData.buffer.insert(soundData.buffer.end(), pData, pData + currentLength);
            pBuffer->Unlock();
        }
    }

    // 5. ハンドル発行と登録
    SoundHandle handle = nextSoundHandle_++;
    soundDatas_[handle] = soundData;

    return handle;
}

void Audio::UnloadAudio(SoundHandle soundHandle)
{
    soundDatas_.erase(soundHandle);
}

Audio::VoiceHandle Audio::PlayAudio(SoundHandle soundHandle, bool loop, float volume, const std::string& category)
{
    auto it = soundDatas_.find(soundHandle);
    if (it == soundDatas_.end()) return 0;

    const SoundData& soundData = it->second;

    IXAudio2SourceVoice* pSourceVoice = nullptr;
    HRESULT result = xAudio2_->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
    if (FAILED(result)) return 0;

    // カテゴリ音量の適用
    float catVol = categoryVolumes_.count(category) ? categoryVolumes_[category] : 1.0f;
    pSourceVoice->SetVolume(volume * catVol);

    XAUDIO2_BUFFER buf{};
    buf.AudioBytes = static_cast<UINT32>(soundData.buffer.size());
    buf.pAudioData = soundData.buffer.data();
    buf.Flags = XAUDIO2_END_OF_STREAM;
    if (loop) buf.LoopCount = XAUDIO2_LOOP_INFINITE;

    result = pSourceVoice->SubmitSourceBuffer(&buf);
    if (FAILED(result)) {
        pSourceVoice->DestroyVoice();
        return 0;
    }

    result = pSourceVoice->Start();
    if (FAILED(result)) {
        pSourceVoice->DestroyVoice();
        return 0;
    }

    // 世代つき VoiceHandle の発行
    VoiceHandle handle = MakeVoiceHandle(nextVoiceIndex_++, currentGeneration_++);

    Voice voice{};
    voice.handle = handle;
    voice.sourceHandle = soundHandle;
    voice.sourceVoice = pSourceVoice;
    voice.category = category;
    voice.state = VoiceState::Playing;
    voice.baseVolume = volume;

    activeVoices_.push_back(voice);

    return handle;
}

void Audio::StopAudio(VoiceHandle voiceHandle)
{
    auto it = std::find_if(
        activeVoices_.begin(), activeVoices_.end(),
        [voiceHandle](const Voice& v) { return v.handle == voiceHandle; }
    );

    if (it != activeVoices_.end())
    {
        if (it->sourceVoice) {
            it->sourceVoice->Stop();
            it->sourceVoice->FlushSourceBuffers();
            it->sourceVoice->DestroyVoice();
            it->sourceVoice = nullptr;
        }
        it->state = VoiceState::Stopped;
        activeVoices_.erase(it);
    }
}

void Audio::PauseAudio(VoiceHandle voiceHandle)
{
    auto it = std::find_if(
        activeVoices_.begin(), activeVoices_.end(),
        [voiceHandle](const Voice& v) { return v.handle == voiceHandle; }
    );

    if (it != activeVoices_.end() && it->state == VoiceState::Playing)
    {
        it->state = VoiceState::Paused;
        if (it->sourceVoice) {
            it->sourceVoice->Stop(0);
        }
    }
}

void Audio::ResumeAudio(VoiceHandle voiceHandle)
{
    auto it = std::find_if(
        activeVoices_.begin(), activeVoices_.end(),
        [voiceHandle](const Voice& v) { return v.handle == voiceHandle; }
    );

    if (it != activeVoices_.end() && it->state == VoiceState::Paused)
    {
        it->state = VoiceState::Playing;
        if (it->sourceVoice) {
            it->sourceVoice->Start(0, XAUDIO2_COMMIT_NOW);
        }
    }
}

bool Audio::IsPlaying(VoiceHandle voiceHandle)
{
    auto it = std::find_if(
        activeVoices_.begin(), activeVoices_.end(),
        [voiceHandle](const Voice& v) { return v.handle == voiceHandle; }
    );

    if (it == activeVoices_.end())
    {
        return false;
    }

    return it->state == VoiceState::Playing;
}

void Audio::SetCategoryVolume(const std::string& category, float volume)
{
    categoryVolumes_[category] = volume;

    for (auto& v : activeVoices_)
    {
        if (v.category == category && v.sourceVoice)
        {
            v.sourceVoice->SetVolume(v.baseVolume * volume);
        }
    }
}

void Audio::StopCategory(const std::string& category)
{
    for (auto it = activeVoices_.begin(); it != activeVoices_.end(); )
    {
        if (it->category == category)
        {
            if (it->sourceVoice) {
                it->sourceVoice->Stop();
                it->sourceVoice->FlushSourceBuffers();
                it->sourceVoice->DestroyVoice();
                it->sourceVoice = nullptr;
            }
            it = activeVoices_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}