#include "Core/pch.h"
#include "Core/FrameTimings.hpp"

#include <algorithm>

FrameTimings& FrameTimings::Get() {
	static FrameTimings timings;
	return timings;
}

FrameTimings::Series& FrameTimings::Register(
	const char* tab,
	GroupKey group,
	const char* name,
	const char* description,
	float targetFramesPerSecond) {
	auto& series = mTabs[tab][std::move(group)].emplace_back();
	series.mName = name;
	series.mDescription = description;
	series.mTargetFramesPerSecond = targetFramesPerSecond;
	return series;
}

void FrameTimings::CommitFrame() {
	for (auto& tab : mTabs) {
		for (auto& group : tab.second) {
			for (auto& series : group.second) {
				if (!mbPaused)
					series.mSamples[mNextSample] = series.mElapsedMilliseconds;

				// 일시정지 중에 측정한 시간은 재개 후 첫 프레임에 이월하지 않습니다.
				series.mElapsedMilliseconds = 0.f;
			}
		}
	}

	if (!mbPaused) {
		mNextSample = (mNextSample + 1) % HistorySize;
		mSampleCount = (std::min)(mSampleCount + 1, HistorySize);
	}
}
