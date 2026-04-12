#pragma once

#include "Asset.hpp"

class ASound : public Asset {
    friend class AssetManager;

public:
    ASound();
    ~ASound();


public:
    // roopCount : -1 (반복재생),  volume : 0 ~ 1(Volume), overlap : 같은 사운드를 중첩해서 켤 수 있는지
    bool Play(int roopCount, float volume, bool overlap, EAudioChannel::Type channelType, int& outChannelIndex);
    bool Play(int roopCount, float volume, bool overlap, EAudioChannel::Type channelType);
    void Stop();

    void RemoveChannel(FMOD::Channel* const targetChannel);

    // 0 ~ 1
    void SetVolume(float volume, int channelIndex);

public:
    CLONE(ASound);

    virtual bool Load(const std::wstring& filePath) override;
    virtual bool Save(const std::wstring& filePath) override;

private:
    FMOD::Sound* mpSound;                   // Sound 객체
    std::list<FMOD::Channel*> mChannels;    // Sound 가 재생되고 있는 채널 리스트
};