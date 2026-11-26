//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

// A-trous 웨이블릿 변환 교차 양방향 필터.
// SVGF 필터의 첫 번째 패스를 기반으로 한다.
// 참고 자료 [Dammertz2010], Edge-Avoiding A-Trous Wavelet Transform for Fast Global Illumination Filtering
// 참고 자료 [SVGF], Spatiotemporal Variance-Guided Filtering
// 참고 자료 [RTGCH19] Ray Tracing Gems (Ch 19)

#ifndef __D3D12ATROUSWAVELETTRANSFORMFILTER_HLSLI__
#define __D3D12ATROUSWAVELETTRANSFORMFILTER_HLSLI__

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/D3D12/D3D12Svgf.hlsli"
#include "./../../assets/Shader/Kernels.hlsli"

ConstantBuffer<AtrousWaveletTransformFilterCB> cbAtrous : register(b0);

SVGF_AtrousWaveletTransformFilter_RootConstants(b1)

Texture2D<Svgf::ValueMapFormat>						gi_Value					: register(t0);
Texture2D<GBuffer::NormalDepthMapFormat>			gi_NormalDepth				: register(t1);
Texture2D<Svgf::VarianceMapFormat>					gi_Variance					: register(t2);
Texture2D<Svgf::RayHitDistanceMapFormat>			gi_HitDistance				: register(t3);
Texture2D<Svgf::DepthPartialDerivativeMapFormat>	gi_DepthPartialDerivative	: register(t4);

RWTexture2D<Svgf::ValueMapFormat>					go_FilteredValue			: register(u0);

//-----------------------------------------------------------------------------------------------------------
//
// 픽셀 간격과 깊이 미분으로 예상 깊이 변화량을 구한다.
//
// @param depth : 기준 픽셀 깊이
// @param ddxy : 축별 깊이 미분
// @param pixelOffset : 기준 픽셀에서 샘플까지의 오프셋
//
// @return 깊이 가중치 계산에 사용할 예상 깊이 변화량
//
//-----------------------------------------------------------------------------------------------------------
float DepthThreshold(float depth, float2 ddxy, float2 pixelOffset) {
	float depthThreshold;
	if (cbAtrous.PerspectiveCorrectDepthInterpolation) {
		float2 newDdxy = Svgf::RemapDdxy(depth, ddxy, pixelOffset);
		depthThreshold = dot(1.f, abs(newDdxy));
	}
	else {
		depthThreshold = dot(1.f, abs(pixelOffset * ddxy));
	}
	return depthThreshold;
}

//-----------------------------------------------------------------------------------------------------------
//
// 유효한 이웃 샘플의 값과 필터 가중치를 누적한다.
//
// @param weightedValueSum : 가중치가 적용된 값의 누적합
// @param weightSum : 가중치 누적합
// @param value : 기준 픽셀의 값
// @param stdDeviation : 기준 픽셀 값의 표준편차
// @param depth : 기준 픽셀 깊이
// @param normal : 기준 픽셀 법선
// @param ddxy : 축별 깊이 미분
// @param row : 커널 행
// @param col : 커널 열
// @param kernelStep : 축별 샘플 간격
// @param DTid : 기준 픽셀 좌표
//
//-----------------------------------------------------------------------------------------------------------
void AddFilterContribution(
		inout float weightedValueSum,
		inout float weightSum,
		in float value,
		in float stdDeviation,
		in float depth,
		in float3 normal,
		in float2 ddxy,
		in uint row,
		in uint col,
		in uint2 kernelStep,
		in uint2 DTid) {
	const float ValueSigma = cbAtrous.ValueSigma;
	const float NormalSigma = cbAtrous.NormalSigma;
	const float DepthSigma = cbAtrous.DepthSigma;

	int2 pixelOffset;
	float kernelWidth;
	float varianceScale = 1.f;

	pixelOffset = int2(row - FilterKernel::Radius, col - FilterKernel::Radius) * kernelStep;
	int2 id = int2(DTid) + pixelOffset;

	if (!ShaderUtil::IsWithinBounds(id, cbAtrous.TextureDim)) return;

	float iDepth;
	float3 iNormal;
	ValuePackaging::DecodeNormalDepth(gi_NormalDepth[id], iNormal, iDepth);

	float iValue = gi_Value[id];
	bool isValidValue = iValue != Svgf::InvalidValue;
	if (!isValidValue || iDepth == 0.f) return;

	// 이웃 샘플 기여도의 가중치를 계산한다.
	float w;
	{
		// 샘플 값에 따른 가중치.
		// 먼 이웃에는 허용하는 값 차이를 줄여 급격한 변화가 과하게 흐려지는 것을 막는다.
		const float ErrorOffset = 0.005f;
		float valueSigmaDistCoef = 1.f / length(pixelOffset);
		float variance = Svgf::ColorVariance(value, iValue);
		float e_x = -abs(variance) / (valueSigmaDistCoef * ValueSigma * stdDeviation + ErrorOffset);
		float w_x = exp(e_x);

		// 법선에 따른 가중치.
		float w_n = pow(max(0.f, dot(normal, iNormal)), NormalSigma);

		// 깊이에 따른 가중치.
		float w_d;
		{
			float2 pixelOffsetForDepth = pixelOffset;

			// 양방향 다운샘플링을 적용한 깊이 미분 버퍼의 샘플 오프셋을 반영한다.
			if (cbAtrous.UsingBilateralDownsamplingBuffers) {
				float2 offsetSign = sign(pixelOffset);
				pixelOffsetForDepth = pixelOffset + offsetSign * float2(0.5, 0.5);
			}

			float depthFloatPrecision = FloatPrecision::FloatPrecision(max(depth, iDepth), cbAtrous.DepthNumMantissaBits);
			float depthThreshold = DepthThreshold(depth, ddxy, pixelOffsetForDepth);
			float depthTolerance = DepthSigma * depthThreshold + depthFloatPrecision;
			float delta = abs(depth - iDepth);
			// 저장 정밀도 이내의 깊이 차이를 무시해 밴딩을 줄인다.
			delta = max(0, delta - depthFloatPrecision);
			w_d = exp(-delta / depthTolerance);

			// 허용 범위를 벗어난 샘플의 기여도를 줄이고 가중치가 임계값에 못 미치면 제외한다.
			w_d *= w_d >= cbAtrous.DepthWeightCutoff;
		}

		// 필터 커널 가중치.
		float w_h = FilterKernel::Kernel[row][col];

		// 최종 가중치.
		w = w_h * w_n * w_x * w_d;
	}

	weightedValueSum += w * iValue;
	weightSum += w;
}

[numthreads(
//-----------------------------------------------------------------------------------------------------------
//
// 픽셀마다 적응형 A-trous 필터를 적용해 결과 텍스처에 기록한다.
//
// @param DTid : 디스패치 스레드의 픽셀 좌표
//
//-----------------------------------------------------------------------------------------------------------
    Svgf::ThreadGroup::Atrous::Width, 
    Svgf::ThreadGroup::Atrous::Height, 
    Svgf::ThreadGroup::Atrous::Depth)]
void CS(uint2 DTid : SV_DispatchThreadID) {
	if (!ShaderUtil::IsWithinBounds(DTid, cbAtrous.TextureDim)) return;

	// 현재 픽셀과 커널 중심을 기준으로 누적값을 초기화한다.
	float value = gi_Value[DTid];
	
	float3 normal;
	float depth;
	ValuePackaging::DecodeNormalDepth(gi_NormalDepth[DTid], normal, depth);

	const bool IsValidValue = value != Svgf::InvalidValue;
	float filteredValue = value;
	float variance = gi_Variance[DTid];

	if (depth != 0.f) {
		float2 ddxy = gi_DepthPartialDerivative[DTid];		
		float stdDeviation = 1.f;
		float weightSum = 0.f;
		float weightedValueSum = 0.f;

		if (IsValidValue) {
			float w = FilterKernel::Kernel[FilterKernel::Radius][FilterKernel::Radius];
			weightSum = w;
			weightedValueSum = weightSum * value;
			stdDeviation = sqrt(variance);
		}

		// 적응형 커널 크기.
		// AO 광선의 충돌 거리에 따라 커널의 샘플 간격을 조절한다.
		// 얼룩처럼 보이는 저주파 노이즈를 줄인다.
		// 참고 자료 [RTGCH19]
		uint2 kernelStep = 0;
		if (cbAtrous.UseAdaptiveKernelSize && IsValidValue) {
			float avgRayHitDistance = gi_HitDistance[DTid];

			float perPixelViewAngle = cbAtrous.FovY / cbAtrous.TextureDim.y;
			float tan_a = tan(perPixelViewAngle);
			float2 projectedSurfaceDim = Svgf::ApproximateProjectedSurfaceDimensionsPerPixel(depth, ddxy, tan_a);

			// 광선 충돌 거리를 픽셀이 덮는 표면 크기로 나눠 커널 너비를 구한다.
			// 상대적인 광선 충돌 거리에 따라 비선형 보정 계수를 적용한다.
			// 가까운 차폐물이 반구의 일부만 덮으면 평균 광선 충돌 거리가 빠르게 증가한다.
			// 이 경우 더 작은 커널을 사용하면 차폐 디테일을 보존할 수 있다.
			float t = min(avgRayHitDistance / 22.0, 1); // 22는 실험적으로 정한 값이다.
			float k = gRayHitDistanceToKernelWidthScale * pow(t, gRayHitDistanceToKernelSizeScaleExponent);
			kernelStep = max(1, round(k * avgRayHitDistance / projectedSurfaceDim));

			uint2 targetKernelStep = clamp(kernelStep, (cbAtrous.MinKernelWidth - 1) / 2, (cbAtrous.MaxKernelWidth - 1) / 2);

			// TODO 추가로 검토할 항목.
			// X·Y 커널 반경이 다르면 줄무늬가 나타난다. 같은 비율을 쓰면 급경사에서 한 축이 과하게 흐려질 수 있다.
			// 픽셀당 누적 샘플 수가 적을 때 더 큰 커널을 사용한다.
			// 목표 커널 간격에 따라 반복 횟수를 바꿔 시간에 걸쳐 더 넓게 샘플링한다. 큰 커널에는 반복을 늘린다.
			uint2 adjustedKernelStep = lerp(1, targetKernelStep, cbAtrous.KernelRadiusLerfCoef);
			kernelStep = adjustedKernelStep;
		}

		if (variance >= cbAtrous.MinVarianceToDenoise) {
			// 이웃 샘플의 기여도를 더한다.
			[unroll]
			for (uint r = 0; r < FilterKernel::Width; ++r) {
				[unroll]
				for (uint c = 0; c < FilterKernel::Width; ++c) {
					if (r != FilterKernel::Radius || c != FilterKernel::Radius) {
						AddFilterContribution(
							weightedValueSum,
							weightSum,
							value,
							stdDeviation,
							depth,
							normal,
							ddxy,
							r, c,
							kernelStep,
							DTid
						);
					}
				}
			}
		}

		float smallValue = 1e-6f;
		if (weightSum > smallValue) filteredValue = weightedValueSum / weightSum;
		else filteredValue = Svgf::InvalidValue;
	}

	go_FilteredValue[DTid] = filteredValue;
}

#endif // __D3D12ATROUSWAVELETTRANSFORMFILTER_HLSLI__