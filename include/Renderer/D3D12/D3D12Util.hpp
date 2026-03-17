#pragma once

struct LogFile;

class D3D12Device;

class D3D12Util {
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
	static bool Initialize(LogFile* const pLogFile);

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

public:
	__forceinline static UINT CeilDivide(UINT value, UINT divisor);

	__forceinline static float Lerp(float a, float b, float  t);
	__forceinline static float Clamp(float a, float _min, float _max);

private:
	static LogFile* mpLogFile;
};

#include "D3D12Util.inl"