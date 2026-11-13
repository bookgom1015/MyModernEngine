#ifndef __D3D12CROSSBILATERALWEIGHTS_HLSLI__
#define __D3D12CROSSBILATERALWEIGHTS_HLSLI__

#include "./../../include/Renderer/D3D12/D3D12HlslCompaction.h"
#include "./../../assets/Shader/D3D12/D3D12Svgf.hlsli"

namespace CrossBilateral {
    namespace Normal {
        struct Parameters {
            float Sigma;
            float SigmaExponent;
        };

        //-----------------------------------------------------------------------------------------------------------
        //
        // 기준 법선과 이웃 법선의 유사도로 가중치를 계산한다.
        //
        // @param targetNormal : 기준 법선
        // @param sampleNormals : 이웃 샘플의 법선 4개
        // @param params : 법선 가중치 설정
        //
        // @return 샘플별 법선 가중치
        //
        //-----------------------------------------------------------------------------------------------------------
        float4 GetWeights(
				in float3 targetNormal,
				in float3 sampleNormals[4],
				in Parameters params) {
            float4 NdotSampleN = float4(
				dot(targetNormal, sampleNormals[0]),
				dot(targetNormal, sampleNormals[1]),
				dot(targetNormal, sampleNormals[2]),
				dot(targetNormal, sampleNormals[3])
			);
			// 법선 내적에 보정 배율을 적용한다.
			// 배율을 1보다 크게 하면 법선 차이에 대한 허용 범위가 커진다.
			// 법선 차이가 실제보다 크게 나타나는 경우에 적용한다.
			// 예를 들어 낮은 정밀도로 법선을 양자화했을 때 발생할 수 있다.
            NdotSampleN *= params.Sigma;

            const float4 NormalWeights = pow(saturate(NdotSampleN), params.SigmaExponent);

            return NormalWeights;
        }
    }

	// 선형 깊이.
    namespace Depth {
        struct Parameters {
            float Sigma;
            float WeightCutoff;
            uint NumMantissaBits;
        };

        //-----------------------------------------------------------------------------------------------------------
        //
        // 깊이 차이와 저장 정밀도를 반영해 가중치를 계산한다.
        //
        // @param targetDepth : 기준 선형 깊이
        // @param ddxy : 축별 깊이 미분
        // @param sampleDepths : 이웃 샘플의 깊이 4개
        // @param params : 깊이 가중치 설정
        //
        // @return 샘플별 깊이 가중치
        //
        //-----------------------------------------------------------------------------------------------------------
        float4 GetWeights(
				in float targetDepth,
				in float2 ddxy,
				in float4 sampleDepths,
				in Parameters params) {
            const float DepthThreshold = dot(1, abs(ddxy));
            const float DepthFloatPrecision = FloatPrecision::FloatPrecision(targetDepth, params.NumMantissaBits);

            const float DepthTolerance = params.Sigma * DepthThreshold + DepthFloatPrecision;
            
            float4 depthWeights = min(DepthTolerance / (abs(sampleDepths - targetDepth) + DepthFloatPrecision), 1);
            depthWeights *= depthWeights >= params.WeightCutoff;

            return depthWeights;
        }

        //-----------------------------------------------------------------------------------------------------------
        //
        // 샘플 오프셋에 맞춰 깊이 미분을 재계산한 뒤 가중치를 구한다.
        //
        // @param targetDepth : 기준 선형 깊이
        // @param ddxy : 축별 깊이 미분
        // @param sampleDepths : 이웃 샘플의 깊이 4개
        // @param sampleOffset : 깊이 미분을 재계산할 샘플 간 오프셋
        // @param params : 깊이 가중치 설정
        //
        // @return 샘플별 깊이 가중치
        //
        //-----------------------------------------------------------------------------------------------------------
        float4 GetWeights(
				in float targetDepth,
				in float2 ddxy,
				in float4 sampleDepths,
				in float2 sampleOffset, // 깊이 미분을 재계산할 샘플 간 오프셋.
				in Parameters params) {
            const float2 RemappedDdxy = Svgf::RemapDdxy(targetDepth, ddxy, sampleOffset);
            
            return GetWeights(targetDepth, RemappedDdxy, sampleDepths, params);
        }
    }

    namespace Bilinear {
        //-----------------------------------------------------------------------------------------------------------
        //
        // 인접한 네 샘플의 쌍선형 보간 가중치를 계산한다.
        //
        // @param targetOffset : 왼쪽 위 샘플에서 보간 지점까지의 상대 오프셋
        //
        // @return 왼쪽 위, 오른쪽 위, 왼쪽 아래, 오른쪽 아래 순서의 가중치
        //
        //-----------------------------------------------------------------------------------------------------------
        float4 GetWeights(in float2 targetOffset) {
            const float4 BilinearWeights = float4(
				(1 - targetOffset.x) * (1 - targetOffset.y),
				targetOffset.x * (1 - targetOffset.y),
				(1 - targetOffset.x) * targetOffset.y,
				targetOffset.x * targetOffset.y
			);
            
            return BilinearWeights;
        }
    }

    namespace BilinearDepthNormal {
        struct Parameters {
            Normal::Parameters Normal;
            Depth::Parameters Depth;
        };

        //-----------------------------------------------------------------------------------------------------------
        //
        // 쌍선형 보간, 법선, 오프셋 보정 깊이 가중치를 결합한다.
        //
        // @param targetDepth : 기준 선형 깊이
        // @param targetNormal : 기준 법선
        // @param targetOffset : 왼쪽 위 샘플에서 보간 지점까지의 상대 오프셋
        // @param ddxy : 축별 깊이 미분
        // @param sampleDepths : 이웃 샘플의 깊이 4개
        // @param sampleNormals : 이웃 샘플의 법선 4개
        // @param samplesOffset : 깊이 미분을 재계산할 샘플 간 오프셋
        // @param params : 깊이 및 법선 가중치 설정
        //
        // @return 샘플별 결합 가중치
        //
        //-----------------------------------------------------------------------------------------------------------
        float4 GetWeights(
				in float targetDepth,
				in float3 targetNormal,
				in float2 targetOffset,
				in float2 ddxy,
				in float4 sampleDepths,
				in float3 sampleNormals[4],
				in float2 samplesOffset,
				Parameters params) {
            const float4 BilinearWeights = Bilinear::GetWeights(targetOffset);
            const float4 DepthWeights = Depth::GetWeights(targetDepth, ddxy, sampleDepths, samplesOffset, params.Depth);
            const float4 NormalWeights = Normal::GetWeights(targetNormal, sampleNormals, params.Normal);

            return BilinearWeights * DepthWeights * NormalWeights;
        }

        //-----------------------------------------------------------------------------------------------------------
        //
        // 쌍선형 보간, 법선, 깊이 가중치를 결합한다.
        //
        // @param targetDepth : 기준 선형 깊이
        // @param targetNormal : 기준 법선
        // @param targetOffset : 왼쪽 위 샘플에서 보간 지점까지의 상대 오프셋
        // @param ddxy : 축별 깊이 미분
        // @param sampleDepths : 이웃 샘플의 깊이 4개
        // @param sampleNormals : 이웃 샘플의 법선 4개
        // @param params : 깊이 및 법선 가중치 설정
        //
        // @return 샘플별 결합 가중치
        //
        //-----------------------------------------------------------------------------------------------------------
        float4 GetWeights(
				in float targetDepth,
				in float3 targetNormal,
				in float2 targetOffset,
				in float2 ddxy,
				in float4 sampleDepths,
				in float3 sampleNormals[4],
				Parameters params) {
            const float4 BilinearWeights = Bilinear::GetWeights(targetOffset);
            const float4 DepthWeights = Depth::GetWeights(targetDepth, ddxy, sampleDepths, params.Depth);
            const float4 NormalWeights = Normal::GetWeights(targetNormal, sampleNormals, params.Normal);

            return BilinearWeights * DepthWeights * NormalWeights;
        }
    }
}

#endif // __D3D12CROSSBILATERALWEIGHTS_HLSLI__