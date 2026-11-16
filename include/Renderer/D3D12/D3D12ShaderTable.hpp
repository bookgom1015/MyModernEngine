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

// 셰이더 레코드 = {{셰이더 ID}, {루트 인수}}
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

// 셰이더 테이블 = {{셰이더 레코드 1}, {셰이더 레코드 2}, ...}
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

	// 셰이더 레코드를 읽기 쉬운 형식으로 출력합니다.
	void DebugPrint(std::unordered_map<void*, std::wstring>& shaderIdToStringMap);

protected:
	D3D12Device* mpDevice;

	std::uint8_t* mMappedShaderRecords;
	UINT mShaderRecordSize;
	UINT mBufferSize;

	// 디버깅 지원
	std::wstring mName;
	std::vector<ShaderRecord> mShaderRecords;
};

#include "D3D12ShaderTable.inl"