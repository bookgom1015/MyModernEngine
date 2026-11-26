#ifndef __PERFORMANCETIMER_INL__
#define __PERFORMANCETIMER_INL__

double PerformanceTimer::ElapsedMilliseconds() const noexcept {
	return std::chrono::duration<double, std::milli>(mElapsedTime).count();
}

double PerformanceTimer::ElapsedSeconds() const noexcept {
	return mElapsedTime.count();
}

#endif // __PERFORMANCETIMER_INL__