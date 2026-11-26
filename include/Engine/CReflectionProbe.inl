#ifndef __CREFLECTIONPROBE_INL__
#define __CREFLECTIONPROBE_INL__

void CReflectionProbe::SetProbeShape(EProbeShape::Type shape) noexcept {
	mProbeDesc.Shape = shape;
}

const ReflectionProbeDesc& CReflectionProbe::GetReflectionProbeDesc() noexcept {
	return mProbeDesc;
}

void CReflectionProbe::SetBoxExtents(const Vec3& extents) noexcept {
	mProbeDesc.BoxExtents = extents;
}

void CReflectionProbe::SetRadius(float radius) noexcept {
	mProbeDesc.Radius = radius;
}

void CReflectionProbe::SetPriority(int priority) noexcept {
	mProbeDesc.Priority = priority;
}

void CReflectionProbe::SetBlendDistance(float blendDistance) noexcept {
	mProbeDesc.BlendDistance = blendDistance;
}

void CReflectionProbe::SetEnabled(bool enabled) noexcept {
	mProbeDesc.Enabled = enabled;
}

void CReflectionProbe::SetUseBoxProjection(bool useBoxProjection) noexcept {
	mProbeDesc.UseBoxProjection = useBoxProjection;
}

#endif // __CREFLECTIONPROBE_INL__