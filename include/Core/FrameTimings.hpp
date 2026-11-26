#pragma once

#include <array>
#include <chrono>
#include <deque>
#include <map>
#include <string>
#include <utility>

#include "Core/PerformanceTimer.hpp"

// Registration, recording, and reading take place on the frame thread.
class FrameTimings {
public:
	static constexpr int HistorySize = 240;
	using History = std::array<float, HistorySize>;

	class Series {
	public:
		const std::string& Name() const noexcept { return mName; }
		const std::string& Description() const noexcept { return mDescription; }
		float TargetFramesPerSecond() const noexcept { return mTargetFramesPerSecond; }
		const History& Samples() const noexcept { return mSamples; }

		// 한 프레임에서 여러 번 실행된 작업은 시간을 합산합니다.
		void Record(float milliseconds) noexcept { mElapsedMilliseconds += milliseconds; }

		template<typename Func>
		bool Measure(Func&& func) {
			PerformanceTimer timer;
			timer.Start();
			const bool result = func();
			timer.Stop();
			Record(static_cast<float>(timer.ElapsedMilliseconds()));
			return result;
		}

	private:
		friend class FrameTimings;

		std::string mName;
		std::string mDescription;
		float mTargetFramesPerSecond{};
		float mElapsedMilliseconds{};
		History mSamples{};
	};

	using GroupKey = std::pair<int, std::string>; // 표시 순서, 그룹 이름
	using Groups = std::map<GroupKey, std::deque<Series>>;
	using Tabs = std::map<std::string, Groups>;

	static FrameTimings& Get();

	// The returned series stays valid for the lifetime of the store.
	// Pass an empty tab name to display the group above all tabs.
	Series& Register(
		const char* tab,
		GroupKey group,
		const char* name,
		const char* description,
		float targetFramesPerSecond = 0.f);

	// 모든 항목을 같은 프레임 슬롯에 기록하고 누적 시간을 초기화합니다.
	void CommitFrame();

	const Tabs& GetTabs() const noexcept { return mTabs; }
	int SampleCount() const noexcept { return mSampleCount; }
	int NextSample() const noexcept { return mNextSample; }
	bool IsPaused() const noexcept { return mbPaused; }
	void SetPaused(bool paused) noexcept { mbPaused = paused; }

private:
	FrameTimings() = default;
	FrameTimings(const FrameTimings&) = delete;
	FrameTimings& operator=(const FrameTimings&) = delete;

	Tabs mTabs;
	int mSampleCount{};
	int mNextSample{};
	bool mbPaused{};
};
