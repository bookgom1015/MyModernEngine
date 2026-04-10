#ifndef __LIGHTMANAGER_INL__
#define __LIGHTMANAGER_INL__

UINT LightManager::GetLightCount() const noexcept {
	return static_cast<UINT>(mLights.size());
}

#endif // __LIGHTMANAGER_INL__