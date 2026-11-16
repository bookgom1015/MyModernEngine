#ifndef __D3D12BLURFILTER_INL__
#define __D3D12BLURFILTER_INL__

int BlurFilter::CalcDiameter(float sigma) {
	const int BlurRadius = static_cast<int>(ceil(2.f * sigma));
	return 2 * BlurRadius + 1;
}

bool BlurFilter::CalcGaussWeights(float sigma, float weights[]) {
	float twoSigma2 = 2.f * sigma * sigma;

	// 시그마가 종 모양 곡선의 폭을 결정하므로 시그마를 기준으로 블러 반경을 추정합니다.
	const int BlurRadius = static_cast<int>(ceil(2.f * sigma));
	const int Size = 2 * BlurRadius + 1;
	float weightSum = 0.f;

	for (int i = -BlurRadius; i <= BlurRadius; ++i) {
		float x = static_cast<float>(i);
		weights[i + BlurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + BlurRadius];
	}

	// 각 가중치를 총합으로 나누어 가중치의 합을 1.0으로 맞춥니다.
	for (int i = 0; i < Size; ++i)
		weights[i] /= weightSum;

	return true;
}

#endif // __D3D12BLURFILTER_INL__