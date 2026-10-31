#include "Core/pch.h"
#include "Core/PerformanceTimer.hpp"

void PerformanceTimer::Start() {
	mStartTime = std::chrono::high_resolution_clock::now();
}

void PerformanceTimer::Stop() {
	mEndTime = std::chrono::high_resolution_clock::now();
	mElapsedTime = mEndTime - mStartTime;
}