#ifndef __SWAPCHAIN_INL__
#define __SWAPCHAIN_INL__

const D3D12_VIEWPORT& D3D12SwapChain::GetScreenViewport() const noexcept { return mScreenViewport; }

const D3D12_RECT& D3D12SwapChain::GetScissorRect() const noexcept { return mScissorRect; }

GpuResource* D3D12SwapChain::GetCurrentBackBuffer() const {
	return mSwapChainBuffers[mCurrBackBuffer].get();
}

#endif // __SWAPCHAIN_INL__