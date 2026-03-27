#include "pch.h"
#include "CLight.hpp"

#include "CTransform.hpp"

CLight::CLight() : Component{ EComponent::E_Light } {}

CLight::~CLight() {}

bool CLight::Final() {
    auto pos = Transform()->GetRelativePosition();

    mLightData.Position = pos;
	mLightData.Direction = Transform()->GetDirection(ETransformDirection::E_Up) * -1.f;

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

Mat4 CLight::GetMat(int idx) const { 
    switch (idx) {
	case 0: return mLightData.Matrix0;
	case 1: return mLightData.Matrix1;
	case 2: return mLightData.Matrix2;
	case 3: return mLightData.Matrix3;
	case 4: return mLightData.Matrix4;
	case 5: return mLightData.Matrix5;
    default: assert(false && "Invalid matrix index"); return Identity4x4;
    }
}

void CLight::SetMat(Mat4 mat, int idx) { 
    switch (idx) {
	case 0: mLightData.Matrix0 = mat; break;
	case 1: mLightData.Matrix1 = mat; break;
	case 2: mLightData.Matrix2 = mat; break;
	case 3: mLightData.Matrix3 = mat; break;
	case 4: mLightData.Matrix4 = mat; break;
	case 5: mLightData.Matrix5 = mat; break;
    }
}