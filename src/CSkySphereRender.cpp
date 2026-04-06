#include "pch.h"
#include "CSkySphereRender.hpp"

#include "AssetManager.hpp"

CSkySphereRender::CSkySphereRender() 
	: CRenderComponent(EComponent::E_SkySphereRender) {}

CSkySphereRender::~CSkySphereRender() {}

bool CSkySphereRender::Initialize() {
	CheckReturn(CRenderComponent::Initialize());

	return true;
}

bool CSkySphereRender::Final() {
	CheckReturn(CRenderComponent::Final());

	return true;
}

bool CSkySphereRender::CreateMaterial() { return true; }

bool CSkySphereRender::SaveToLevelFile(FILE* const pFile) {
	CheckReturn(CRenderComponent::SaveToLevelFile(pFile));

	SaveAssetRef(pFile, mEnvironmentCubeMap.Get());

	return true;
}

bool CSkySphereRender::LoadFromLevelFile(FILE* const pFile) {
	CheckReturn(CRenderComponent::LoadFromLevelFile(pFile));

	mEnvironmentCubeMap = LoadAssetRef<ATexture>(pFile);

	return true;
}