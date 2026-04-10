#include "pch.h"
#include "ComponentUI.hpp"

#include "EditorManager.hpp"
#include "TaskManager.hpp"

ComponentUI::ComponentUI(EComponent::Type type, const std::string& name)
	: EditorUI{ name }
	, mType { type } {}

ComponentUI::~ComponentUI() {}

void ComponentUI::OutputTitle(const std::string& title) {
	ImGui::PushID(0);

	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
	ImGui::Button(title.c_str());
	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	if (mType != EComponent::E_Transform && mType != EComponent::E_Light) {
		EditorManager::RightAlignNextItem({ "X" });
		if (ImGui::Button(std::format("X##{}", title).c_str())) RemoveComponent();
	}

	ImGui::PopID();

	ImGui::Spacing();
	ImGui::Spacing();
}

void ComponentUI::TargetChanged() {}

void ComponentUI::SetTarget(Ptr<GameObject> obj) {
	mTarget = obj;

	if (mType == EComponent::E_CompButton) {
		SetActive(mTarget != nullptr);
		return;
	}
	else if (mType != EComponent::E_Script) {
		if (mTarget == nullptr || mTarget->GetComponent(mType) == nullptr)
			SetActive(false);
		else
			SetActive(true);
	}

	TargetChanged();
}

void ComponentUI::RemoveComponent() {
	TaskInfo info{};
	info.Type = ETask::E_DeferredProcessing;

	if (mType == EComponent::E_Script) {
		//auto scriptUI = static_cast<ScriptUI*>(this);
		//
		//task.Param_0 = DWORD_PTR_DEFERRED_TASK({
		//	m_Target->RemoveComponent(m_ComType, scriptUI->GetScriptType());
		//	SetActive(false);
		//	}, &, scriptUI);
	}
	else {
		info.Param_0 = DWORD_PTR_DEFERRED_TASK({
			mTarget->RemoveComponent(mType);
			SetActive(false);
		}, &);
		TASK_MANAGER->AddTask(info);
	}
}