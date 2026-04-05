#include "pch.h"
#include "CSkySphereRender.hpp"

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