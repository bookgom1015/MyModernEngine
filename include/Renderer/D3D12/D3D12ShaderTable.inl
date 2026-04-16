#ifndef __D3D12SHADERTABLE_INL__
#define __D3D12SHADERTABLE_INL__

Microsoft::WRL::ComPtr<ID3D12Resource> GpuUploadBuffer::GetResource() const { return mResource; }

constexpr std::uint8_t* ShaderTable::GetMappedShaderRecords() const { return mMappedShaderRecords; }

constexpr UINT ShaderTable::GetShaderRecordSize() const { return mShaderRecordSize; }

#endif // __D3D12SHADERTABLE_INL__