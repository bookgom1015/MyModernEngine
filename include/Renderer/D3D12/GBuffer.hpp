#pragma once

#include "Renderer/D3D12/D3D12RenderPass.hpp"

class GBuffer : public D3D12RenderPass {
public:
	struct InitData {
		BOOL MeshShaderSupported;
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		UINT Width;
		UINT Height;
	};

public:
	GBuffer();
	virtual ~GBuffer();

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

private:
	
};