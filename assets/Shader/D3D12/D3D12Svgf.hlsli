#ifndef __D3D12SVGF_HLSLI__
#define __D3D12SVGF_HLSLI__

namespace Svgf {
    //-----------------------------------------------------------------------------------------------------------
    //
    // 박스 내부 광선이 빠져나가는 지점을 이용해 큐브맵 조회 방향을 보정한다.
    //
    // @param rayOrigin : 박스 내부 광선 시작점
    // @param unitRayDir : 정규화된 광선 방향
    // @param boxCenter : 박스 중심
    // @param boxExtents : 박스의 축별 반길이
    //
    // @return 박스 중심에서 광선이 빠져나가는 지점까지의 벡터
    //
    //-----------------------------------------------------------------------------------------------------------
    float3 BoxCubeMapLookup(in float3 rayOrigin, in float3 unitRayDir, in float3 boxCenter, in float3 boxExtents) {
	    // 박스 중심을 좌표 원점으로 옮긴다.
        const float3 p = rayOrigin - boxCenter;

	    // AABB의 i번째 축에 수직인 두 평면과 광선의 교차 매개변수를 구한다.
	    //
	    // t1 = (-dot(n_i, p) + h_i) / dot(n_i, d) = (-p_i + h_i) / d_i
	    // t2 = (-dot(n_i, p) - h_i) / dot(n_i, d) = (-p_i - h_i) / d_i
        const float3 t1 = (-p + boxExtents) / unitRayDir;
        const float3 t2 = (-p - boxExtents) / unitRayDir;

	    // 축마다 두 교차 매개변수 중 큰 값을 선택한다.
	    // 광선이 박스 안에서 시작하므로 축마다 나가는 쪽의 교차점만 필요하다.
        const float3 tmax = max(t1, t2);

	    // 축별 이탈 매개변수 중 가장 작은 값을 선택한다.
        const float t = min(min(tmax.x, tmax.y), tmax.z);

	    // 큐브맵을 샘플링할 때는
	    // 박스 중심을 기준으로 한 방향을 사용한다.
        return p + t * unitRayDir;
    }

    //-----------------------------------------------------------------------------------------------------------
    //
    // @param ref : 비교 기준 값
    // @param values : 비교할 값 두 개
    //
    // @return 기준 값과 절대 차이가 가장 작은 성분의 인덱스
    //
    //-----------------------------------------------------------------------------------------------------------
    uint GetIndexOfValueClosestToReference(in float ref, in float2 values) {
        const float2 delta = abs(ref - values);
    
        const uint index = delta[1] < delta[0] ? 1 : 0;
    
        return index;
    }

    //-----------------------------------------------------------------------------------------------------------
    //
    // @param ref : 비교 기준 값
    // @param values : 비교할 값 네 개
    //
    // @return 기준 값과 절대 차이가 가장 작은 성분의 인덱스
    //
    //-----------------------------------------------------------------------------------------------------------
    uint GetIndexOfValueClosestToReference(in float ref, in float4 values) {
        const float4 delta = abs(ref - values);
    
        uint index = delta[1] < delta[0] ? 1 : 0;
        index = delta[2] < delta[index] ? 2 : index;
        index = delta[3] < delta[index] ? 3 : index;
    
        return index;
    }

    //-----------------------------------------------------------------------------------------------------------
    //
    // 원근 보간으로 단위 픽셀 간격의 깊이 미분을 지정한 간격에 맞춘다.
    //
    // @param z0 : 기준 선형 깊이
    // @param ddxy : 단위 픽셀 간격의 축별 깊이 미분
    // @param pixelOffset : 새 샘플의 픽셀 오프셋
    //
    // @return 지정 간격에 맞춘 축별 깊이 변화량
    //
    //-----------------------------------------------------------------------------------------------------------
    float2 RemapDdxy(in float z0, in float2 ddxy, in float2 pixelOffset) {
	    // 비선형 깊이 보간에 원근 보정을 적용한다.
	    // 참고 자료 https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation
	    // z0에서 z1까지 오프셋 q에 해당하는 깊이 z를 구하는 보간식.
	    //           z =  1 / (1 / z0 * (1 - q) + 1 / z1 * q)
	    // z1 = z0 + ddxy이며 z1은 단위 픽셀 오프셋 [1, 1]의 깊이다.
	    // ddxy로 정리하면 다음과 같다.
	    //
	    //           z = (z0 + ddxy) / (1 + (1-q) / z0 * ddxy) 
        const float2 z = (z0 + ddxy) / (1 + ((1 - pixelOffset) / z0) * ddxy);
    
        return sign(pixelOffset) * (z - z0);
    }

    //-----------------------------------------------------------------------------------------------------------
    //
    // 픽셀마다 시야각 간격이 같다고 가정해 한 픽셀이 덮는 표면 크기를 근사한다.
    //
    // @param z : 픽셀에 해당하는 표면의 선형 깊이
    // @param ddxy : 축별 깊이 미분
    // @param tan_a : 픽셀당 시야각의 탄젠트
    //
    // @return 축별 표면 크기 근삿값
    //
    //-----------------------------------------------------------------------------------------------------------
    float2 ApproximateProjectedSurfaceDimensionsPerPixel(in float z, in float2 ddxy, in float tan_a) {
	    // 뷰 평면에 평행한 표면이 깊이 z에 있을 때의 픽셀당 크기를 구한다.
        const float2 dx = tan_a * z;
	    // 피타고라스 정리로 깊이 변화량을 반영한 표면 크기를 근사한다.
        const float2 w = sqrt(dx * dx + ddxy * ddxy);
    
        return w;
    }

    //-----------------------------------------------------------------------------------------------------------
    //
    // @param lval : 첫 번째 색상
    // @param rval : 두 번째 색상
    //
    // @return RGB 성분 차이의 제곱평균제곱근
    //
    //-----------------------------------------------------------------------------------------------------------
    float ColorVariance(in float4 lval, in float4 rval) {
        const float3 diff = (lval - rval).rgb;
        const float variance = sqrt(dot(diff, diff)) * 0.577350269189;
    
        return variance;
    }
}

#endif // __D3D12SVGF_HLSLI__