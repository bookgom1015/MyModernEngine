#pragma once

#include "D3D12RenderPassManager.hpp"

namespace Ssr {
	namespace Shader {
		enum Type {
			VS_Ssr = 0,
			MS_Ssr,
			PS_Ssr_View,
			PS_Ssr_Screen,

			Count
		};
	}

	namespace RootSignature {
		namespace Default {
			enum {
				CB_Pass = 0,
				CB_Ssr,

				SI_BackBuffer,
				SI_PositionMap,
				SI_NormalMap,
				SI_DepthMap,
				SI_RMSMap,

				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_Ssr_View = 0,
			MP_Ssr_View,

			GP_Ssr_Screen,
			MP_Ssr_Screen,

			Count
		};
	}

	namespace Resource {
		enum Type {
			E_ReflectionMap0 = 0,
			E_ReflectionMap1,

			Count
		};
	}

	namespace Descriptor {
		namespace Srv {
			enum Type {
				E_ReflectionMap0 = 0,
				E_ReflectionMap1,

				Count
			};
		}

		namespace Rtv {
			enum Type {
				E_ReflectionMap0 = 0,
				E_ReflectionMap1,

				Count
			};
		}
	}

	namespace Space {
		enum Type {
			E_View,
			E_Screen
		};
	}
}

class D3D12Ssr : public D3D12RenderPass { 
public:
	struct InitData {
		D3D12Device* Device;
		D3D12CommandObject* CommandObject;
		D3D12ShaderManager* ShaderManager;
		UINT Width;
		UINT Height;
	};

public:
	D3D12Ssr();
	virtual ~D3D12Ssr();

	DISALLOW_COPY_AND_MOVE(D3D12Ssr)

public:
	virtual bool Initialize(D3D12DescriptorHeap* const pDescHeap, void* const pData) override;

	virtual bool CompileShaders() override;
	virtual bool BuildRootSignatures() override;
	virtual bool BuildPipelineStates() override;
	virtual bool AllocateDescriptors() override;

	virtual bool OnResize(unsigned width, unsigned height) override;

public:
	bool Run(
		D3D12FrameResource* const pFrameResource,
		D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
		GpuResource* const pBackBuffer, D3D12_GPU_DESCRIPTOR_HANDLE si_backBuffer,
		GpuResource* const pDepthMap, D3D12_GPU_DESCRIPTOR_HANDLE si_depthMap,
		GpuResource* const pNormalMap, D3D12_GPU_DESCRIPTOR_HANDLE si_normalMap,
		GpuResource* const pRMSMap, D3D12_GPU_DESCRIPTOR_HANDLE si_rmsMap,
		GpuResource* const pPositionMap, D3D12_GPU_DESCRIPTOR_HANDLE si_positionMap,
		Ssr::Space::Type space);

private:
	bool BuildResources();
	bool BuildDescriptors();

public:
	__forceinline GpuResource* GetResource(Ssr::Resource::Type type) const noexcept;

	__forceinline D3D12_GPU_DESCRIPTOR_HANDLE GetSrv(Ssr::Descriptor::Srv::Type type) const noexcept;

private:
	InitData mInitData;

	std::array<Hash, Ssr::Shader::Count> mShaderHashes;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, Ssr::PipelineState::Count> mPipelineStates;

	std::array<std::unique_ptr<GpuResource>, Ssr::Resource::Count> mResources;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, Ssr::Descriptor::Srv::Count> mhSrvs;
	std::array<D3D12DescriptorHeap::DescriptorAllocation, Ssr::Descriptor::Rtv::Count> mhRtvs;
};

REGISTER_RENDER_PASS(D3D12Ssr);

#include "D3D12Ssr.inl"