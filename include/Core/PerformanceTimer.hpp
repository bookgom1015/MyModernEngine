#pragma once

class PerformanceTimer {
public:
	__forceinline double ElapsedMilliseconds() const noexcept;
	__forceinline double ElapsedSeconds() const noexcept;

public:
	void Start();
	void Stop();

private:
	std::chrono::high_resolution_clock::time_point mStartTime;
	std::chrono::high_resolution_clock::time_point mEndTime;
	std::chrono::duration<double> mElapsedTime;
};

#include "PerformanceTimer.inl"