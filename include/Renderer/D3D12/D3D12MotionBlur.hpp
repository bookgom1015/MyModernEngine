#pragma once

#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

namespace MotionBlur {
	namespace Shader {
		enum Type {
			VS_MotionBlur = 0,
			MS_MotionBlur,
			PS_MotionBlur,
			Count
		};
	}

	namespace RootSignature {
		namespace Default {
			enum {
				RC_Consts = 0,
				SI_BackBuffer,
				SI_DepthMap,
				SI_VelocityMap,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_MotionBlur = 0,
			MP_MotionBlur,
			Count
		};
	}
}

class D3D12MotionBlur : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
		UINT Depth;
	};

public:
	D3D12MotionBlur();
	virtual ~D3D12MotionBlur();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;

public:
	bool ApplyMotionBlur(
		D3D12FrameResource* const pFrameResource,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pBackBufferCopy, D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		GpuResource* const pVelocityMap, D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap);

private:
	InitData mInitData;

	std::array<Hash, MotionBlur::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>,
		MotionBlur::PipelineState::Count> mPipelineStates;
};

REGISTER_RENDER_PASS(D3D12MotionBlur);