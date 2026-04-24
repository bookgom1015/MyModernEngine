#pragma once

class ShaderArgumentManager : public Singleton<ShaderArgumentManager> {
	SINGLETON(ShaderArgumentManager);

public:
	struct _ToneMapping {
		enum _Type {
			E_None = 0,
			E_ACES,
			E_Exponential,
			E_Reinhard,
			E_ReinhardExt,
			E_Uncharted2,
			E_Log,
			Count
		};

		const char* TypeNames[Count] = {
			"None",
			"ACES",
			"Exponential",
			"Reinhard",
			"ReinhardExt",
			"Uncharted2",
			"Log"
		};

		const int MaxType = Count;
		_Type Type = E_ACES;

		const float MaxMiddleGrayKey = 0.99f;
		const float MinMiddleGrayKey = 0.01f;
		float MiddleGrayKey = 0.3f;
	} ToneMapping;

	struct _GammaCorrection {
		bool Enabled = true;

		const float MaxGamma = 5.f;
		const float MinGamma = 0.1f;
		float Gamma = 2.2f;
	} GammaCorrection;

	struct _TAA {
		bool Enabled = true;

		const float MaxModulationFactor = 0.99f;
		const float MinModulationFactor = 0.01f;
		float ModulationFactor = 0.9f;
	} TAA;

	struct _Bloom {
		bool Enabled = true;

		const float MaxSharpness = 0.99f;
		const float MinSharpness = 0.01f;
		float Sharpness = 0.8f;
	} Bloom;

	struct _Vignette {
		bool Enabled = true;

		const float MaxStrength = 0.99f;
		const float MinStrength = 0.01f;
		float Strength = 0.4f;
	} Vignette;

	struct _VolumetricLight {
		bool Enabled = true;
		float DepthExponent = 4.f;

		const float MinAnisotropicCoefficient = -0.5f;
		const float MaxAnisotropicCoefficient = 0.5f;
		float AnisotropicCoefficient = 0.f;

		const float MaxUniformDensity = 1.f;
		const float MinUniformDensity = 0.f;
		float UniformDensity = 0.1f;

		const float MaxDensityScale = 1.f;
		const float MinDensityScale = 0.f;
		float DensityScale = 0.01f;

		bool TricubicSamplingEnabled = true;
	} VolumetricLight;

	struct _BlendWithCurrentFrame {
		bool UseClamping = true;
		float DepthSigma = 1.f;
		float StdDevGamma = 0.6f;
		float MinStdDevTolerance = 0.05f;

		float ClampDifferenceToTSPPScale = 4.f;
		std::uint32_t MinTSPPToUseTemporalVariance = 4;
		std::uint32_t LowTSPPMaxTSPP = 12;
		float LowTSPPDecayConstant = 1.f;
	};

	struct _AtrousWaveletTransformFilter {
		float ValueSigma = 1.f;
		float NormalSigma = 64.f;
		float DepthSigma = 1.f;
		float DepthWeightCutoff = 0.2f;

		float MinVarianceToDenoise = 0.f;

		bool UseSmoothedVariance = false;

		bool PerspectiveCorrectDepthInterpolation = true;

		bool UseAdaptiveKernelSize = true;
		bool KernelRadiusRotateKernelEnabled = true;
		std::int32_t KernelRadiusRotateKernelNumCycles = 3;
		std::int32_t FilterMinKernelWidth = 3;
		float FilterMaxKernelWidthPercentage = 1.5f;
		float AdaptiveKernelSizeRayHitDistanceScaleFactor = 0.02f;
		float AdaptiveKernelSizeRayHitDistanceScaleExponent = 2.f;
	};

	struct _Denoiser {
		bool DisocclusionBlurEnabled = true;
		bool FullscreenBlurEnabaled = true;
		bool UseSmoothingVariance = true;
		std::uint32_t LowTsppBlurPassCount = 3;
	};

	struct _SSAO {
		const float MinOcclusionRadius = 0.1f;
		const float MaxOcclusionRadius = 10.f;
		float OcclusionRadius = 4.f;

		const float MinOcclusionFadeStart = 0.f;
		float OcclusionFadeStart = 1.f;

		const float MaxOcclusionFadeEnd = 10.f;
		float OcclusionFadeEnd = 8.f;

		const float MinOcclusionStrength = 0.1f;
		const float MaxOcclusionStrength = 4.f;
		float OcclusionStrength = 1.f;

		const float MinSurfaceEpsilon = 0.001f;
		const float MaxSurfaceEpsilon = 0.1f;
		float SurfaceEpsilon = 0.05f;

		const std::uint32_t MaxSampleCount = 14;
		const std::uint32_t MinSampleCount = 1;
		std::uint32_t SampleCount = 6;

		_BlendWithCurrentFrame BlendWithCurrentFrame;
		_AtrousWaveletTransformFilter AtrousWaveletTransformFilter;
		_Denoiser Denoiser;
	} SSAO;

	struct _RTAO {
		float OcclusionRadius = 10.f;
		float OcclusionFadeStart = 0.f;
		float OcclusionFadeEnd = 10.f;
		float SurfaceEpsilon = 0.0001f;

		std::uint32_t SampleCount = 1;
		const std::uint32_t MaxSampleCount = 128;
		const std::uint32_t MinSampleCount = 1;

		std::uint32_t SampleSetSize = 8;
		const std::uint32_t MaxSampleSetSize = 8;
		const std::uint32_t MinSampleSetSize = 1;

		// RaySorting
		bool RaySortingEnabled = true;
		bool CheckerboardGenerateRaysForEvenPixels = false;
		bool CheckboardRayGeneration = true;
		bool RandomFrameSeed = true;

		//
		std::uint32_t MaxTSPP = 33;

		_BlendWithCurrentFrame BlendWithCurrentFrame;
		_AtrousWaveletTransformFilter AtrousWaveletTransformFilter;
		_Denoiser Denoiser;
	} RTAO;

	struct _MotionBlur {
		bool Enabled = true;

		float Intensity = 0.01f;
		float Limit = 0.005f;
		float DepthBias = 0.05f;

		const std::uint32_t MaxSampleCount = 32;
		const std::uint32_t MinSampleCount = 8;
		std::uint32_t SampleCount = 16;
	} MotionBlur;


	bool RaytracingEnabled = false;
	bool AOEnabled = true;
};

#ifndef SHADER_ARGUMENT_MANAGER
#define SHADER_ARGUMENT_MANAGER ShaderArgumentManager::GetInstance()
#endif // SHADER_ARGUMENT_MANAGER