#include "pch.h"
#include "ListUI.hpp"

ListUI::ListUI() 
	: EditorUI{ "ListUI" }
	, mpInstance{}
	, mMemberFunc{} {
	SetUIKey("##ListUI");
}

ListUI::~ListUI() {}

void ListUI::DrawUI() {
	ImGui::Separator();

	// ListUI에 등록된 문자열을 TreeNode 위젯으로 표시합니다.
	for (size_t i = 0; i < mItems.size(); ++i) {
		// TreeNode 플래그를 설정합니다.
		UINT Flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

		// 선택된 문자열에는 Selected 플래그를 추가합니다.
		if (i == mSelectedIndex) Flags |= ImGuiTreeNodeFlags_Selected;

		// 트리 노드에 등록한 문자열을 키로 사용하여 표시합니다.
		if (ImGui::TreeNodeEx(mItems[i].c_str(), Flags)) {
			// 위젯을 클릭한 경우
			if (ImGui::IsItemClicked()) {
				// 해당 인덱스를 선택 상태로 저장합니다.
				mSelectedIndex = static_cast<int>(i);
				mSelectedItem = mItems[i];
			}

			if (ImGui::IsItemHovered()
				&& ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				SetActive(false);

				if (mpInstance && mMemberFunc) 
					(mpInstance->*mMemberFunc)(reinterpret_cast<DWORD_PTR>(this));
			}

			ImGui::TreePop();
		}

		ImGui::Separator();
	}
}

void ListUI::OnActivated() { mSelectedItem.clear(); }

void ListUI::OnDeactivated() { mItems.clear(); }