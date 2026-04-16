#pragma once

#include "Component.hpp"

class CReflectionProbe : public Component {
public:
	CReflectionProbe();
	CReflectionProbe(const CReflectionProbe& other);
	virtual ~CReflectionProbe();

public:
	virtual bool Initialize() override;
    virtual bool Final() override;

	virtual bool OnLoaded() override;
	virtual bool OnUnloaded() override;

public:
	__forceinline const ReflectionProbeDesc& GetReflectionProbeDesc() noexcept;

	__forceinline void SetProbeShape(EProbeShape::Type shape) noexcept;
	__forceinline void SetBoxExtents(const Vec3& extents) noexcept;
	__forceinline void SetRadius(float radius) noexcept;
	__forceinline void SetPriority(int priority) noexcept;
	__forceinline void SetBlendDistance(float blendDistance) noexcept;
	__forceinline void SetEnabled(bool enabled) noexcept;
	__forceinline void SetUseBoxProjection(bool useBoxProjection) noexcept;

public:
	CLONE(CReflectionProbe);

    virtual bool SaveToLevelFile(FILE* const pFile) override;
    virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
    ReflectionProbeDesc mProbeDesc;
	ReflectionProbeID mProbeID;
};

#include "CReflectionProbe.inl"