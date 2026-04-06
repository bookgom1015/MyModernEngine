#include "pch.h"
#include "Inspector.hpp"

#include "TaskManager.hpp"
#include "LevelManager.hpp"

#include "TransformUI.hpp"
#include "MeshRenderUI.hpp"
#include "SkeletalMeshRenderUI.hpp"
#include "SkySphereRenderUI.hpp"
#include "ReflectionProbeUI.hpp"
#include "LightUi.hpp"
#include "AddComponentButton.hpp"

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
	// 입력된 게임오브젝트의 정보를 보여줄 ComponentUI 들을 활성화 시킨다.
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
	//// AssetUI 를 비활성화한다.
	//m_TargetAsset = nullptr;
	//for (UINT i = 0; i < EAsset::Count; ++i) {
	//	if (m_arrAssetUI[i] != nullptr) m_arrAssetUI[i]->SetActive(false);
	//}
}

void Inspector::NeedToResetTarget() {
	if (mTargetObject == nullptr) return;

	auto name = mTargetObject->GetName();

	TaskInfo info{};
	info.Type = ETask::E_DeferredProcessing;
	info.Param_0 = DWORD_PTR_DEFERRED_TASK({
		auto target = LEVEL_MANAGER->FindObjectByName(name);
		SetTargetObject(target);
		}, &, name);

	TASK_MANAGER->AddTask(info);
}

void Inspector::CreateChildUIs() {
	ADD_COMPONENT_UI(EComponent::E_Transform, TransformUI, 20.f, 0.f);
	ADD_COMPONENT_UI(EComponent::E_MeshRender, MeshRenderUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_SkeletalMeshRender, SkeletalMeshRenderUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_SkySphereRender, SkySphereRenderUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_ReflectionProbe, ReflectionProbeUI, 20.f, 5.f);
	ADD_COMPONENT_UI(EComponent::E_Light, LightUI, 20.f, 5.f);

	mAddCompButton = NEW AddComponentButton;
	AddChildUI(mAddCompButton.Get());
}