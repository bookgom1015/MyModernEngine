#include "pch.h"
#include "Inspector.hpp"

#include "TaskManager.hpp"
#include "LevelManager.hpp"

#include "TransformUI.hpp"

Inspector::Inspector() : EditorUI("Inspector") {
	CreateChildUIs();
	SetTargetObject(nullptr);
}

Inspector::~Inspector() {}

void Inspector::DrawUI() {}

void Inspector::SetTargetObject(Ptr<GameObject> target) {
	// 입력된 게임오브젝트의 정보를 보여줄 ComponentUI 들을 활성화 시킨다.
	mTargetObject = target;

	//m_AddCompBtn->SetTarget(m_TargetObject);
	
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
	mComponentUIs[EComponent::E_Transform] = NEW TransformUI;
	AddChildUI(mComponentUIs[EComponent::E_Transform].Get());
}