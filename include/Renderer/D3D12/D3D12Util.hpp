#pragma once

struct LogFile;

class D3D12Device;

class D3D12Util {
public:
	static const UINT StaticSamplerCount = 11;

	using StaticSamplers = std::array<const D3D12_STATIC_SAMPLER_DESC, StaticSamplerCount>;

public:
	struct D3D12BufferCreateInfo {
		UINT64					Size = 0;
		UINT64					Alignment = 0;
		D3D12_HEAP_TYPE			HeapType = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_HEAP_FLAGS		HeapFlags = D3D12_HEAP_FLAG_NONE;
		D3D12_RESOURCE_FLAGS	Flags = D3D12_RESOURCE_FLAG_NONE;
		D3D12_RESOURCE_STATES	State = D3D12_RESOURCE_STATE_COMMON;

		D3D12BufferCreateInfo();
		D3D12BufferCreateInfo(UINT64 size, D3D12_RESOURCE_FLAGS flags);
		D3D12BufferCreateInfo(UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state);
		D3D12BufferCreateInfo(UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state);
		D3D12BufferCreateInfo(UINT64 size, UINT64 alignment, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state);
		D3D12BufferCreateInfo(UINT64 size, UINT64 alignment, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state);
	};

public:
	static UINT CalcConstantBufferByteSize(UINT byteSize);
	static bool CreateDefaultBuffer(
		D3D12Device* const pDevice,
		ID3D12GraphicsCommandList4* const cmdList,
		const void* const pInitData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer,
		Microsoft::WRL::ComPtr<ID3D12Resource>& defaultBuffer);
	static bool CreateUploadBuffer(
		D3D12Device* const pDevice,
		UINT64 byteSize,
		const IID& riid,
		void** const ppResource);
	static bool CreateBuffer(
		D3D12Device* const pDevice,
		D3D12BufferCreateInfo& info,
		const IID& riid,
		void** const ppResource,
		ID3D12InfoQueue* pInfoQueue = nullptr);

	static bool CreateRootSignature(
		D3D12Device* const pDevice,
		const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc,
		const IID& riid,
		void** const ppRootSignature,
		LPCWSTR name);

	static D3D12_GRAPHICS_PIPELINE_STATE_DESC DefaultPsoDesc(
		D3D12_INPUT_LAYOUT_DESC inputLayout, DXGI_FORMAT dsvFormat);
	static D3D12_GRAPHICS_PIPELINE_STATE_DESC FitToScreenPsoDesc();
	static D3DX12_MESH_SHADER_PIPELINE_STATE_DESC DefaultMeshPsoDesc(DXGI_FORMAT dsvFormat);
	static D3DX12_MESH_SHADER_PIPELINE_STATE_DESC FitToScreenMeshPsoDesc();

	static bool CreateComputePipelineState(
		D3D12Device* const pDevice,
		const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc,
		const IID& riid,
		void** const ppPipelineState,
		LPCWSTR name);
	static bool CreateGraphicsPipelineState(
		D3D12Device* const pDevice,
		const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
		const IID& riid,
		void** const ppPipelineState,
		LPCWSTR name);
	static bool CreatePipelineState(
		D3D12Device* const pDevice,
		const D3DX12_MESH_SHADER_PIPELINE_STATE_DESC& desc,
		const IID& riid,
		void** const ppPipelineState,
		LPCWSTR name);
	static bool CreateStateObject(
		D3D12Device* const pDevice,
		const D3D12_STATE_OBJECT_DESC* pDesc,
		const IID& riid,
		void** const ppStateObject);

	static D3D12_INPUT_LAYOUT_DESC InputLayoutDesc();

	static void CreateShaderResourceView(
		D3D12Device* const pDevice,
		ID3D12Resource* const pResource,
		const D3D12_SHADER_RESOURCE_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);

	static void CreateUnorderedAccessView(
		D3D12Device* const pDevice,
		ID3D12Resource* const pResource,
		ID3D12Resource* const pCounterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);

	static void CreateRenderTargetView(
		D3D12Device* const pDevice,
		ID3D12Resource* const pResource,
		const D3D12_RENDER_TARGET_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);

	static void CreateDepthStencilView(
		D3D12Device* const pDevice,
		ID3D12Resource* const pResource,
		const D3D12_DEPTH_STENCIL_VIEW_DESC* const pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor);

	static const D3D12_STATIC_SAMPLER_DESC* GetStaticSamplers() noexcept;

	template <typename T>
	static void SetRoot32BitConstants(
		UINT RootParameterIndex,
		UINT Num32BitValuesToSet,
		const void* pSrcData,
		UINT DestOffsetIn32BitValues,
		ID3D12GraphicsCommandList6* const pCmdList,
		bool isCompute);


public:
	__forceinline static UINT CeilDivide(UINT value, UINT divisor);

	__forceinline static float Lerp(float a, float b, float  t);
	__forceinline static float Clamp(float a, float _min, float _max);

private:
	static StaticSamplers msStaticSamplers;
};

#include "D3D12Util.inl"