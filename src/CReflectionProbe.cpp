#include "pch.h"
#include "CReflectionProbe.hpp"

CReflectionProbe::CReflectionProbe() 
	: Component(EComponent::E_ReflectionProbe)
	, mShape{ EProbeShape::E_Box }
	, mBakeState{ EProbeBakeState::E_Dirty } 
	, mRadius{ 5.f }
	, mBoxExtents{ Vec3(5.0f) }
	, mPriority{ 0 }
	, mBlendDistance{ 1.0f }
	, mbEnabled{ true }
	, mbUseBoxProjection{ false } {}

CReflectionProbe::~CReflectionProbe() {}

bool CReflectionProbe::Final() {
	return true;
}

bool CReflectionProbe::SaveToLevelFile(FILE* const pFile) {
	fwrite(&mShape, sizeof(mShape), 1, pFile);
	fwrite(&mBakeState, sizeof(mBakeState), 1, pFile);

	fwrite(&mRadius, sizeof(mRadius), 1, pFile);
	fwrite(&mBoxExtents, sizeof(mBoxExtents), 1, pFile);

	fwrite(&mPriority, sizeof(mPriority), 1, pFile);
	fwrite(&mBlendDistance, sizeof(mBlendDistance), 1, pFile);

	fwrite(&mbEnabled, sizeof(mbEnabled), 1, pFile);
	fwrite(&mbUseBoxProjection, sizeof(mbUseBoxProjection), 1, pFile);

    return true;
}

bool CReflectionProbe::LoadFromLevelFile(FILE* const pFile) {
	fread(&mShape, sizeof(mShape), 1, pFile);
	fread(&mBakeState, sizeof(mBakeState), 1, pFile);

	fread(&mRadius, sizeof(mRadius), 1, pFile);
	fread(&mBoxExtents, sizeof(mBoxExtents), 1, pFile);

	fread(&mPriority, sizeof(mPriority), 1, pFile);
	fread(&mBlendDistance, sizeof(mBlendDistance), 1, pFile);

	fread(&mbEnabled, sizeof(mbEnabled), 1, pFile);
	fread(&mbUseBoxProjection, sizeof(mbUseBoxProjection), 1, pFile);

    return true;
}