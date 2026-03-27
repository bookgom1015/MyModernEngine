#include "pch.h"
#include "Renderer/D3D12/D3D12Texture.hpp"

void D3D12Texture::ReleaseUploadBuffer() { 
	UploadBuffer.Reset(); 
}

bool D3D12Texture::LoadTextureFromFile(
    ID3D12Device* device
    , ID3D12GraphicsCommandList* cmdList
    , const std::wstring& filePath
    , D3D12Texture* outTexture
    , bool generateMips) {
    if (!device || !cmdList) return false;

    DirectX::ScratchImage scratch{};
    DirectX::TexMetadata metadata{};

    const std::wstring ext = std::filesystem::path(filePath).extension().wstring();

    HRESULT hr = E_FAIL;
    if (_wcsicmp(ext.c_str(), L".dds") == 0) {
        hr = DirectX::LoadFromDDSFile(
            filePath.c_str(),
            DirectX::DDS_FLAGS_NONE,
            &metadata,
            scratch);
    }
    else if (_wcsicmp(ext.c_str(), L".tga") == 0) {
        hr = DirectX::LoadFromTGAFile(
            filePath.c_str(),
            &metadata,
            scratch);
    }
    else {
        hr = DirectX::LoadFromWICFile(
            filePath.c_str(),
            DirectX::WIC_FLAGS_NONE,
            &metadata,
            scratch);
    }

    if (FAILED(hr)) 
        ReturnFalse(std::format("Failed to load texture from file: {}", WStrToStr(filePath)));

    DirectX::ScratchImage mipChain{};

    const bool isCompressed = DirectX::IsCompressed(metadata.format);

    if (generateMips && metadata.mipLevels <= 1 && !isCompressed) {
        hr = DirectX::GenerateMipMaps(
            scratch.GetImages(),
            scratch.GetImageCount(),
            scratch.GetMetadata(),
            DirectX::TEX_FILTER_DEFAULT,
            0,
            mipChain);

        if (SUCCEEDED(hr)) metadata = mipChain.GetMetadata();
    }

    const DirectX::Image* images = nullptr;
    size_t imageCount = 0;

    if (mipChain.GetImageCount() > 0) {
        images = mipChain.GetImages();
        imageCount = mipChain.GetImageCount();
    }
    else {
        images = scratch.GetImages();
        imageCount = scratch.GetImageCount();
    }

    if (!images || imageCount == 0)
        ReturnFalse(std::format("Failed to load texture from file: {}", WStrToStr(filePath)));

    D3D12_RESOURCE_DESC texDesc{};
    texDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
    texDesc.Alignment = 0;
    texDesc.Width = static_cast<UINT64>(metadata.width);
    texDesc.Height = static_cast<UINT>(metadata.height);
    texDesc.DepthOrArraySize =
        metadata.dimension == DirectX::TEX_DIMENSION_TEXTURE3D
        ? static_cast<UINT16>(metadata.depth)
        : static_cast<UINT16>(metadata.arraySize);
    texDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);
    texDesc.Format = metadata.format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    {
        auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        hr = device->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(outTexture->Resource.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
            ReturnFalse(std::format("Failed to load texture from file: {}", WStrToStr(filePath)));
    }

    const UINT numSubresources = static_cast<UINT>(imageCount);

    UINT64 uploadBufferSize = GetRequiredIntermediateSize(
        outTexture->Resource.Get(),
        0,
        numSubresources);

    {
        auto prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
        hr = device->CreateCommittedResource(
            &prop,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(outTexture->UploadBuffer.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
            ReturnFalse(std::format("Failed to load texture from file: {}", WStrToStr(filePath)));
    }

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    subresources.resize(numSubresources);

    for (UINT i = 0; i < numSubresources; ++i) {
        subresources[i].pData = images[i].pixels;
        subresources[i].RowPitch = static_cast<LONG_PTR>(images[i].rowPitch);
        subresources[i].SlicePitch = static_cast<LONG_PTR>(images[i].slicePitch);
    }

    UpdateSubresources(
        cmdList,
        outTexture->Resource.Get(),
        outTexture->UploadBuffer.Get(),
        0,
        0,
        numSubresources,
        subresources.data());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        outTexture->Resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    cmdList->ResourceBarrier(1, &barrier);

    outTexture->Metadata = metadata;
    outTexture->CurrentState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    return true;
}

bool D3D12Texture::BuildTextureShaderResourceView(
    ID3D12Device* device
    , D3D12Texture* texture
    , D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = texture->Metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    switch (texture->Metadata.dimension) {
    case DirectX::TEX_DIMENSION_TEXTURE1D:
        if (texture->Metadata.arraySize > 1) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            srvDesc.Texture1DArray.MostDetailedMip = 0;
            srvDesc.Texture1DArray.MipLevels = static_cast<UINT>(texture->Metadata.mipLevels);
            srvDesc.Texture1DArray.FirstArraySlice = 0;
            srvDesc.Texture1DArray.ArraySize = static_cast<UINT>(texture->Metadata.arraySize);
            srvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
        }
        else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            srvDesc.Texture1D.MostDetailedMip = 0;
            srvDesc.Texture1D.MipLevels = static_cast<UINT>(texture->Metadata.mipLevels);
            srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
        }
        break;

    case DirectX::TEX_DIMENSION_TEXTURE2D:
        if (texture->Metadata.IsCubemap()) {
            if (texture->Metadata.arraySize > 6) {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                srvDesc.TextureCubeArray.MostDetailedMip = 0;
                srvDesc.TextureCubeArray.MipLevels = static_cast<UINT>(texture->Metadata.mipLevels);
                srvDesc.TextureCubeArray.First2DArrayFace = 0;
                srvDesc.TextureCubeArray.NumCubes = static_cast<UINT>(texture->Metadata.arraySize / 6);
                srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
            }
            else {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MostDetailedMip = 0;
                srvDesc.TextureCube.MipLevels = static_cast<UINT>(texture->Metadata.mipLevels);
                srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
            }
        }
        else if (texture->Metadata.arraySize > 1) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MostDetailedMip = 0;
            srvDesc.Texture2DArray.MipLevels = static_cast<UINT>(texture->Metadata.mipLevels);
            srvDesc.Texture2DArray.FirstArraySlice = 0;
            srvDesc.Texture2DArray.ArraySize = static_cast<UINT>(texture->Metadata.arraySize);
            srvDesc.Texture2DArray.PlaneSlice = 0;
            srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
        }
        else {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = static_cast<UINT>(texture->Metadata.mipLevels);
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        }
        break;

    case DirectX::TEX_DIMENSION_TEXTURE3D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MostDetailedMip = 0;
        srvDesc.Texture3D.MipLevels = static_cast<UINT>(texture->Metadata.mipLevels);
        srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
        break;
    default:
        ReturnFalse("Invalid texture dimension");
    }

    device->CreateShaderResourceView(
        texture->Resource.Get(),
        &srvDesc,
		cpuHandle);

    return true;
}