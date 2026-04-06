#pragma once

#include "Component.hpp"

class CReflectionProbe : public Component {
public:
	CReflectionProbe();
	virtual ~CReflectionProbe();

public:
    virtual bool Final() override;

public:
	CLONE(CReflectionProbe);

    virtual bool SaveToLevelFile(FILE* const pFile) override;
    virtual bool LoadFromLevelFile(FILE* const pFile) override;

public:
    EProbeShape::Type mShape;
    EProbeBakeState::Type mBakeState;

    float mRadius;
    Vec3 mBoxExtents;

    int mPriority;
    float mBlendDistance;

    bool mbEnabled;
    bool mbUseBoxProjection;
};