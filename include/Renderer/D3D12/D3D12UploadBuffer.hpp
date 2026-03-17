#pragma once

#include "D3D12Util.hpp"

struct LogFile;

class D3D12Device;


template<typename T>
class UploadBuffer {
public:
	UploadBuffer();
	virtual ~UploadBuffer();

public:
	bool Initialize(
		LogFile* const pLogFile,
		D3D12Device* const pDevice,
		UINT elementCount,
		UINT instanceCount,
		bool isConstantBuffer,
		LPCWSTR name = nullptr);

public:
	ID3D12Resource* Resource() const;

	D3D12_GPU_VIRTUAL_ADDRESS CBAddress() const;
	D3D12_GPU_VIRTUAL_ADDRESS CBAddress(UINT index) const;
	constexpr UINT CBByteSize() const noexcept;

	void CopyCB(const T& data, INT elementIndex = 0);
	void CopyData(INT elementIndex, const T& data);

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
	BYTE* mMappedData;

	UINT mElementByteSize;
	bool mbIsConstantBuffer;

	bool mbIsDirty;
};

#include "D3D12UploadBuffer.inl"