#include "pch.h"
#include "ASound.hpp"

#include "AudioManager.hpp"

namespace {
	FMOD_RESULT ChannelCallback(
		FMOD_CHANNELCONTROL* channelcontrol
		, FMOD_CHANNELCONTROL_TYPE controltype
		, FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype
		, void* commanddata1, void* commanddata2) {
		auto cppchannel = (FMOD::Channel*)channelcontrol;
		ASound* pSound{};

		switch (controltype) {
		// 사운즈 재생 종료시 발생하는 이벤트
		case FMOD_CHANNELCONTROL_CALLBACK_END: {
			cppchannel->getUserData((void**)&pSound);
			pSound->RemoveChannel(cppchannel);
		}
		break;
		}

		return FMOD_OK;
	}
}

ASound::ASound() 
	: Asset{EAsset::E_Sound}
	, mpSound{} {}

ASound::~ASound() {
	if (mpSound) {
		auto result = mpSound->release();
		mpSound = nullptr;
	}
}

bool ASound::Play(int roopCount, float volume, bool overlap, EAudioChannel::Type channelType, int& outChannelIndex) {
	// 중첩재생 X + 이미 다른채널에서 재생중 ==> 재생 불가
	if (!overlap && !mChannels.empty())
		ReturnFalse("Sound is already playing and overlap is not allowed.");

	if (roopCount > 0) --roopCount;

	auto channelGroup = AUDIO_MANAGER->GetChannelGroup(channelType);

	FMOD::Channel* pChannel{};
	AUDIO_FMOD_SYSTEM->playSound(mpSound, channelGroup, false, &pChannel);

	// 재생을 했는데, 재생중인 채널이 없다 --> 실패
	if (!pChannel)
		ReturnFalse("Failed to play sound.");

	pChannel->setVolume(volume);

	pChannel->setCallback(&ChannelCallback);
	pChannel->setUserData(this);

	pChannel->setMode(FMOD_LOOP_NORMAL);
	pChannel->setLoopCount(roopCount);

	// 어떤 채널에서 Sound 가 재생중인지 기록
	mChannels.push_back(pChannel);

	int iIdx = -1;
	pChannel->getIndex(&iIdx);

	outChannelIndex = iIdx;

	return true;
}

bool ASound::Play(int roopCount, float volume, bool overlap, EAudioChannel::Type channelType) {
	// 중첩재생 X + 이미 다른채널에서 재생중 ==> 재생 불가
	if (!overlap && !mChannels.empty())
		ReturnFalse("Sound is already playing and overlap is not allowed.");

	if (roopCount > 0) --roopCount;

	auto channelGroup = AUDIO_MANAGER->GetChannelGroup(channelType);

	FMOD::Channel* pChannel{};
	AUDIO_FMOD_SYSTEM->playSound(mpSound, channelGroup, false, &pChannel);

	// 재생을 했는데, 재생중인 채널이 없다 --> 실패
	if (!pChannel)
		ReturnFalse("Failed to play sound.");

	pChannel->setVolume(volume);

	pChannel->setCallback(&ChannelCallback);
	pChannel->setUserData(this);

	pChannel->setMode(FMOD_LOOP_NORMAL);
	pChannel->setLoopCount(roopCount);

	// 어떤 채널에서 Sound 가 재생중인지 기록
	mChannels.push_back(pChannel);

	return true;
}


void ASound::Stop() {
	std::list<FMOD::Channel*>::iterator iter{};

	while (!mChannels.empty()) {
		iter = mChannels.begin();
		(*iter)->stop();
	}
}

void ASound::RemoveChannel(FMOD::Channel* const targetChannel) {
	auto iter = mChannels.begin();

	for (; iter != mChannels.end(); ++iter) {
		if (*iter == targetChannel) {
			mChannels.erase(iter);
			return;
		}
	}
}

void ASound::SetVolume(float volume, int channelIndex) {
	auto iter = mChannels.begin();

	int iIdx = -1;
	for (; iter != mChannels.end(); ++iter)	{
		(*iter)->getIndex(&iIdx);
		if (channelIndex == iIdx) {
			(*iter)->setVolume(volume);
			return;
		}
	}
}

bool ASound::Load(const std::wstring& filePath) {
	auto path = WStrToStr(filePath);

	auto result = AUDIO_FMOD_SYSTEM->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &mpSound);
	if (FMOD_OK != result) 
		ReturnFalseFormat("Failed to create sound from file: {}", path);

	return true;
}

bool ASound::Save(const std::wstring& filePath) { return true; }