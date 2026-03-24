#include "pch.h"
#include "Renderer/D3D12/D3D12DepthStencilBuffer.hpp"

#include "Renderer/D3D12/D3D12Device.hpp"
#include "Renderer/D3D12/D3D12CommandObject.hpp"

D3D12DepthStencilBuffer::D3D12DepthStencilBuffer() 
	: mInitData{}
	, mDepthStencilBuffer{}
	, mhDepthStencilBufferSrv{}
	, mhDepthStencilBufferDsv{} {}

D3D12DepthStencilBuffer::~D3D12DepthStencilBuffer() {}

bool D3D12DepthStencilBuffer::Initialize(
	D3D12DescriptorHeap* const pDescHeap
	, void* const pData) {
	CheckReturn(D3D12RenderPass::Initialize(pDescHeap, pData));

	mInitData = *reinterpret_cast<InitData*>(pData);

	mDepthStencilBuffer = std::make_unique<GpuResource>();

	CheckReturn(BuildResources());

	return true;
}

bool D3D12DepthStencilBuffer::AllocateDescriptors() {
	CheckReturn(mpDescHeap->AllocateCbvSrvUav(1, mhDepthStencilBufferSrv));
	CheckReturn(mpDescHeap->AllocateDsv(1, mhDepthStencilBufferDsv));

	CheckReturn(BuildDescriptors());

	return true;
}

bool D3D12DepthStencilBuffer::OnResize(unsigned width, unsigned height) {
	mInitData.Width = width;
	mInitData.Height = height;

	CheckReturn(BuildResources());
	CheckReturn(BuildDescriptors());

	return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DepthStencilBuffer::GetDepthStencilBufferDsv() const {
	return mpDescHeap->GetCpuHandle(mhDepthStencilBufferDsv);
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DepthStencilBuffer::GetDepthStencilBufferSrv() const {
	return mpDescHeap->GetGpuHandle(mhDepthStencilBufferSrv);
}

bool D3D12DepthStencilBuffer::BuildResources() {
	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc{};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mInitData.Width;
	depthStencilDesc.Height = mInitData.Height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DepthStencilBuffer::DepthStencilBufferFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear{};
	optClear.Format = DepthStencilBuffer::DepthStencilBufferFormat;
	optClear.DepthStencil.Depth = DepthStencilBuffer::InvalidDepthValue;
	optClear.DepthStencil.Stencil = DepthStencilBuffer::InvalidStencilValue;

	auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CheckReturn(mDepthStencilBuffer->Initialize(
		mInitData.Device,
		&prop,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_DEPTH_READ,
		&optClear,
		L"DepthStencilBuffer"
	));

	return true;
}

bool D3D12DepthStencilBuffer::BuildDescriptors() {
	// Srv
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = DepthStencilBuffer::DepthBufferFormat;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
		srvDesc.Texture2D.MipLevels = 1;

		mInitData.Device->CreateShaderResourceView(
			mDepthStencilBuffer->Resource(), 
			&srvDesc, 
			mpDescHeap->GetCpuHandle(mhDepthStencilBufferSrv));
	}
	// Dsv
	{
		// Create descriptor to mip level 0 of entire resource using the format of the resource.
		mInitData.Device->CreateDepthStencilView(
			mDepthStencilBuffer->Resource(), 
			nullptr, 
			mpDescHeap->GetCpuHandle(mhDepthStencilBufferDsv));
	}

	return true;
}
