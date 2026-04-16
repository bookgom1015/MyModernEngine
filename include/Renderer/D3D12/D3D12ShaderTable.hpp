#pragma once

class D3D12Device;

class GpuUploadBuffer {
protected:
	GpuUploadBuffer();
	virtual ~GpuUploadBuffer();

public:
	__forceinline Microsoft::WRL::ComPtr<ID3D12Resource> GetResource() const;

protected:
	bool Allocate(D3D12Device* const pDevice, UINT bufferSize, LPCWSTR resourceName = nullptr);
	bool MapCpuWriteOnly(std::uint8_t*& pData);

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
};

// Shader record = {{Shader ID}, {RootArguments}}
class ShaderRecord {
public:
	struct PointerWithSize {
		void* Ptr;
		UINT Size;

		PointerWithSize();
		PointerWithSize(void* const ptr, UINT size);
	};

public:
	ShaderRecord(void* const pShaderIdentifier, UINT shaderIdentifierSize);
	ShaderRecord(void* const pShaderIdentifier, UINT shaderIdentifierSize, void* const pLocalRootArguments, UINT localRootArgumentsSize);

public:
	void CopyTo(void* const dest) const;

public:
	PointerWithSize mShaderIdentifier;
	PointerWithSize mLocalRootArguments;
};

// Shader table = {{ ShaderRecord 1}, {ShaderRecord 2}, ...}
class ShaderTable : public GpuUploadBuffer {
public:
	ShaderTable(
		D3D12Device* const pDevice,
		UINT numShaderRecords,
		UINT shaderRecordSize,
		LPCWSTR resourceName = nullptr);

public:
	__forceinline constexpr std::uint8_t* GetMappedShaderRecords() const;
	__forceinline constexpr UINT GetShaderRecordSize() const;

public:
	bool Initialze();
	bool push_back(const ShaderRecord& shaderRecord);

	// Pretty-print the shader records.
	void DebugPrint(std::unordered_map<void*, std::wstring>& shaderIdToStringMap);

protected:
	D3D12Device* mpDevice;

	std::uint8_t* mMappedShaderRecords;
	UINT mShaderRecordSize;
	UINT mBufferSize;

	// Debug support
	std::wstring mName;
	std::vector<ShaderRecord> mShaderRecords;
};

#include "D3D12ShaderTable.inl"