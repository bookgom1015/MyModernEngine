#ifndef __SWAPCHAIN_INL__
#define __SWAPCHAIN_INL__

const D3D12_VIEWPORT& D3D12SwapChain::GetScreenViewport() const noexcept { return mScreenViewport; }

const D3D12_RECT& D3D12SwapChain::GetScissorRect() const noexcept { return mScissorRect; }

GpuResource* D3D12SwapChain::GetCurrentBackBuffer() const { 
	return mSwapChainBuffers[mCurrBackBuffer].get();
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetCurrentBackBufferSrv() const {
	return mpDescHeap->GetGpuHandle(mhBackBufferSrvs[mCurrBackBuffer]);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetCurrentBackBufferRtv() const {
	return mpDescHeap->GetCpuHandle(mhBackBufferRtvs[mCurrBackBuffer]);
}

GpuResource* D3D12SwapChain::GetHdrMap() const { return mHdrMap.get(); }

D3D12_GPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetHdrMapSrv() const {
	return mpDescHeap->GetGpuHandle(mhSceneMapCopySrv);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetHdrMapRtv() const {
	return mpDescHeap->GetCpuHandle(mhSceneMapCopySrv);
}

GpuResource* D3D12SwapChain::GetHdrMapCopy() const { return mHdrMapCopy.get(); }

D3D12_GPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetHdrMapCopySrv() const {
	return mpDescHeap->GetGpuHandle(mhHdrMapCopySrv);
}

GpuResource* D3D12SwapChain::GetSceneMap() const {	return mSceneMap.get(); }

D3D12_GPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetSceneMapSrv() const {
	return mpDescHeap->GetGpuHandle(mhSceneMapSrv);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetSceneMapRtv() const {
	return mpDescHeap->GetCpuHandle(mhSceneMapRtv);
}

GpuResource* D3D12SwapChain::GetSceneMapCopy() const { return mSceneMapCopy.get(); }

D3D12_GPU_DESCRIPTOR_HANDLE D3D12SwapChain::GetSceneMapCopySrv() const {
	return mpDescHeap->GetGpuHandle(mhSceneMapCopySrv);
}

#endif // __SWAPCHAIN_INL__