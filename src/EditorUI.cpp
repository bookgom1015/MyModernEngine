#include "pch.h"
#include "EditorUI.hpp"

#include "EditorManager.hpp"

EditorUI::EditorUI(const std::string& name) 
	: mUIName{ name }
	, mbIsModal{}
	, mbActivated{ true }
	, mbNeedSeperator{}
	, mpParentUI{}
	, mUpperDampSize{}
	, mLowerDampSize{} {}

EditorUI::~EditorUI() {}

void EditorUI::Draw() {
	if (mbIsModal) {
		std::string key = mUIName + mUIKey;
		ImGui::OpenPopup(key.c_str());

		bool Active = mbActivated;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30.f, 20.f));
		if (ImGui::BeginPopupModal(
			key.c_str()
			, &Active
			, ImGuiWindowFlags_AlwaysAutoResize)) {
			CheckFocus();

			DrawUI();

			for (size_t i = 0, end = mChildUIs.size(); i < end; ++i) {
				if (mChildUIs[i]->IsActive()) {
					mChildUIs[i]->Draw();
					ImGui::Separator();
				}
			}

			ImGui::EndPopup();
		}
		else {
			SetActive(Active);
		}

		ImGui::PopStyleVar();
	}
	else if (!mpParentUI) {
		bool Active = mbActivated;

		std::string key = mUIName + mUIKey;

		ImGui::Begin(key.c_str(), &Active);

		if (mbActivated != Active) SetActive(Active);

		CheckFocus();

		DrawUI();

		for (size_t i = 0, end = mChildUIs.size(); i < end; ++i)
			if (mChildUIs[i]->IsActive()) mChildUIs[i]->Draw();

		ImGui::End();
	}
	else {
		CheckFocus();

		ImGui::Dummy({ 0.f, mUpperDampSize });

		DrawUI();
		
		for (size_t i = 0, end = mChildUIs.size(); i < end; ++i) {
			if (!mChildUIs[i]->IsActive()) continue;

			mChildUIs[i]->Draw();
		}
		
		ImGui::Dummy({ 0.f, mLowerDampSize });
		if (mbNeedSeperator) ImGui::Separator();
	}
}

void EditorUI::AddChildUI(Ptr<EditorUI> child) {
	if (child == nullptr) return;	
	child->mpParentUI = this;
	mChildUIs.push_back(child);
}	

void EditorUI::SetActive(bool state) noexcept {
	if (mbActivated == state)
		return;

	mbActivated = state;

	if (mbActivated)
		OnActivated();
	else
		OnDeactivated();
}

void EditorUI::CheckFocus() {
	if (ImGui::IsWindowFocused())
		EDITOR_MANAGER->RegisterFocusedUI(this);
	
	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
		EDITOR_MANAGER->RegisterFocusedUI(this);
}
