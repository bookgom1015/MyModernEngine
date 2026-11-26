#ifndef __D3D12AMBIENTOCCLUSION_HLSL__
#define __D3D12AMBIENTOCCLUSION_HLSL__

namespace AmbientOcclusion {
    //-----------------------------------------------------------------------------------------------------------
    //
    // 샘플과 기준점의 깊이 차이로 차폐 기여도를 계산한다.
    //
    // @param dist : 차폐 판정에 사용하는 부호 있는 깊이 차이
    // @param epsilon : 차폐로 인정하는 최소 깊이 차이
    // @param fadeStart : 차폐 감쇠 시작 거리
    // @param fadeEnd : 차폐 감쇠 종료 거리
    //
    // @return [0, 1] 범위의 차폐량
    //
    //-----------------------------------------------------------------------------------------------------------
    float OcclusionFunction(in float dist, in float epsilon, in float fadeStart, in float fadeEnd) {
		//       1.0     -------------.
		//               |           |  .
		//               |           |    .
		//               |           |      . 
		//               |           |        .
		//               |           |          .
		//               |           |            .
		//  ------|------|-----------|-------------|---------|--> zv
		//        0     Eps          z0            z1        
        float occlusion = 0.f;
        if (dist > epsilon) {
            const float FadeLength = fadeEnd - fadeStart;

			// 깊이 차이가 감쇠 시작점에서 끝점으로 증가하는 동안 차폐량을 1에서 0으로 선형 감소시킨다.
            occlusion = saturate((fadeEnd - dist) / FadeLength);
        }

        return occlusion;
    }
}

#endif // __D3D12AMBIENTOCCLUSION_HLSL__