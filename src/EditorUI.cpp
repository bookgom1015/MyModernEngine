#include "pch.h"
#include "EditorUI.hpp"

EditorUI::EditorUI(const std::string& name) 
	: mUIName{ name }
	, mbIsModal{}
	, mbActivated{ true }
	, mpParentUI{} {}

EditorUI::~EditorUI() {}

void EditorUI::Render() {
	if (mbIsModal) {
		
	}
	else if (!mpParentUI) {
		bool Active = mbActivated;

		std::string key = mUIName + mUIKey;

		ImGui::Begin(key.c_str(), &Active);

		if (mbActivated != Active) SetActive(Active);

		CheckFocus();

		RenderUI();

		for (size_t i = 0, end = mChildUIs.size(); i < end; ++i)
			if (mChildUIs[i]->IsActive()) mChildUIs[i]->Render();

		ImGui::End();
	}
	else {
		CheckFocus();

		RenderUI();
		
		for (size_t i = 0, end = mChildUIs.size(); i < end; ++i) {
			if (!mChildUIs[i]->IsActive()) continue;

			mChildUIs[i]->Render();
			ImGui::Separator();
		}
		
		ImGui::Dummy({ mDampSize.x, mDampSize.y});
	}
}

void EditorUI::CheckFocus() {
	//if (ImGui::IsWindowFocused())
	//	EditorMgr::GetInst()->RegisterFocusedUI(this);
	//
	//if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
	//	EditorMgr::GetInst()->RegisterFocusedUI(this);
}
