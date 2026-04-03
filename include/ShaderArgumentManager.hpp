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
};

#ifndef SHADER_ARGUMENT_MANAGER
#define SHADER_ARGUMENT_MANAGER ShaderArgumentManager::GetInstance()
#endif // SHADER_ARGUMENT_MANAGER