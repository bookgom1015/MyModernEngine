#ifndef __D3D12GPUREOUSRCE_INL__
#define __D3D12GPUREOUSRCE_INL__

void GpuResource::Reset() { mResource.Reset(); }

ID3D12Resource* GpuResource::Resource() const noexcept { return mResource.Get(); }

D3D12_RESOURCE_DESC GpuResource::Desc() const { return mResource->GetDesc(); }

D3D12_RESOURCE_STATES GpuResource::State() const { return mCurrState; }

#endif // __D3D12GPUREOUSRCE_INL__