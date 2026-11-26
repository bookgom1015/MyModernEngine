#pragma once

#include "D3D12RenderPassManager.hpp"

namespace Taa {
	namespace Shader {
		enum Type {
			VS_Taa= 0,
			MS_Taa,
			PS_Taa,
			Count
		};
	}

	namespace RootSignature {
		namespace Default {
			enum {
				RC_Consts = 0,
				SI_BackBuffer,
				SI_HistoryMap,
				SI_VelocityMap,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_Taa = 0,
			MP_Taa,
			Count
		};
	}
}

class D3D12Taa : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Taa();
	virtual ~D3D12Taa();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;
	virtual bool AllocateDescriptors() override;

	virtual bool OnResize(unsigned width, unsigned height) override;

public:
	__forceinline Vec2 HaltonSequence(size_t index) const;

public:
	bool ApplyTAA(
		D3D12FrameResource* const pFrameResource,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		GpuResource* const pBackBuffer,
		D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pBackBufferCopy,
		D3D12_GPU_DESCRIPTOR_HANDLE si_backBufferCopy,
		GpuResource* const pVelocityMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_velocityMap);

private:
	bool BuildResources();
	bool BuildDescriptors();

public:
	static const size_t HaltonSequenceSize = 16;

private:
	InitData mInitData;

	std::array<Hash, Taa::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, Taa::PipelineState::Count> mPipelineStates;

	std::unique_ptr<GpuResource> mHistoryMap;
	D3D12DescriptorHeap::DescriptorAllocation mhHistoryMapSrv;

	const std::array<Vec2, HaltonSequenceSize> mHaltonSequence = {
			Vec2(0.5f, 0.333333f),
			Vec2(0.25f, 0.666667f),
			Vec2(0.75f, 0.111111f),
			Vec2(0.125f, 0.444444f),
			Vec2(0.625f, 0.777778f),
			Vec2(0.375f, 0.222222f),
			Vec2(0.875f, 0.555556f),
			Vec2(0.0625f, 0.888889f),
			Vec2(0.5625f, 0.037037f),
			Vec2(0.3125f, 0.37037f),
			Vec2(0.8125f, 0.703704f),
			Vec2(0.1875f, 0.148148f),
			Vec2(0.6875f, 0.481481f),
			Vec2(0.4375f, 0.814815f),
			Vec2(0.9375f, 0.259259f),
			Vec2(0.03125f, 0.592593f)
	};
	std::array<Vec2, HaltonSequenceSize> mFittedToBakcBufferHaltonSequence;
};

Vec2 D3D12Taa::HaltonSequence(size_t index) const {
	return mFittedToBakcBufferHaltonSequence[index];
}

REGISTER_RENDER_PASS(D3D12Taa);