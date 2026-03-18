#ifndef __D3D12FRAMERESOURCE_INL__
#define __D3D12FRAMERESOURCE_INL__

ID3D12CommandAllocator* D3D12FrameResource::CommandAllocator() const {
	return mCmdAllocator.Get();
}

#endif // __D3D12FRAMERESOURCE_INL__