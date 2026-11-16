#pragma once

#include "Asset.hpp"

class ASound : public Asset {
    friend class AssetManager;

public:
    ASound();
    ~ASound();


public:
    // loopCount가 -1이면 반복 재생합니다. volume은 0~1이며, overlap은 같은 소리의 중첩 재생을 허용할지 지정합니다.
    bool Play(int roopCount, float volume, bool overlap, EAudioChannel::Type channelType, int& outChannelIndex);
    bool Play(int roopCount, float volume, bool overlap, EAudioChannel::Type channelType);
    void Stop();

    void RemoveChannel(FMOD::Channel* const targetChannel);

    // 볼륨의 범위는 0~1입니다.
    void SetVolume(float volume, int channelIndex);

public:
    CLONE(ASound);

    virtual bool Load(const std::wstring& filePath) override;
    virtual bool Save(const std::wstring& filePath) override;

private:
    FMOD::Sound* mpSound;                   // FMOD 사운드 객체입니다.
    std::list<FMOD::Channel*> mChannels;    // 현재 사운드를 재생 중인 채널 목록입니다.
};