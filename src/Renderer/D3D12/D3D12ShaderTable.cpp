#include "pch.h"
#include "Renderer/D3D12/D3D12ShaderTable.hpp"

#include "Renderer/D3D12/D3D12Util.hpp"
#include "Renderer/D3D12/D3D12Device.hpp"

GpuUploadBuffer::GpuUploadBuffer() {}

GpuUploadBuffer::~GpuUploadBuffer() {
	if (mResource.Get()) mResource->Unmap(0, nullptr);
}

bool GpuUploadBuffer::Allocate(D3D12Device* const pDevice, UINT bufferSize, LPCWSTR resourceName) {
	CheckReturn(D3D12Util::CreateUploadBuffer(pDevice, bufferSize, IID_PPV_ARGS(&mResource)));
	mResource->SetName(resourceName);

	return true;
}

bool GpuUploadBuffer::MapCpuWriteOnly(std::uint8_t*& pData) {
	// We don't unmap this until the app closes. Keeping buffer mapped for the lifetime of the resource is okay.
	// We do not intend to read from this resource on the CPU.
	const auto range = CD3DX12_RANGE(0, 0);
	CheckHResult(mResource->Map(0, &range, reinterpret_cast<void**>(&pData)));

	return true;
}

ShaderRecord::ShaderRecord(void* const pShaderIdentifier, UINT shaderIdentifierSize)
	: mShaderIdentifier(pShaderIdentifier, shaderIdentifierSize) {}

ShaderRecord::ShaderRecord(
	void* const pShaderIdentifier
	, UINT shaderIdentifierSize
	, void* const pLocalRootArguments
	, UINT localRootArgumentsSize)
	: mShaderIdentifier(pShaderIdentifier, shaderIdentifierSize)
	, mLocalRootArguments(pLocalRootArguments, localRootArgumentsSize) {}

void ShaderRecord::CopyTo(void* const dest) const {
	uint8_t* byteDest = static_cast<uint8_t*>(dest);

	std::memcpy(byteDest, mShaderIdentifier.Ptr, mShaderIdentifier.Size);
	if (mLocalRootArguments.Ptr) std::memcpy(byteDest + mShaderIdentifier.Size, mLocalRootArguments.Ptr, mLocalRootArguments.Size);
}

ShaderRecord::PointerWithSize::PointerWithSize()
	: Ptr(nullptr), Size(0) {}

ShaderRecord::PointerWithSize::PointerWithSize(void* const ptr, UINT size)
	: Ptr(ptr), Size(size) {}

ShaderTable::ShaderTable(
	D3D12Device* const pDevice
	, UINT numShaderRecords
	, UINT shaderRecordSize
	, LPCWSTR resourceName)
	: mpDevice{ pDevice } {
	mName = resourceName != nullptr ? std::wstring(resourceName) : L"";

	mShaderRecordSize = Align(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
	mShaderRecords.reserve(numShaderRecords);

	mBufferSize = numShaderRecords * mShaderRecordSize;
}

bool ShaderTable::Initialze() {
	CheckReturn(Allocate(mpDevice, mBufferSize, mName.length() > 0 ? mName.c_str() : nullptr));
	CheckReturn(MapCpuWriteOnly(mMappedShaderRecords));

	return true;
}

bool ShaderTable::push_back(const ShaderRecord& shaderRecord) {
	CheckReturn(mShaderRecords.size() < mShaderRecords.capacity());

	mShaderRecords.push_back(shaderRecord);

	shaderRecord.CopyTo(mMappedShaderRecords);

	mMappedShaderRecords += mShaderRecordSize;

	return true;
}

void ShaderTable::DebugPrint(std::unordered_map<void*, std::wstring>& shaderIdToStringMap) {
	std::wstringstream wsstream;
	wsstream << L"|--------------------------------------------------------------------" << std::endl;
	wsstream << L"|Shader table - " << mName << L": " << mShaderRecordSize << L" | " << mShaderRecords.size() * mShaderRecordSize << L" bytes" << std::endl;
	for (UINT i = 0; i < mShaderRecords.size(); ++i) {
		wsstream << L"| [" << i << L"]: " << shaderIdToStringMap[mShaderRecords[i].mShaderIdentifier.Ptr] << L", ";
		wsstream << mShaderRecords[i].mShaderIdentifier.Size << L" + " << mShaderRecords[i].mLocalRootArguments.Size << L" bytes" << std::endl;
	}
	wsstream << L"|--------------------------------------------------------------------";
	WLogln(wsstream.str());
}