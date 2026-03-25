#include "pch.h"
#include "CLight.hpp"

#include "CTransform.hpp"

CLight::CLight() : Component{ EComponent::E_Light } {}

CLight::~CLight() {}

bool CLight::Final() {
    auto pos = Transform()->GetRelativePosition();

    mLightData.Position = pos;
	mLightData.Direction = Transform()->GetDirection(ETransformDirection::E_Forward);

    return true;
}

bool CLight::SaveToLevelFile(FILE* const pFile) { 
    fwrite(&mLightData, sizeof(LightData), 1, pFile);

    return true; 
}

bool CLight::LoadFromLevelFile(FILE* const pFile) { 
    fread(&mLightData, sizeof(LightData), 1, pFile);

    return true; 
}