#include "pch.h"
#include "EditorUI.hpp"

EditorUI::EditorUI(const std::string& name) 
	: mUIName{ name }
	, mbIsModal{}
	, mbActivated{ true }
	, mpParentUI{} {}

EditorUI::~EditorUI() {}

void EditorUI::Draw() {
	if (mbIsModal) {
		
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

		DrawUI();
		
		for (size_t i = 0, end = mChildUIs.size(); i < end; ++i) {
			if (!mChildUIs[i]->IsActive()) continue;

			mChildUIs[i]->Draw();
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
