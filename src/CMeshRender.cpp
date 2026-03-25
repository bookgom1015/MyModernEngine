#include "pch.h"
#include "CMeshRender.hpp"

#include "GameObject.hpp"

#include RENDERER_HEADER

CMeshRender::CMeshRender() 
	: CRenderComponent(EComponent::E_MeshRender) {}

CMeshRender::~CMeshRender() {}

bool CMeshRender::Initialize() {
	CheckReturn(CRenderComponent::Initialize());

	if (GetMesh() != nullptr) {
		CheckReturn(RENDERER->AddRenderItem(GetOwner()->GetName(), GetMesh()->GetKey(), L""));
		mbAddedRenderItem = true;
	}

	return true;
}

bool CMeshRender::Update(float dt) {
	

	return true;
}

bool CMeshRender::Final() {


	return true;
}

bool CMeshRender::CreateMaterial() { return true; };

bool CMeshRender::OnMeshChanged() {
	if (mbAddedRenderItem) {
		CheckReturn(RENDERER->UpdateRenderItemMesh(GetOwner()->GetName(), GetMesh()->GetKey()));
	}
	else {
		CheckReturn(RENDERER->AddRenderItem(GetOwner()->GetName(), GetMesh()->GetKey(), L""));
		mbAddedRenderItem = true;
	}

	return true;
}

bool CMeshRender::OnMaterialChanged() {
	

	return true;
}

bool CMeshRender::SaveToLevelFile(FILE* const _FileStream) {
	CheckReturn(CRenderComponent::SaveToLevelFile(_FileStream));

	return true;
}

bool CMeshRender::LoadFromLevelFile(FILE* const _FileStream) {
	CheckReturn(CRenderComponent::LoadFromLevelFile(_FileStream));

	if (GetMesh() != nullptr) {
		CheckReturn(RENDERER->AddRenderItem(GetOwner()->GetName(), GetMesh()->GetKey(), L""));
		mbAddedRenderItem = true;
	}

	return false;
}