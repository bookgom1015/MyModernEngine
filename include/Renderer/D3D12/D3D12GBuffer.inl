#ifndef __D3D12GBUFFER_INL__
#define __D3D12GBUFFER_INL__

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetAlbedoMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Albedo]);
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetNormalMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Normal]);
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetNormalDepthMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_NormalDepth]);
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetReprojNormalDepthMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_ReprojNormalDepth]);
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetSpecularMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Specular]);
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetRoughnessMetalnessMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_RoughnessMetalness]);
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetVelocityMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Velocity]);
}

__forceinline D3D12_GPU_DESCRIPTOR_HANDLE D3D12GBuffer::GetPositionMapSrv() const noexcept {
	return mpDescHeap->GetGpuHandle(mhSrvs[GBuffer::Descriptor::Srv::E_Position]);
}

#endif // __D3D12GBUFFER_INL__