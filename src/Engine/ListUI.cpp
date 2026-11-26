#include "Engine/pch.h"
#include "Engine/ListUI.hpp"

ListUI::ListUI(bool embedded)
	: EditorUI{ "ListUI" }
	, mbEmbedded{ embedded }
	, mpInstance{}
	, mMemberFunc{} {
	SetUIKey("##ListUI");
}

ListUI::~ListUI() {}

void ListUI::DrawUI() {
	if (!mbEmbedded)
		ImGui::Separator();

	ImGui::PushID(this);
	for (int i = 0; i < static_cast<int>(mItems.size()); ++i) {
		ImGui::PushID(i);
		if (mbEmbedded) {
			if (ImGui::Selectable(mItems[i].c_str(), i == mSelectedIndex))
				SetSelectedIndex(i);
			ImGui::PopID();
			continue;
		}

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
		if (i == mSelectedIndex)
			flags |= ImGuiTreeNodeFlags_Selected;

		bool confirmed = false;
		if (ImGui::TreeNodeEx(mItems[i].c_str(), flags)) {
			confirmed = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
			if (ImGui::IsItemClicked() || confirmed)
				SetSelectedIndex(i);
			ImGui::TreePop();
		}
		ImGui::PopID();

		if (confirmed) {
			ImGui::PopID();
			SetActive(false);
			if (mpInstance && mMemberFunc)
				(mpInstance->*mMemberFunc)(reinterpret_cast<DWORD_PTR>(this));
			return;
		}

		ImGui::Separator();
	}
	ImGui::PopID();
}

void ListUI::SetSelectedIndex(int index) {
	mSelectedIndex = index >= 0 && static_cast<size_t>(index) < mItems.size() ? index : -1;
	mSelectedItem = mSelectedIndex >= 0 ? mItems[mSelectedIndex] : std::string{};
}

void ListUI::OnActivated() {
	if (!mbEmbedded)
		SetSelectedIndex(-1);
}

void ListUI::OnDeactivated() {
	if (!mbEmbedded)
		mItems.clear();
}
