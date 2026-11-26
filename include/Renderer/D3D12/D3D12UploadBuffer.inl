#ifndef __D3D12UploadBuffer_INL__
#define __D3D12UploadBuffer_INL__

template <typename T>
UploadBuffer<T>::UploadBuffer()
	: mUploadBuffer{}
	, mMappedData{}
	, mElementByteSize{}
	, mbIsConstantBuffer{}
	, mbIsDirty{ true } {}

template <typename T>
UploadBuffer<T>::~UploadBuffer() {
	if (mUploadBuffer) {
		mUploadBuffer->Unmap(0, nullptr);
		mUploadBuffer.Reset();
	}
}

template <typename T>
bool UploadBuffer<T>::Initialize(
	D3D12Device* const pDevice
	, UINT elementCount
	, UINT instanceCount
	, bool isConstantBuffer
	, LPCWSTR name) {
	mbIsConstantBuffer = isConstantBuffer;
	mElementByteSize = sizeof(T);

	// 상수 버퍼 원소의 크기는 256바이트의 배수여야 합니다.
	// 하드웨어가 상수 데이터에 접근할 때
	// 바이트 오프셋과 크기를 모두 256바이트의 배수로 지정해야 하기 때문입니다.
	// typedef struct D3D12_CONSTANT_BUFFER_VIEW_DESC {
	// UINT64 OffsetInBytes; // 256의 배수
	// UINT   SizeInBytes;   // 256의 배수
	// } D3D12_CONSTANT_BUFFER_VIEW_DESC;
	if (isConstantBuffer)
		mElementByteSize = D3D12Util::CalcConstantBufferByteSize(sizeof(T));

	CheckReturn(D3D12Util::CreateUploadBuffer(
		pDevice, mElementByteSize * elementCount * instanceCount, IID_PPV_ARGS(&mUploadBuffer)));

	if (name != nullptr) CheckHResult(mUploadBuffer->SetName(name));

	if (FAILED(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData))))
		ReturnFalse("Failed mapping upload buffer");

	// 리소스 사용이 끝날 때까지 매핑을 유지해도 됩니다.
	// GPU가 리소스를 사용하는 동안에는 쓰지 않도록 동기화해야 합니다.
	return true;
}

template <typename T>
ID3D12Resource* UploadBuffer<T>::Resource() const { return mUploadBuffer.Get(); }

template <typename T>
D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer<T>::CBAddress() const {
	return mUploadBuffer->GetGPUVirtualAddress();
}

template <typename T>
D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer<T>::CBAddress(UINT index) const {
	return mUploadBuffer->GetGPUVirtualAddress() +
		static_cast<UINT64>(index) * static_cast<UINT64>(CBByteSize());
}

template <typename T>
constexpr UINT UploadBuffer<T>::CBByteSize() const noexcept {
	return (sizeof(T) + 255) & ~255;
}

template <typename T>
void UploadBuffer<T>::CopyCB(const T& data, INT elementIndex) {	
	CopyData(elementIndex, data);
}

template <typename T>
void UploadBuffer<T>::CopyData(INT elementIndex, const T& data) {
	std::memcpy(&mMappedData[elementIndex * mElementByteSize], &data, sizeof(T));
}

#endif // __D3D12UploadBuffer_INL__