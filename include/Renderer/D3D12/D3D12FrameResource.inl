#ifndef __D3D12FRAMERESOURCE_INL__
#define __D3D12FRAMERESOURCE_INL__

ID3D12CommandAllocator* D3D12FrameResource::FrameCommandAllocator() const {
	return mFrameCmdAllocator.Get();
}

ID3D12CommandAllocator* D3D12FrameResource::UploadCommandAllocator() const {
	return mUploadCmdAllocator.Get();
}

ID3D12CommandAllocator* D3D12FrameResource::ImmediateCommandAllocator() const {
	return mImmediateCmdAllocator.Get();
}

#endif // __D3D12FRAMERESOURCE_INL__