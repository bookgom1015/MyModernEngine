#ifndef __D3D12DOFSAMPLING_HLSLI__
#define __D3D12DOFSAMPLING_HLSLI__

ConstantBuffer<PassCB> cbPass : register(b1);

Texture2D<Dof::CircleOfConfusionMapFormat> gi_CoCMap : register(t1);
Texture2D<DepthStencilBuffer::DepthBufferFormat> gi_DepthMap : register(t2);

float DofBlurBlend(float radius) {
	return smoothstep(0.75f, 2.f, radius);
}

int2 DofTextureSize() {
	uint width, height;
	gi_CoCMap.GetDimensions(width, height);
	return int2(width, height);
}

float DofViewDepth(int2 pixel) {
	return ShaderUtil::NdcDepthToViewDepth(gi_DepthMap.Load(int3(pixel, 0)), cbPass.Proj);
}

float DofSurfaceWeight(float centerDepth, float sampleDepth, float centerCoC, float sampleCoC) {
	// CoC는 최솟값과 최댓값에서 포화되므로, CoC가 같은 표면은 뷰 공간 깊이로 구분합니다.
	const float depthTolerance = max(0.05f, 0.05f * min(centerDepth, sampleDepth));
	const float depthWeight = 1.f - smoothstep(0.5f, 2.f, abs(centerDepth - sampleDepth) / depthTolerance);
	const float cocWeight = 1.f - smoothstep(0.05f, 0.25f, abs(centerCoC - sampleCoC));
	return depthWeight * cocWeight;
}

#endif // __D3D12DOFSAMPLING_HLSLI__
