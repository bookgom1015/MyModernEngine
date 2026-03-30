#ifndef __D3D12COMMANDOBJECT_INL__
#define __D3D12COMMANDOBJECT_INL__

ID3D12CommandQueue* D3D12CommandObject::GetCommandQueue() const {
	return mCommandQueue.Get();
}

ID3D12GraphicsCommandList6* D3D12CommandObject::GetDirectCommandList() const {
	return mDirectCommandList.Get();
}

ID3D12GraphicsCommandList6* D3D12CommandObject::GetUploadCommandList() const {
	return mUploadCommandList.Get();
}

#endif // __D3D12COMMANDOBJECT_INL__