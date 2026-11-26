#include "Engine/pch.h"
#include "Engine/Inspector.hpp"

#include "Engine/TaskManager.hpp"
#include "Engine/LevelManager.hpp"

#include "Engine/TransformUI.hpp"
#include "Engine/MeshRenderUI.hpp"
#include "Engine/SkeletalMeshRenderUI.hpp"
#include "Engine/SkySphereRenderUI.hpp"
#include "Engine/ReflectionProbeUI.hpp"
#include "Engine/BoxColliderUI.hpp"
#include "Engine/SphereColliderUI.hpp"
#include "Engine/CapsuleColliderUI.hpp"
#include "Engine/MeshColliderUI.hpp"
#include "Engine/RigidbodyUI.hpp"
#include "Engine/LightUI.hpp"
#include "Engine/AddComponentButton.hpp"

#define ADD_COMPONENT_UI(__CompType, __Type, __Lower, __Upper)		\
	mComponentUIs[__CompType] = NEW __Type;							\
	mComponentUIs[__CompType]->SetLowerDampSize(__Lower);			\
	mComponentUIs[__CompType]->SetUpperDampSize(__Upper);			\
	mComponentUIs[__CompType]->SetNeedToSeperator(true);			\
	AddChildUI(mComponentUIs[__CompType].Get());

Inspector::Inspector() : EditorUI("Inspector") {
	CreateChildUIs();
	SetTargetObject(nullptr);
}

Inspector::~Inspector() {}

void Inspector::DrawUI() {}

void Inspector::SetTargetObject(Ptr<GameObject> target) {
	// 전달받은 게임 객체의 정보를 표시할 ComponentUI를 활성화합니다.
	mTargetObject = target;

	mAddCompButton->SetTarget(mTargetObject);

	for (UINT i = 0; i < EComponent::Count; ++i) {
		if (mComponentUIs[i] == nullptr) continue;

		mComponentUIs[i]->SetTarget(mTargetObject);
	}

	//for (UINT i = 0; i < SCRIPT_TYPE::Count; ++i) {
	//	if (m_arrScriptUI[i] == nullptr) continue;
	//
	//	m_arrScriptUI[i]->SetTarget(m_TargetObject);
	//}
	//
	//// AssetUI를 비활성화합니다.
	//m_TargetAsset = nullptr;
	//for (UINT i = 0; i < EAsset::Count; ++i) {
	//	if (m_arrAssetUI[i] != nullptr) m_arrAssetUI[i]->SetActive(false);
	//}
}

void Inspector::CreateChildUIs() {
	ADD_COMPONENT_UI(EComponent::E_Transform, TransformUI, 20.f, 0.f);
	ADD_COMPONENT_UI(EComponent::E_Light, LightUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_MeshRender, MeshRenderUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_SkeletalMeshRender, SkeletalMeshRenderUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_SkySphereRender, SkySphereRenderUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_BoxCollider, BoxColliderUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_SphereCollider, SphereColliderUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_CapsuleCollider, CapsuleColliderUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_MeshCollider, MeshColliderUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_Rigidbody, RigidbodyUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_ReflectionProbe, ReflectionProbeUI, 20.f, 5.f);

	mAddCompButton = NEW AddComponentButton;
	AddChildUI(mAddCompButton.Get());
}