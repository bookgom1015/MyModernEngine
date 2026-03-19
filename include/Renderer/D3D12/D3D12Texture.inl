#ifndef __D3D12TEXTURE_INL__
#define __D3D12TEXTURE_INL__

constexpr UINT D3D12Texture::Width() const noexcept {
    return static_cast<UINT>(Metadata.width);
}

constexpr UINT D3D12Texture::Height() const noexcept {
    return static_cast<UINT>(Metadata.height);
}

constexpr UINT D3D12Texture::MipLevels() const noexcept {
    return static_cast<UINT>(Metadata.mipLevels);
}

constexpr DXGI_FORMAT D3D12Texture::Format() const noexcept {
    return Metadata.format;
}

#endif // __D3D12TEXTURE_INL__