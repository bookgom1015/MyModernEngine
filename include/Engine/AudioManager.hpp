#pragma once

class AudioManager : public Singleton<AudioManager> {
	SINGLETON(AudioManager);

public:
	bool Initialize();

public:
	__forceinline FMOD::System* GetSystem() const noexcept { return mpSystem; }

	__forceinline FMOD::ChannelGroup* GetChannelGroup(EAudioChannel::Type type) const noexcept {
		return mChannelGroups[type];
	}

private:
	FMOD::System* mpSystem;

	FMOD::ChannelGroup* mChannelGroups[EAudioChannel::Count];
};

#ifndef AUDIO_MANAGER
#define AUDIO_MANAGER AudioManager::GetInstance()
#endif // AUDIO_MANAGER

#ifndef AUDIO_FMOD_SYSTEM
#define AUDIO_FMOD_SYSTEM AUDIO_MANAGER->GetSystem()
#endif // FMOD_SYSTEM