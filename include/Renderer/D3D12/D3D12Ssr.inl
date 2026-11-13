#ifndef __D3D12_SSR_INL__
#define __D3D12_SSR_INL__

GpuResource* D3D12Ssr::GetResource(Ssr::Resource::Type type) const noexcept {
	return mResources[type].get();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12Ssr::GetSrv(Ssr::Descriptor::Srv::Type type) const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[type]);
}

#endif // __D3D12_SSR_INL__