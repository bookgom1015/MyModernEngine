#pragma once

#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

namespace BlurFilter {
	__forceinline int CalcDiameter(float sigma);
	__forceinline bool CalcGaussWeights(float sigma, float weights[]);

	namespace Shader {
		enum Type {
			CS_GaussianBlurFilter3x3 = 0,
			CS_GaussianBlurFilterRGBA3x3,
			CS_GaussianBlurFilterNxN3x3,
			CS_GaussianBlurFilterNxN5x5,
			CS_GaussianBlurFilterNxN7x7,
			CS_GaussianBlurFilterNxN9x9,
			CS_GaussianBlurFilterRGBANxN3x3,
			CS_GaussianBlurFilterRGBANxN5x5,
			CS_GaussianBlurFilterRGBANxN7x7,
			CS_GaussianBlurFilterRGBANxN9x9,
			Count
		};
	}

	namespace RootSignature {
		namespace Default {
			enum {
				RC_Consts = 0,
				SI_InputMap,
				UO_OutputMap,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			CP_GaussianBlurFilter3x3 = 0,
			CP_GaussianBlurFilterRGBA3x3,
			CP_GaussianBlurFilterNxN3x3,
			CP_GaussianBlurFilterNxN5x5,
			CP_GaussianBlurFilterNxN7x7,
			CP_GaussianBlurFilterNxN9x9,
			CP_GaussianBlurFilterRGBANxN3x3,
			CP_GaussianBlurFilterRGBANxN5x5,
			CP_GaussianBlurFilterRGBANxN7x7,
			CP_GaussianBlurFilterRGBANxN9x9,
			Count
		};
	}
}

class D3D12BlurFilter : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12BlurFilter();
	virtual ~D3D12BlurFilter();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;

public:
	bool GaussianBlur(
		D3D12FrameResource* const pFrameResource,
		BlurFilter::PipelineState::Type type,
		GpuResource* const pInputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE si_inputMap,
		GpuResource* const pOutputMap,
		D3D12_GPU_DESCRIPTOR_HANDLE uo_outputMap,
		UINT texWidth, UINT texHeight);

private:
	InitData mInitData;

	std::array<Hash, BlurFilter::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, BlurFilter::PipelineState::Count> mPipelineStates;
};

#include "D3D12BlurFilter.inl"

REGISTER_RENDER_PASS(D3D12BlurFilter);