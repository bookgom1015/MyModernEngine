#ifndef __D3D12EYEADAPTION_INL__
#define __D3D12EYEADAPTION_INL__

GpuResource* D3D12EyeAdaption::GetLuminanceMap() const { return mSmoothedLuminance.get(); }

#endif // __D3D12EYEADAPTION_INL__