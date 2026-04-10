#pragma once

class CLight;

struct LightData;

class LightManager : public Singleton<LightManager> {
	SINGLETON(LightManager);

public:
	bool Initialize();

public:
	__forceinline UINT GetLightCount() const noexcept;
	const LightData* GetLightData(UINT index) const;
	void GetLightData(std::vector<const LightData*>& outLightData) const;

public:
	void RegisterLight(CLight* light);
	void UnregisterLight(CLight* light);
	bool IsLightRegistered(const CLight* light) const;

private:
	std::vector<CLight*> mLights;
};

#include "LightManager.inl"

#ifndef LIGHT_MANAGER
#define LIGHT_MANAGER LightManager::GetInstance()
#endif // LIGHT_MANAGER