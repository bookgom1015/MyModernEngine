#ifndef __D3D12ENVIRONMENTMANAGER_INL__
#define __D3D12ENVIRONMENTMANAGER_INL__

GpuResource* D3D12EnvironmentManager::GetBrdfLutMap() const noexcept { return mBrdfLutMap.get(); }

D3D12_GPU_DESCRIPTOR_HANDLE D3D12EnvironmentManager::GetBrdfLutMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhBrdfLutMapSrv);
}

D3D12Texture* D3D12EnvironmentManager::GetGlobalDiffuseIrradianceMap() const noexcept { 
	return mpGlobalDiffuseIrradianceTex; 
}

const std::wstring& D3D12EnvironmentManager::GetGlobalDiffuseIrradianceMapPath() const noexcept {
	return mGlobalDiffuseIrradianceTexPath;
}

void D3D12EnvironmentManager::SetGlobalDiffuseIrradianceMap(const std::wstring& key, D3D12Texture* const pTexture) noexcept {
	mpGlobalDiffuseIrradianceTex = pTexture; 
	mGlobalDiffuseIrradianceTexPath = key;
}

D3D12Texture* D3D12EnvironmentManager::GetGlobalSpecularIrradianceMap() const noexcept { 
	return mpGlobalSpecularIrradianceTex; 
}

const std::wstring& D3D12EnvironmentManager::GetGlobalSpecularIrradianceMapPath() const noexcept {
	return mGlobalSpecularIrradianceTexPath;
}

void D3D12EnvironmentManager::SetGlobalSpecularIrradianceMap(const std::wstring& key, D3D12Texture* const pTexture) noexcept { 
	mpGlobalSpecularIrradianceTex = pTexture; 
	mGlobalSpecularIrradianceTexPath = key;
}

size_t D3D12EnvironmentManager::GetReflectionProbeCount() const noexcept {
	return mReflectionProbes.size();
}

#endif // __D3D12ENVIRONMENTMANAGER_INL__