#pragma once

#include "Renderer/HlslCompaction.h"

struct LogFile;

class D3D12Device;
class D3D12DescriptorHeap;
class D3D12CommandObject;

class GpuResource;

class D3D12RenderPass {
public:
	D3D12RenderPass();
	virtual ~D3D12RenderPass();

public:
	virtual bool Initialize(
		LogFile* const pLogFile, 
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
	LogFile* mpLogFile;

	D3D12DescriptorHeap* mpDescHeap;
};