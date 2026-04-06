#pragma once

#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

namespace Debug {
	namespace Shader {
		enum Type {
			VS_DrawShape = 0,
			MS_DrawShape,
			PS_DrawShape,
			Count = 0
		};
	}

	namespace RootSignature {
		enum Type {
			GR_DrawShape = 0,
			Count
		};
	}

	namespace PipelineState {
		enum Type {
			GP_DrawShape = 0,
			MP_DrawShape,
			Count
		};
	}
}

class D3D12Debug : public D3D12RenderPass {
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Debug();
	virtual ~D3D12Debug();

public:
	virtual bool Initialize(
		D3D12DescriptorHeap* const pDescHeap,
		void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;

private:
	InitData mInitData;

	std::array<Hash, Debug::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, Debug::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, Debug::PipelineState::Count> mPipelineStates;
};

REGISTER_RENDER_PASS(D3D12Debug);