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

	// ListUI 에 등록된 문자열들을 TreeNode 위젯으로 출력
	for (size_t i = 0; i < mItems.size(); ++i) {
		// TreeNode Flag 설정
		UINT Flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

		// 선택된 문자열은 Selected 플래그 추가
		if (i == mSelectedIndex) Flags |= ImGuiTreeNodeFlags_Selected;

		// 트리노드에 등록한 문자열을 Key 로 해서 출력
		if (ImGui::TreeNodeEx(mItems[i].c_str(), Flags)) {
			// 해당 위젯이 클릭된 적이 있으면
			if (ImGui::IsItemClicked()) {
				// 선택된 상태로 인덱스를 기억
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