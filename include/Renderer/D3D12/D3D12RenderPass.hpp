#pragma once

#include "D3D12HlslCompaction.h"
#include "D3D12DescriptorHeap.hpp"
#include "D3D12GpuResource.hpp"
#include "D3D12ShaderManager.hpp"

struct LogFile;

class D3D12Device;
class D3D12DescriptorHeap;
class D3D12CommandObject;
class D3D12ShaderManager;
class D3D12FrameResource;

class GpuResource;

class D3D12RenderPass {
public:
	D3D12RenderPass();
	virtual ~D3D12RenderPass();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData);

	virtual bool CompileShaders();
	virtual bool BuildRootSignatures();
	virtual bool BuildPipelineStates();
	virtual bool AllocateDescriptors();
	virtual bool BuildShaderTables(unsigned numRitems);

	virtual bool OnResize(unsigned width, unsigned height);
	virtual bool Update();

protected:
	D3D12DescriptorHeap* mpDescHeap;
};