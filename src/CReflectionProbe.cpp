#include "pch.h"
#include "CReflectionProbe.hpp"

#include RENDERER_HEADER

#include "CTransform.hpp"

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

CReflectionProbe::CReflectionProbe(const CReflectionProbe& other)
	: Component(other) {
	mProbeDesc = other.mProbeDesc;
	mProbeID = other.mProbeID;
}

CReflectionProbe::~CReflectionProbe() {
	RENDERER->RemoveReflectionProbe(mProbeID);
}

bool CReflectionProbe::Initialize() {
	if (!RENDERER->HasReflectionProbe(mProbeID))
		mProbeID = RENDERER->AddReflectionProbe(mProbeDesc);

	return true;
}

bool CReflectionProbe::Final() {
	mProbeDesc.World = Transform()->GetWorldMatrix();

	RENDERER->UpdateReflectionProbe(mProbeID, mProbeDesc);

	return true;
}

bool CReflectionProbe::OnLoaded() {
	if (!RENDERER->HasReflectionProbe(mProbeID))
		mProbeID = RENDERER->AddReflectionProbe(mProbeDesc);

	return true;
}

bool CReflectionProbe::OnUnloaded() {
	RENDERER->RemoveReflectionProbe(mProbeID);

	return true;
}

bool CReflectionProbe::SaveToLevelFile(FILE* const pFile) {
	fwrite(&mProbeDesc, sizeof(mProbeDesc), 1, pFile);

    return true;
}

bool CReflectionProbe::LoadFromLevelFile(FILE* const pFile) {
	fread(&mProbeDesc, sizeof(mProbeDesc), 1, pFile);

    return true;
}