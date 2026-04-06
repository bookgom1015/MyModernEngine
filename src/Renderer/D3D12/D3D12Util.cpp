#include "pch.h"
#include "Renderer/D3D12/D3D12Util.hpp"

#include "Vertex.h"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"
#include "Renderer/D3D12/D3D12Texture.hpp"
#include "Renderer/D3D12/D3D12GpuResource.hpp"

#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>

using namespace Microsoft::WRL;

using namespace DirectX;

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo() {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, D3D12_RESOURCE_FLAGS flags) : Size(size), Flags(flags) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state) : Size(size), HeapType(heapType), State(state) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state) : Size(size), Flags(flags), State(state) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, UINT64 alignment, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state)
	: Size(size), Alignment(alignment), HeapType(heapType), Flags(flags), State(state) {}

D3D12Util::D3D12BufferCreateInfo::D3D12BufferCreateInfo(UINT64 size, UINT64 alignment, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state)
	: Size(size), Alignment(alignment), HeapType(heapType), HeapFlags(heapFlags), Flags(flags), State(state) {}

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
			CheckHResult(pDevice->md3dDevice->CreateCommittedResource(
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
			CheckHResult(pDevice->md3dDevice->CreateCommittedResource(
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
		CheckHResult(pDevice->md3dDevice->CreateCommittedResource(
			&heapDesc, info.HeapFlags, &resourceDesc, info.State, nullptr, riid, ppResource));
	}
	else {
		CheckHResult(pDevice->md3dDevice->CreateCommittedResource(
			&heapDesc, info.HeapFlags, &resourceDesc, info.State, nullptr, riid, ppResource));
	}

	return true;
}

bool D3D12Util::CreateRootSignature(
	D3D12Device* const pDevice
	, const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc
	, const IID& riid
	, void** const ppRootSignature
	, LPCWSTR name) {
	ComPtr<ID3DBlob> serializedRootSig{};
	ComPtr<ID3DBlob> errorBlob{};

	HRESULT hr = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf());

	std::wstringstream wsstream{};
	if (errorBlob != nullptr)
		wsstream << reinterpret_cast<char*>(errorBlob->GetBufferPointer());

	if (FAILED(hr)) ReturnFalse(WStrToStr(wsstream.str()));

	CheckHResult(pDevice->md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		riid,
		ppRootSignature));

	if (name != nullptr) {
		auto rootSig = reinterpret_cast<ID3D12RootSignature*>(*ppRootSignature);
		rootSig->SetName(name);
	}

	return true;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12Util::DefaultPsoDesc(
	D3D12_INPUT_LAYOUT_DESC inputLayout
	, DXGI_FORMAT dsvFormat) {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = inputLayout;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = dsvFormat;

	return psoDesc;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12Util::FitToScreenPsoDesc() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { nullptr, 0 };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	return psoDesc;
}

D3DX12_MESH_SHADER_PIPELINE_STATE_DESC D3D12Util::DefaultMeshPsoDesc(DXGI_FORMAT dsvFormat) {
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.DSVFormat = dsvFormat;

	return psoDesc;
}

D3DX12_MESH_SHADER_PIPELINE_STATE_DESC D3D12Util::FitToScreenMeshPsoDesc() {
	D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	return psoDesc;
}

bool D3D12Util::CreateComputePipelineState(
	D3D12Device* const pDevice
	, const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc
	, const IID& riid
	, void** const ppPipelineState
	, LPCWSTR name) {
	CheckHResult(pDevice->md3dDevice->CreateComputePipelineState(&desc, riid, ppPipelineState));

	if (name != nullptr) {
		auto pso = reinterpret_cast<ID3D12PipelineState*>(*ppPipelineState);
		pso->SetName(name);
	}

	return true;
}

bool D3D12Util::CreateGraphicsPipelineState(
	D3D12Device* const pDevice
	, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc
	, const IID& riid
	, void** const ppPipelineState
	, LPCWSTR name) {
	CheckHResult(pDevice->md3dDevice->CreateGraphicsPipelineState(&desc, riid, ppPipelineState));

	if (name != nullptr) {
		auto pso = reinterpret_cast<ID3D12PipelineState*>(*ppPipelineState);
		pso->SetName(name);
	}

	return true;
}

bool D3D12Util::CreatePipelineState(
	D3D12Device* const pDevice
	, const D3DX12_MESH_SHADER_PIPELINE_STATE_DESC& desc
	, const IID& riid
	, void** const ppPipelineState
	, LPCWSTR name) {
	auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(desc);

	D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
	streamDesc.SizeInBytes = sizeof(meshStreamDesc);
	streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

	CheckHResult(pDevice->md3dDevice->CreatePipelineState(&streamDesc, riid, ppPipelineState));

	if (name != nullptr) {
		auto pso = reinterpret_cast<ID3D12PipelineState*>(*ppPipelineState);
		pso->SetName(name);
	}

	return true;
}

bool D3D12Util::CreateStateObject(
	D3D12Device* const pDevice
	, const D3D12_STATE_OBJECT_DESC* pDesc
	, const IID& riid
	, void** const ppStateObject) {
	CheckHResult(pDevice->md3dDevice->CreateStateObject(pDesc, riid, ppStateObject));
	return true;
}

namespace {
	const D3D12_INPUT_ELEMENT_DESC gStaticVertexInputLayout[] = {
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, offsetof(Vertex, Position),	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, offsetof(Vertex, Normal),	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, offsetof(Vertex, Tangent),	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,		0, offsetof(Vertex, TexCoord),	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	const D3D12_INPUT_LAYOUT_DESC gStaticVertexInputLayoutDesc = { gStaticVertexInputLayout, static_cast<UINT>(_countof(gStaticVertexInputLayout)) };

	const D3D12_INPUT_ELEMENT_DESC gSkinnedVertexInputLayout[] = {
		{ "POSITION",		0, DXGI_FORMAT_R32G32B32_FLOAT,		0, offsetof(SkinnedVertex, Position),		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",			0, DXGI_FORMAT_R32G32B32_FLOAT,		0, offsetof(SkinnedVertex, Normal),			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",		0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, offsetof(SkinnedVertex, Tangent),		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",		0, DXGI_FORMAT_R32G32_FLOAT,		0, offsetof(SkinnedVertex, TexCoord),		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES",	0, DXGI_FORMAT_R32G32B32A32_UINT,	0, offsetof(SkinnedVertex, JointIndices),	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHTS",	0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, offsetof(SkinnedVertex, JointWeights),	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	const D3D12_INPUT_LAYOUT_DESC gSkinnedVertexInputLayoutDesc = { gSkinnedVertexInputLayout, static_cast<UINT>(_countof(gSkinnedVertexInputLayout)) };
}

D3D12_INPUT_LAYOUT_DESC D3D12Util::StaticVertexInputLayoutDesc() { return gStaticVertexInputLayoutDesc; }

D3D12_INPUT_LAYOUT_DESC D3D12Util::SkinnedVertexInputLayoutDesc() { return gSkinnedVertexInputLayoutDesc; }

namespace {
	const CD3DX12_STATIC_SAMPLER_DESC gPointWrap{
		0,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP		// addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC gPointClamp{
		1,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP	// addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC gLinearWrap{
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP		// addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC gLinearClamp{
		3,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP	// addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC gAnisotropicWrap{
		4,									// shaderRegister
		D3D12_FILTER_ANISOTROPIC,			// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,	// addressW
		0.f,								// mipLODBias
		8									// maxAnisotropy
	};

	const CD3DX12_STATIC_SAMPLER_DESC gAnisotropicClamp{
		5,									// shaderRegister
		D3D12_FILTER_ANISOTROPIC,			// filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,	// addressW
		0.f,								// mipLODBias
		8									// maxAnisotropy
	};

	const CD3DX12_STATIC_SAMPLER_DESC gAnisotropicBorder{
		6,									// shaderRegister
		D3D12_FILTER_ANISOTROPIC,			// filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressW
		0.f,								// mipLODBias
		8,									// maxAnisotropy
		D3D12_COMPARISON_FUNC_ALWAYS,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK
	};

	const CD3DX12_STATIC_SAMPLER_DESC gDepthMap{
		7,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,	// addressW
		0.f,								// mipLODBias
		0,									// maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE
	};

	const CD3DX12_STATIC_SAMPLER_DESC gShadow{
		8,													// shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,					// addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,					// addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,					// addressW
		0.f,												// mipLODBias
		16,													// maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE
	};

	const CD3DX12_STATIC_SAMPLER_DESC gPointMirror{
		9,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT,		// filter
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR	// addressW
	};

	const CD3DX12_STATIC_SAMPLER_DESC gLinearMirror{
		10,									// shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,	// addressU
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,	// addressV
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR	// addressW
	};
}

D3D12Util::StaticSamplers D3D12Util::msStaticSamplers = {
		gPointWrap,
		gPointClamp,
		gLinearWrap,
		gLinearClamp,
		gAnisotropicWrap,
		gAnisotropicClamp,
		gAnisotropicBorder,
		gDepthMap,
		gShadow,
		gPointMirror,
		gLinearMirror };

void D3D12Util::CreateShaderResourceView(
	D3D12Device* const pDevice,
	ID3D12Resource* const pResource,
	const D3D12_SHADER_RESOURCE_VIEW_DESC* const pDesc,
	D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateShaderResourceView(pResource, pDesc, destDescriptor);
}

void D3D12Util::CreateUnorderedAccessView(
	D3D12Device* const pDevice,
	ID3D12Resource* const pResource,
	ID3D12Resource* const pCounterResource,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC* const pDesc,
	D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateUnorderedAccessView(pResource, pCounterResource, pDesc, destDescriptor);
}

void D3D12Util::CreateRenderTargetView(
	D3D12Device* const pDevice,
	ID3D12Resource* const pResource,
	const D3D12_RENDER_TARGET_VIEW_DESC* const pDesc,
	D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateRenderTargetView(pResource, pDesc, destDescriptor);
}

void D3D12Util::CreateDepthStencilView(
	D3D12Device* const pDevice,
	ID3D12Resource* const pResource,
	const D3D12_DEPTH_STENCIL_VIEW_DESC* const pDesc,
	D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) {
	pDevice->md3dDevice->CreateDepthStencilView(pResource, pDesc, destDescriptor);
}

const D3D12_STATIC_SAMPLER_DESC* D3D12Util::GetStaticSamplers() noexcept {
	return msStaticSamplers.data();
}

void D3D12Util::UavBarrier(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResource) {
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pResource;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	pCmdList->ResourceBarrier(1, &uavBarrier);
}

void D3D12Util::UavBarriers(ID3D12GraphicsCommandList* const pCmdList, ID3D12Resource* pResources[], UINT length) {
	std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;
	for (UINT i = 0; i < length; ++i) {
		D3D12_RESOURCE_BARRIER uavBarrier;
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = pResources[i];
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		uavBarriers.push_back(uavBarrier);
	}
	pCmdList->ResourceBarrier(static_cast<UINT>(uavBarriers.size()), uavBarriers.data());
}

void D3D12Util::UavBarrier(ID3D12GraphicsCommandList* const pCmdList, GpuResource* pResource) {
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pResource->Resource();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	pCmdList->ResourceBarrier(1, &uavBarrier);
}

void D3D12Util::UavBarriers(ID3D12GraphicsCommandList* const pCmdList, GpuResource* pResources[], UINT length) {
	std::vector<D3D12_RESOURCE_BARRIER> uavBarriers;
	for (UINT i = 0; i < length; ++i) {
		D3D12_RESOURCE_BARRIER uavBarrier;
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = pResources[i]->Resource();
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		uavBarriers.push_back(uavBarrier);
	}
	pCmdList->ResourceBarrier(static_cast<UINT>(uavBarriers.size()), uavBarriers.data());
}

bool D3D12Util::CreateTexture(
	D3D12Device* const pDevice
	, D3D12CommandObject* const pCmdObject
	, D3D12Texture* const pTexture
	, LPCWSTR filePath
	, BOOL bGenerateMipmapIfMissing
	, UINT maxSize) {
	ResourceUploadBatch resourceUpload(pDevice->md3dDevice.Get());
	resourceUpload.Begin();

	const HRESULT status = DirectX::CreateDDSTextureFromFile(
		pDevice->md3dDevice.Get(),
		resourceUpload,
		filePath,
		pTexture->Resource.ReleaseAndGetAddressOf(),
		bGenerateMipmapIfMissing,
		maxSize);

	auto finished = resourceUpload.End(pCmdObject->GetCommandQueue());
	finished.wait();

	if (FAILED(status)) 
		ReturnFalseFormat(
			"Returned 0x{:X}; when creating texture: {}", status, WStrToStr(filePath));

	return true;
}