// [ 의존성 ]
//  - ShaderUtil.hlsli
//
// [ 참고 자료 ]
//  - https://xtozero.tistory.com/5
//   -- https://github.com/diharaw/volumetric-fog
//   -- https://github.com/bartwronski/CSharpRenderer
//   -- https://github.com/Unity-Technologies/VolumetricLighting/tree/master/Assets/VolumetricFog

#ifndef __D3D12VOLUMETRICLIGHT_HLSLI__
#define __D3D12VOLUMETRICLIGHT_HLSLI__

namespace VolumetricLight {
	//-----------------------------------------------------------------------------------------------------------
	//
	// @param wi : 빛의 입사 방향
	// @param wo : 빛의 출사 방향
	// @param g : 비등방성 계수
	//
	// @return 입사 방향과 출사 방향 사이의 Henyey–Greenstein 위상 함수 값
	//
	//-----------------------------------------------------------------------------------------------------------
	float HenyeyGreensteinPhaseFunction(float3 wi, float3 wo, float g) {
		const float CosTheta = dot(wi, wo);
		const float g2 = g * g;
		const float denom = pow(1.f + g2 - 2.f * g * CosTheta, 3.f / 2.f);
		return (1.f / (4.f * PI)) * ((1.f - g2) / max(denom, FLT_EPSILON));
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//
	// @param z_ndc : 현재 슬라이스의 정규화 깊이
	// @param z_exp : 깊이 분포 지수
	// @param near : 근평면 거리
	// @param far : 원평면 거리
	// @param dimZ : 깊이 축 슬라이스 수
	//
	// @return 현재 슬라이스와 다음 슬라이스의 뷰 공간 깊이 차이
	//
	//-----------------------------------------------------------------------------------------------------------
	float SliceTickness(float z_ndc, float z_exp, float near, float far, uint dimZ) {
		const float CurrDepthV = ShaderUtil::NdcDepthToExpViewDepth(z_ndc, z_exp, near, far);
		const float NextDepthV = ShaderUtil::NdcDepthToExpViewDepth(z_ndc + 1.f / (float)dimZ, z_exp, near, far);
		return NextDepthV - CurrDepthV;
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//
	// 현재 슬라이스의 체적 산란광과 투과율을 누적한다.
	//
	// @param accumLight : 누적 산란광
	// @param accumTransmittance : 누적 투과율
	// @param sliceLight : 현재 슬라이스의 조명
	// @param sliceDensity : 현재 슬라이스의 밀도
	// @param tickness : 슬라이스 두께
	// @param densityScale : 밀도 배율
	//
	// @return 누적 산란광 RGB와 누적 투과율 A
	//
	//-----------------------------------------------------------------------------------------------------------
	float4 ScatterStep(
			inout float3 accumLight, 
			inout float accumTransmittance, 
			in float3 sliceLight,
			in float sliceDensity, 
			in float tickness,
			in float densityScale) {
		sliceDensity = max(sliceDensity, 0.0000001f);
		sliceDensity *= densityScale;
		const float sliceTransmittance = exp(-sliceDensity * tickness);
	
		// Frostbite에서 사용한 슬라이스 산란광 적분식.
		const float3 sliceLightIntegral = sliceLight * (1.f - sliceTransmittance) / sliceDensity;
	
		accumLight += sliceLightIntegral * accumTransmittance;
		accumTransmittance *= sliceTransmittance;
	
		return float4(accumLight, accumTransmittance);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//
	// @param pos : 투영할 월드 공간 위치
	// @param viewProj : 뷰-투영 행렬
	//
	// @return 텍스처 UV와 투영 깊이
	//
	//-----------------------------------------------------------------------------------------------------------
	float3 ConvertPositionToUV(in float3 pos, in float4x4 viewProj) {
		float4 posH = mul(float4(pos, 1), viewProj);
		posH /= posH.w;
		return float3(posH.xy * float2(0.5f, -0.5f) + (float2)0.5f, posH.z);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//
	// @param v : 텍셀 내부의 보간 위치
	//
	// @return 세제곱 B-스플라인 가중치 네 개
	//
	//-----------------------------------------------------------------------------------------------------------
	float4 Cubic(float v) {
		const float4 n = float4(1.f, 2.f, 3.f, 4.f) - v;
		const float4 s = n * n * n;
		const float x = s.x;
		const float y = s.y - 4.f * s.x;
		const float z = s.z - 4.f * s.y + 6.f * s.x;
		const float w = 6.f - x - y - z;
		return float4(x, y, z, w) * (1.f / 6.f);
	}
	
	//-----------------------------------------------------------------------------------------------------------
	//
	// @param tex : 샘플링할 3D 텍스처
	// @param samp : 텍스처 샘플러
	// @param texCoords : 정규화된 3D 텍스처 좌표
	// @param texSize : 축별 텍스처 크기
	//
	// @return 세제곱 보간으로 샘플링한 값
	//
	//-----------------------------------------------------------------------------------------------------------
	float4 Tex3DTricubic(Texture3D tex, SamplerState samp, float3 texCoords, float3 texSize) {
		const float4 TexelSize = float4(1.f / texSize.xz, texSize.xz);
		
		texCoords = texCoords * texSize - 0.5f;
		
		float3 f = frac(texCoords);
		texCoords -= f;
		
		const float4 xCubic = Cubic(f.x); 
		const float4 yCubic = Cubic(f.y);
		const float4 zCubic = Cubic(f.z);
	   
		const float2 cx = texCoords.xx + float2(-0.5f, 1.5f);
		const float2 cy = texCoords.yy + float2(-0.5f, 1.5f);
		const float2 cz = texCoords.zz + float2(-0.5f, 1.5f);
		const float2 sx = xCubic.xz + xCubic.yw;
		const float2 sy = yCubic.xz + yCubic.yw;
		const float2 sz = zCubic.xz + zCubic.yw;
		
		float2 offsetx = cx + xCubic.yw / sx;
		float2 offsety = cy + yCubic.yw / sy;
		float2 offsetz = cz + zCubic.yw / sz;
		offsetx /= texSize.xx;
		offsety /= texSize.yy;
		offsetz /= texSize.zz;
		
		const float4 sample0 = tex.Sample(samp, float3(offsetx.x, offsety.x, offsetz.x));
		const float4 sample1 = tex.Sample(samp, float3(offsetx.y, offsety.x, offsetz.x));
		const float4 sample2 = tex.Sample(samp, float3(offsetx.x, offsety.y, offsetz.x));	
		const float4 sample3 = tex.Sample(samp, float3(offsetx.y, offsety.y, offsetz.x));
		const float4 sample4 = tex.Sample(samp, float3(offsetx.x, offsety.x, offsetz.y));
		const float4 sample5 = tex.Sample(samp, float3(offsetx.y, offsety.x, offsetz.y));
		const float4 sample6 = tex.Sample(samp, float3(offsetx.x, offsety.y, offsetz.y));
		const float4 sample7 = tex.Sample(samp, float3(offsetx.y, offsety.y, offsetz.y));
		
		const float gx = sx.x / (sx.x + sx.y);
		const float gy = sy.x / (sy.x + sy.y);
		const float gz = sz.x / (sz.x + sz.y);
		
		const float4 x0 = lerp(sample1, sample0, gx);
		const float4 x1 = lerp(sample3, sample2, gx);
		const float4 x2 = lerp(sample5, sample4, gx);
		const float4 x3 = lerp(sample7, sample6, gx);
		const float4 y0 = lerp(x1, x0, gy);
		const float4 y1 = lerp(x3, x2, gy);
		
		return lerp(y1, y0, gz);
	}
}

#endif // __D3D12VOLUMETRICLIGHT_HLSLI__