#ifndef __HARDCODEDCOORDINATES_HLSLI__
#define __HARDCODEDCOORDINATES_HLSLI__

#ifdef _CUBE_COORD
static const float3 gVertices[36] = {
	// 앞면
	float3(-0.5, -0.5, -0.5),	// 왼쪽 아래
	float3(0.5,  0.5, -0.5),	// 오른쪽 위
	float3(0.5, -0.5, -0.5),	// 오른쪽 아래
	float3(-0.5, -0.5, -0.5),	// 왼쪽 아래
	float3(-0.5,  0.5, -0.5),	// 왼쪽 위
	float3(0.5,  0.5, -0.5),	// 오른쪽 위
	// 뒷면
	float3(-0.5, -0.5,  0.5),	// 왼쪽 아래
	float3(0.5, -0.5,  0.5),	// 오른쪽 아래
	float3(0.5,  0.5,  0.5),	// 오른쪽 위
	float3(-0.5, -0.5,  0.5),	// 왼쪽 아래
	float3(0.5,  0.5,  0.5),	// 오른쪽 위
	float3(-0.5,  0.5,  0.5),	// 왼쪽 위
	// 아랫면
	float3(-0.5, -0.5, -0.5),	// 왼쪽 아래 (앞쪽)
	float3(0.5, -0.5, -0.5),	// 오른쪽 아래 (앞쪽)
	float3(0.5, -0.5,  0.5),	// 오른쪽 아래 (뒤쪽)
	float3(-0.5, -0.5, -0.5),	// 왼쪽 아래 (앞쪽)
	float3(0.5, -0.5,  0.5),	// 오른쪽 아래 (뒤쪽)
	float3(-0.5, -0.5,  0.5),	// 왼쪽 아래 (뒤쪽)
	// 윗면
	float3(-0.5,  0.5, -0.5),	// 왼쪽 위 (앞쪽)
	float3(0.5,  0.5,  0.5),	// 오른쪽 위 (뒤쪽)
	float3(0.5,  0.5, -0.5),	// 오른쪽 위 (앞쪽)
	float3(-0.5,  0.5, -0.5),	// 왼쪽 위 (앞쪽)
	float3(-0.5,  0.5,  0.5),	// 왼쪽 위 (뒤쪽)
	float3(0.5,  0.5,  0.5),	// 오른쪽 위 (뒤쪽)
	// 왼쪽 면
	float3(-0.5, -0.5, -0.5),	// 왼쪽 아래
	float3(-0.5, -0.5,  0.5),	// 오른쪽 아래
	float3(-0.5,  0.5,  0.5),	// 오른쪽 위
	float3(-0.5, -0.5, -0.5),	// 왼쪽 아래
	float3(-0.5,  0.5,  0.5),	// 오른쪽 위
	float3(-0.5,  0.5, -0.5),	// 왼쪽 위
	// 오른쪽 면
	float3(0.5, -0.5, -0.5),	// 왼쪽 아래
	float3(0.5,  0.5,  0.5),	// 오른쪽 위
	float3(0.5, -0.5,  0.5),	// 오른쪽 아래
	float3(0.5, -0.5, -0.5),	// 왼쪽 아래
	float3(0.5,  0.5, -0.5),	// 왼쪽 위
	float3(0.5,  0.5,  0.5),	// 오른쪽 위
};
#endif // _CUBE_COORD

#ifdef _FIT_TO_SCREEN_COORD
static const float2 gVertices[6] = {
	float2(0.f, 1.f),
	float2(0.f, 0.f),
	float2(1.f, 0.f),
	float2(0.f, 1.f),
	float2(1.f, 0.f),
	float2(1.f, 1.f)
};
#endif // _FIT_TO_SCREEN_COORD

#endif // __HARDCODEDCOORDINATES_HLSLI__