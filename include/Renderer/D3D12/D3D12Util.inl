#ifndef __D3D12UTIL_INL__
#define __D3D12UTIL_INL__

UINT D3D12Util::CeilDivide(UINT value, UINT divisor) { return (value + divisor - 1) / divisor; }

float D3D12Util::Lerp(float a, float b, float t) { return a + t * (b - a); }

float D3D12Util::Clamp(float a, float _min, float _max) {
	return std::max(_min, std::min(_max, a));
}

#endif // __D3D12UTIL_INL__