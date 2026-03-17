#include "Renderer/D3D12/pch_d3d12.h"
#include "Renderer/D3D12/D3D12Util.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"

using namespace Microsoft::WRL;

LogFile* D3D12Util::mpLogFile = nullptr;

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo() {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, D3D12_RESOURCE_FLAGS flags) : Size(size), Flags(flags) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state) : Size(size), HeapType(heapType), State(state) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state) : Size(size), Flags(flags), State(state) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, UINT64 alignment, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state)
	: Size(size), Alignment(alignment), HeapType(heapType), Flags(flags), State(state) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, UINT64 alignment, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state)
	: Size(size), Alignment(alignment), HeapType(heapType), HeapFlags(heapFlags), Flags(flags), State(state) {}

bool D3D12Util::Initialize(LogFile* const pLogFile) {
	mpLogFile = pLogFile;

	return true;
}

UINT D3D12Util::CalcConstantBufferByteSize(UINT byteSize) {
	// Constant buffers must be a multiple of the minimum hardware
	// allocation size (usually 256 bytes).  So round up to nearest
	// multiple of 256.  We do this by adding 255 and then masking off
	// the lower 2 bytes which store all bits < 256.
	// Example: Suppose byteSize = 300.
	// (300 + 255) & ~255
	// 555 & ~255
	// 0x022B & ~0x00ff
	// 0x022B & 0xff00
	// 0x0200
	// 512
	return (byteSize + 255) & ~255;
}

bool D3D12Util::CreateDefaultBuffer(
	D3D12Device* const pDevice
	, ID3D12GraphicsCommandList4* const cmdList
	, const void* const pInitData
	, UINT64 byteSize
	, ComPtr<ID3D12Resource>& uploadBuffer
	, ComPtr<ID3D12Resource>& defaultBuffer) {
	// Create the actual default buffer resource.
		{
			auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
			CheckHResult(mpLogFile, pDevice->md3dDevice->CreateCommittedResource(
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(defaultBuffer.GetAddressOf())));
		}

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap. 
		{
			auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
			CheckHResult(mpLogFile, pDevice->md3dDevice->CreateCommittedResource(
				&prop,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(uploadBuffer.GetAddressOf())));
		}

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData{};
	subResourceData.pData = pInitData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	{
		auto transit = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		cmdList->ResourceBarrier(1, &transit);
	}

	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	{
		auto transit = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		cmdList->ResourceBarrier(1, &transit);
	}

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.

	return true;
}

bool D3D12Util::CreateUploadBuffer(
	D3D12Device* const pDevice
	, UINT64 byteSize
	, const IID& riid
	, void** const ppResource) {
	auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	if (FAILED(pDevice->md3dDevice->CreateCommittedResource(
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		riid,
		ppResource)))
		return false;
	return true;
}

bool D3D12Util::CreateBuffer(
	D3D12Device* const pDevice
	, D3D12BufferCreateInfo& info
	, const IID& riid
	, void** const ppResource
	, ID3D12InfoQueue* pInfoQueue) {
	D3D12_HEAP_PROPERTIES heapDesc{};
	heapDesc.Type = info.HeapType;
	heapDesc.CreationNodeMask = 1;
	heapDesc.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = info.Alignment;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Width = info.Size;
	resourceDesc.Flags = info.Flags;

	if (pInfoQueue != nullptr) {
		CheckHResult(mpLogFile, pDevice->md3dDevice->CreateCommittedResource(
			&heapDesc, info.HeapFlags, &resourceDesc, info.State, nullptr, riid, ppResource));
	}
	else {
		CheckHResult(mpLogFile, pDevice->md3dDevice->CreateCommittedResource(
			&heapDesc, info.HeapFlags, &resourceDesc, info.State, nullptr, riid, ppResource));
	}

	return TRUE;
}
