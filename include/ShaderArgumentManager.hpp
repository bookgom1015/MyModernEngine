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
};

#ifndef SHADER_ARGUMENT_MANAGER
#define SHADER_ARGUMENT_MANAGER ShaderArgumentManager::GetInstance()
#endif // SHADER_ARGUMENT_MANAGER