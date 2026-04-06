#pragma once

#include "D3D12DescriptorHeap.hpp"
#include "Renderer/D3D12/D3D12GpuResource.hpp"

struct D3D12Texture {
public:
    DirectX::TexMetadata Metadata;

    GpuResource Resource;
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadBuffer;

    D3D12_RESOURCE_STATES CurrentState = D3D12_RESOURCE_STATE_COMMON;

	D3D12DescriptorHeap::DescriptorAllocation Allocation;

public:
    void ReleaseUploadBuffer();

public:
    static bool LoadTextureFromFile(
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList,
        const std::wstring& filePath,
        D3D12Texture* outTexture,
        bool generateMips = false);

    static bool BuildTextureShaderResourceView(
        ID3D12Device* device, 
        D3D12Texture* texture,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

public:
    __forceinline constexpr UINT Width() const noexcept;
    __forceinline constexpr UINT Height() const noexcept;
    __forceinline constexpr UINT MipLevels() const noexcept;
    __forceinline constexpr DXGI_FORMAT Format() const noexcept;

};

#include "D3D12Texture.inl"