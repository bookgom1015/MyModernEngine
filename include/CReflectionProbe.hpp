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

public:
	__forceinline const ReflectionProbeDesc& GetReflectionProbeDesc() noexcept;

public:
	CLONE(CReflectionProbe);

    virtual bool SaveToLevelFile(FILE* const pFile) override;
    virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
    ReflectionProbeDesc mProbeDesc;
	ReflectionProbeID mProbeID;
};

#include "CReflectionProbe.inl"