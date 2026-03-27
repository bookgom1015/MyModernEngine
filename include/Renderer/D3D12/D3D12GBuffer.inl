#ifndef __D3D12GBUFFER_INL__
#define __D3D12GBUFFER_INL__

__forceinline GpuResource* D3D12GBuffer::GetAlbedoMap() const noexcept {
	return mResources[GBuffer::Resource::E_Albedo].get();
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetAlbedoMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Albedo]);
}

__forceinline GpuResource* D3D12GBuffer::GetNormalMap() const noexcept {
	return mResources[GBuffer::Resource::E_Normal].get();
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetNormalMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Normal]);
}

__forceinline GpuResource* D3D12GBuffer::GetNormalDepthMap() const noexcept {
	return mResources[GBuffer::Resource::E_NormalDepth].get();
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetNormalDepthMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_NormalDepth]);
}

__forceinline GpuResource* D3D12GBuffer::GetReprojNormalDepthMap() const noexcept {
	return mResources[GBuffer::Resource::E_ReprojNormalDepth].get();
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetReprojNormalDepthMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_ReprojNormalDepth]);
}

__forceinline GpuResource* D3D12GBuffer::GetRMSMap() const noexcept {
	return mResources[GBuffer::Resource::E_RMS].get();
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetRMSMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_RMS]);
}

__forceinline GpuResource* D3D12GBuffer::GetVelocityMap() const noexcept {
	return mResources[GBuffer::Resource::E_Velocity].get();
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetVelocityMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Velocity]);
}

__forceinline GpuResource* D3D12GBuffer::GetPositionMap() const noexcept {
	return mResources[GBuffer::Resource::E_Position].get();
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetPositionMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Position]);
}

#endif // __D3D12GBUFFER_INL__