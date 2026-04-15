#ifndef __CREFLECTIONPROBE_INL__
#define __CREFLECTIONPROBE_INL__

const ReflectionProbeDesc& CReflectionProbe::GetReflectionProbeDesc() noexcept {
	return mProbeDesc;
}

void CReflectionProbe::SetBoxExtents(const Vec3& extents) noexcept {
	mProbeDesc.BoxExtents = extents;
}

#endif // __CREFLECTIONPROBE_INL__