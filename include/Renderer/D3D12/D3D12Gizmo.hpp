#pragma once

#include "D3D12RenderPassManager.hpp"

namespace Gizmo {
	namespace Shader {
		enum Type {
			VS_DrawAxisLine = 0,
			PS_DrawAxisLine,
			VS_DrawAxisCap,
			PS_DrawAxisCap,
			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_DrawAxisLine = 0,
			Count
		};

		namespace DrawAxisLine {
			enum {
				CB_Gizmo = 0,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_DrawAxisLine = 0,
			GP_DrawAxisCap,
			Count
		};

		namespace DrawAxisLine {
			enum {
				Default = 0,
				Count
			};
		}
	}
}

class D3D12Gizmo : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Gizmo();
	virtual ~D3D12Gizmo();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;

public:
	bool DrawAxisLine(
		D3D12FrameResource* const pFrameResource,
		GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pDepthBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE dio_depthBuffer);
	bool DrawAxisCap(
		D3D12FrameResource* const pFrameResource,
		GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pDepthBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE dio_depthBuffer);


private:
	InitData mInitData;

	std::array<Hash, Gizmo::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, Gizmo::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, Gizmo::PipelineState::Count> mPipelineStates;

	D3D12_VIEWPORT mDrawAxisLineViewport;
	D3D12_RECT mDrawAxisLineScissorRect;
};

REGISTER_RENDER_PASS(D3D12Gizmo);