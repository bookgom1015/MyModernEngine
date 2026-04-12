#include "pch.h"
#include "AudioManager.hpp"

AudioManager::AudioManager() 
	: mpSystem{} {}

AudioManager::~AudioManager() {
    if (mpSystem) {
        mpSystem->close();
        mpSystem->release();
        mpSystem = nullptr;
    }
}

bool AudioManager::Initialize() {
    FMOD::System_Create(&mpSystem);
    assert(mpSystem);

    FMOD_RESULT result = mpSystem->init(EAudioChannel::Count, FMOD_DEFAULT, nullptr);

    // Master 가져오기
    mpSystem->getMasterChannelGroup(&mChannelGroups[(int)EAudioChannel::E_Master]);

    // 나머지 채널 그룹들
    mpSystem->createChannelGroup("Editor", &mChannelGroups[(int)EAudioChannel::E_Editor]);
    mpSystem->createChannelGroup("Music", &mChannelGroups[(int)EAudioChannel::E_Music]);
    mpSystem->createChannelGroup("SFX", &mChannelGroups[(int)EAudioChannel::E_SFX]);
    mpSystem->createChannelGroup("Voice", &mChannelGroups[(int)EAudioChannel::E_Voice]);

	for (int i = EAudioChannel::E_CommonChannel0; i < EAudioChannel::Count; ++i) {
        std::string name = std::format("CommonChannel{}", (i - (int)EAudioChannel::E_CommonChannel0));
        mpSystem->createChannelGroup(name.c_str(), &mChannelGroups[i]);
    }

	// master에 나머지 채널 그룹들 추가
    auto& master = mChannelGroups[EAudioChannel::E_Master];
	for (int i = EAudioChannel::E_Editor; i < EAudioChannel::Count; ++i)
        master->addGroup(mChannelGroups[i]);

	return true;
}