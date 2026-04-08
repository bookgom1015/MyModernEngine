#pragma once

#include "Renderer/D3D12/D3D12RenderPassManager.hpp"

namespace Debug {
	namespace Shader {
		enum Type {
			VS_DrawDebugLine = 0,
			PS_DrawDebugLine,
			Count
		};
	}

	namespace RootSignature {
		enum Type {
			GR_DrawDebugLine = 0,
			Count
		};

		namespace DrawDebugLine {
			enum {
				CB_Pass = 0,
				Count
			};
		}
	}

	namespace PipelineState {
		enum Type {
			GP_DrawDebugLine = 0,
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

public:
	__forceinline const std::vector<DebugLineVertex>& GetDebugLineVertices() const noexcept {
		return mDebugLineVertices;
	}

	__forceinline void ClearDebugLines() noexcept {
		mDebugLineVertices.clear();
	}

public:
	void AddDebugLine(const Vec3& start, const Vec3& end, const Vec4& color);
	void AddWireBox(const Mat4& world, const Vec3& extents, const Vec4& color);
	void AddWireSphere(const Mat4& world, float radius, const Vec4& color, UINT segments = 16);
	void AddDebugCross(const Mat4& world, float size, const Vec4& color);
	void AddDebugArrow(const Mat4& world, float length, const Vec4& color);
	void AddDebugBasis(const Mat4& world, float size);

	void BuildReflectionProbeDebugLines(const ReflectionProbeDesc& desc);

	bool DrawDebugLines(
		D3D12FrameResource* const pFrameResource, 
		D3D12_VIEWPORT viewport, D3D12_RECT scissorRect,
		GpuResource* const pBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE ro_backBuffer,
		GpuResource* const pDepthBuffer, D3D12_CPU_DESCRIPTOR_HANDLE di_depthBuffer);

private:
	InitData mInitData;

	std::array<Hash, Debug::Shader::Count> mShaderHashes;

	std::array<Microsoft::WRL::ComPtr<ID3D12RootSignature>, Debug::RootSignature::Count> mRootSignatures;
	std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, Debug::PipelineState::Count> mPipelineStates;

	std::vector<DebugLineVertex> mDebugLineVertices;
};

REGISTER_RENDER_PASS(D3D12Debug);