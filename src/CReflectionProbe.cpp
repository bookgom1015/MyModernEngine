#include "pch.h"
#include "CReflectionProbe.hpp"

CReflectionProbe::CReflectionProbe()
	: Component(EComponent::E_ReflectionProbe)
	, mProbeDesc{
		.Shape = EProbeShape::E_Box,
		.BakeState = EProbeBakeState::E_Dirty,
		.Radius = 5.f,
		.BoxExtents = Vec3(5.0f),
		.Priority = 0,
		.BlendDistance = 1.f,
		.Enabled = true,
		.UseBoxProjection = false
	} {}

CReflectionProbe::~CReflectionProbe() {}

bool CReflectionProbe::Final() {
	return true;
}

bool CReflectionProbe::SaveToLevelFile(FILE* const pFile) {
	fwrite(&mProbeDesc.Shape, sizeof(mProbeDesc.Shape), 1, pFile);
	fwrite(&mProbeDesc.BakeState, sizeof(mProbeDesc.BakeState), 1, pFile);

	fwrite(&mProbeDesc.Radius, sizeof(mProbeDesc.Radius), 1, pFile);
	fwrite(&mProbeDesc.BoxExtents, sizeof(mProbeDesc.BoxExtents), 1, pFile);

	fwrite(&mProbeDesc.Priority, sizeof(mProbeDesc.Priority), 1, pFile);
	fwrite(&mProbeDesc.BlendDistance, sizeof(mProbeDesc.BlendDistance), 1, pFile);

	fwrite(&mProbeDesc.Enabled, sizeof(mProbeDesc.Enabled), 1, pFile);
	fwrite(&mProbeDesc.UseBoxProjection, sizeof(mProbeDesc.UseBoxProjection), 1, pFile);

    return true;
}

bool CReflectionProbe::LoadFromLevelFile(FILE* const pFile) {
	fread(&mProbeDesc.Shape, sizeof(mProbeDesc.Shape), 1, pFile);
	fread(&mProbeDesc.BakeState, sizeof(mProbeDesc.BakeState), 1, pFile);

	fread(&mProbeDesc.Radius, sizeof(mProbeDesc.Radius), 1, pFile);
	fread(&mProbeDesc.BoxExtents, sizeof(mProbeDesc.BoxExtents), 1, pFile);

	fread(&mProbeDesc.Priority, sizeof(mProbeDesc.Priority), 1, pFile);
	fread(&mProbeDesc.BlendDistance, sizeof(mProbeDesc.BlendDistance), 1, pFile);

	fread(&mProbeDesc.Enabled, sizeof(mProbeDesc.Enabled), 1, pFile);
	fread(&mProbeDesc.UseBoxProjection, sizeof(mProbeDesc.UseBoxProjection), 1, pFile);

    return true;
}