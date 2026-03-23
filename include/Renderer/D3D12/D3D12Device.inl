#ifndef __D3D12DEVICE_INL__
#define __D3D12DEVICE_INL__

constexpr bool D3D12Device::IsAllowingTearing() const noexcept { return mbAllowTearing == TRUE; }

ID3D12Device5* D3D12Device::GetD3DDevice() const noexcept { return md3dDevice.Get(); }

bool D3D12Device::IsRaytracingSupported() const noexcept { return mbRaytracingSupported; }

bool D3D12Device::IsMeshShaderSupported() const noexcept { return mbMeshShaderSupported; }

#endif // __D3D12DEVICE_INL__