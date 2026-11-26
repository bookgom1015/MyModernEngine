#ifndef __D3D12DEPTHSTENCILBUFFER_INL__
#define __D3D12DEPTHSTENCILBUFFER_INL__

GpuResource* D3D12DepthStencilBuffer::GetDepthStencilBuffer() const noexcept {
	return mDepthStencilBuffer.get(); 
}

#endif // __D3D12DEPTHSTENCILBUFFER_INL__