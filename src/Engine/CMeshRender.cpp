#include "Engine/pch.h"
#include "Engine/CMeshRender.hpp"

#include "Engine/GameObject.hpp"

#include RENDERER_HEADER

CMeshRender::CMeshRender() 
	: CRenderComponent(EComponent::E_MeshRender) {}

CMeshRender::~CMeshRender() {}

bool CMeshRender::Initialize() {
	CheckReturn(CRenderComponent::Initialize());

	return true;
}

bool CMeshRender::Final() {
	CheckReturn(CRenderComponent::Final());

	return true;
}

bool CMeshRender::CreateMaterial() { return true; };
