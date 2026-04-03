#ifndef __D3D12BLURFILTER_INL__
#define __D3D12BLURFILTER_INL__

int BlurFilter::CalcDiameter(float sigma) {
	const int BlurRadius = static_cast<int>(ceil(2.f * sigma));
	return 2 * BlurRadius + 1;
}

bool BlurFilter::CalcGaussWeights(float sigma, float weights[]) {
	float twoSigma2 = 2.f * sigma * sigma;

	// Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
	const int BlurRadius = static_cast<int>(ceil(2.f * sigma));
	const int Size = 2 * BlurRadius + 1;
	float weightSum = 0.f;

	for (int i = -BlurRadius; i <= BlurRadius; ++i) {
		float x = static_cast<float>(i);
		weights[i + BlurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + BlurRadius];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (int i = 0; i < Size; ++i)
		weights[i] /= weightSum;

	return true;
}

#endif // __D3D12BLURFILTER_INL__