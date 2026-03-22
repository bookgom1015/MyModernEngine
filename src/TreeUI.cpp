#include "pch.h"
#include "TreeUI.hpp"

namespace {
	bool NaturalLess(const Ptr<TreeNode>& _a, const Ptr<TreeNode>& _b) {
		size_t ia = 0;
		size_t ib = 0;

		auto a = _a->GetStr();
		auto b = _b->GetStr();

		while (ia < a.size() && ib < b.size()) {
			const unsigned char ca = static_cast<unsigned char>(a[ia]);
			const unsigned char cb = static_cast<unsigned char>(b[ib]);

			const bool aIsDigit = std::isdigit(ca) != 0;
			const bool bIsDigit = std::isdigit(cb) != 0;

			if (aIsDigit && bIsDigit) {
				size_t ja = ia;
				size_t jb = ib;

				while (ja < a.size() && std::isdigit(static_cast<unsigned char>(a[ja])))
					++ja;
				while (jb < b.size() && std::isdigit(static_cast<unsigned char>(b[jb])))
					++jb;

				// 앞쪽 0 무시한 숫자 구간 비교
				size_t na = ia;
				size_t nb = ib;

				while (na < ja && a[na] == '0') ++na;
				while (nb < jb && b[nb] == '0') ++nb;

				const size_t lenA = ja - na;
				const size_t lenB = jb - nb;

				// 유효 숫자 길이가 더 긴 쪽이 더 큰 수
				if (lenA != lenB) return lenA < lenB;

				// 길이가 같으면 자리수별 비교
				for (size_t k = 0; k < lenA; ++k)
					if (a[na + k] != b[nb + k]) return a[na + k] < b[nb + k];

				// 숫자값이 같으면 원래 숫자 길이 짧은 쪽 우선 (예: 3 < 03)
				if ((ja - ia) != (jb - ib)) return (ja - ia) < (jb - ib);

				ia = ja;
				ib = jb;
				continue;
			}

			if (ca != cb) return ca < cb;

			++ia;
			++ib;
		}

		return a.size() < b.size();
	}
}

TreeNode::TreeNode()
	: Parent{}
	, Owner{}
	, Framed{} {}

TreeNode::~TreeNode() {}

bool TreeNode::Draw() {
	// TreeNode Flag 설정
	UINT Flags = ImGuiTreeNodeFlags_SpanFullWidth	// 클릭 판정범위 확장
		| ImGuiTreeNodeFlags_OpenOnDoubleClick		// 더블 클릭으로만 열리기
		| ImGuiTreeNodeFlags_OpenOnArrow;			// 화살표 누르면 열리기			   

	// 노드가 자식노드를 보유하고 있지 않으면 Leaf 플래그 추가
	if (Children.empty()) Flags |= ImGuiTreeNodeFlags_Leaf;

	if (Owner->GetSelected() == this) Flags |= ImGuiTreeNodeFlags_Selected;

	if (Framed) Flags |= ImGuiTreeNodeFlags_Framed;

	std::string nodeName = Str + Key;
	if (Framed && Children.empty()) nodeName = "      " + nodeName;

	// 트리노드에 등록한 문자열을 Key 로 해서 출력
	if (ImGui::TreeNodeEx(nodeName.c_str(), Flags)) {
		ClickCheck();
		DragCheck();
		DropCheck();

		for (size_t i = 0; i < Children.size(); ++i)
			Children[i]->Draw();

		ImGui::TreePop();
	}
	else {
		ClickCheck();
		DragCheck();
		DropCheck();
	}

	return true;
}

void TreeNode::AddChildNode(Ptr<TreeNode> node) {
	Children.push_back(node);
	node->Parent = this;

	std::sort(Children.begin(), Children.end(), NaturalLess);
}

void TreeNode::SetFramed(bool frame) { Framed = frame; }

void TreeNode::ClickCheck() {
	if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		Owner->RegisterSelected(this);
}

void TreeNode::DragCheck() {
	// 드래그
	if (ImGui::BeginDragDropSource()) {
		// Drag 사이에 Text 를 넣어주면, 드래그중인 마우스 위치에 Text 가 따라다니면서 렌더링된다.
		ImGui::Text(Str.c_str());

		if (0 != Data)
			ImGui::SetDragDropPayload(
				Owner->GetParentUI()->GetUIName().c_str()
				, &Data, sizeof(DWORD_PTR));

		Owner->RegisterDragged(this);

		ImGui::EndDragDropSource();
	}
}

void TreeNode::DropCheck() {
	if (ImGui::BeginDragDropTarget()) {
		const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(Owner->GetDropKey().c_str());

		if (Payload)
			// Drag Drop 성공
			Owner->RegisterDropped(this);

		ImGui::EndDragDropTarget();
	}
}

TreeUI::TreeUI() 
	: EditorUI("TreeUI")
	, mNodes{}
	, mSelectedNode{}
	, mDraggedNode{}
	, mDropTargetNode{}
	, mpSelectedUI{}
	, mSelectedFunc{}
	, mDropKey{}
	, mpDragDropedUI{}
	, mDragDropedFunc{} {}

TreeUI::~TreeUI() {}

void TreeUI::DrawUI() {
	for (size_t i = 0, end = mNodes.size(); i < end; ++i)
		mNodes[i]->Draw();

	// Drag 하던 노드를 특정 노드에 Drop 함
	if ((mDraggedNode.Get() && mDropTargetNode.Get())
		|| (mDraggedNode.Get() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))) {
		if (mpDragDropedUI && mDragDropedFunc)
			(mpDragDropedUI->*mDragDropedFunc)(
				(DWORD_PTR)mDraggedNode.Get(), (DWORD_PTR)mDropTargetNode.Get());

		mDraggedNode = mDropTargetNode = nullptr;
	}
}

void TreeUI::AddDynamicSelect(EditorUI* pInst, DELEGATE_1 func) {
	mpSelectedUI = pInst; mSelectedFunc = func;
}

void TreeUI::AddDynamicDragDrop(EditorUI* pInst, DELEGATE_2 func) {
	mpDragDropedUI = pInst; mDragDropedFunc = func;
}

void TreeUI::Clear() { mNodes.clear(); }

Ptr<TreeNode> TreeUI::AddItem(Ptr<TreeNode> parent, std::string name, DWORD_PTR data) {
	Ptr<TreeNode> newNode = NEW TreeNode;
	newNode->Str = name;
	newNode->Owner = this;
	newNode->Data = data;

	// 최상위 부모노드로 추가
	if (parent == nullptr)
		mNodes.push_back(newNode);
	// 특정 노드 및에 자식으로 추가
	else
		parent->AddChildNode(newNode);

	return newNode;
}

void TreeUI::RegisterSelected(Ptr<TreeNode> node) { 
	mSelectedNode = node; 

	if (mpSelectedUI && mSelectedFunc) (mpSelectedUI->*mSelectedFunc)(node->Data);
}

void TreeUI::RegisterDragged(Ptr<TreeNode> node) { mDraggedNode = node; }

void TreeUI::RegisterDropped(Ptr<TreeNode> node) { mDropTargetNode = node; }