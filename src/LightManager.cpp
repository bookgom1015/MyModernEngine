#include "pch.h"
#include "LightManager.hpp"

#include "LightData.h"

#include "CLight.hpp"

LightManager::LightManager() {}

LightManager::~LightManager() {}

bool LightManager::Initialize() {
	mLights.clear();

	return true;
}

const LightData* LightManager::GetLightData(UINT index) const {
	if (index >= mLights.size()) return nullptr;
	return &mLights[index]->GetData();
}

void LightManager::GetLightData(std::vector<const LightData*>& outLightData) const {
	outLightData.clear();
	outLightData.reserve(mLights.size());

	for (const auto& light : mLights) 
		outLightData.push_back(&light->GetData());
}

void LightManager::RegisterLight(CLight* light) {
	mLights.push_back(light);
}

void LightManager::UnregisterLight(CLight* light) {
	if (light == nullptr) return;

	std::erase(mLights, light);
}

bool LightManager::IsLightRegistered(const CLight* light) const {
	return std::find(mLights.begin(), mLights.end(), light) != mLights.end();
}