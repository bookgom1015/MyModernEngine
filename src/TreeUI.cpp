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

				// 앞쪽의 0을 건너뛰고 유효 숫자 구간을 비교합니다.
				size_t na = ia;
				size_t nb = ib;

				while (na < ja && a[na] == '0') ++na;
				while (nb < jb && b[nb] == '0') ++nb;

				const size_t lenA = ja - na;
				const size_t lenB = jb - nb;

				// 유효 숫자의 자릿수가 더 긴 쪽이 큰 값입니다.
				if (lenA != lenB) return lenA < lenB;

				// 자릿수가 같으면 자리별로 비교합니다.
				for (size_t k = 0; k < lenA; ++k)
					if (a[na + k] != b[nb + k]) return a[na + k] < b[nb + k];

				// 숫자 값이 같으면 원래 자릿수가 짧은 쪽을 우선합니다. 예를 들어 3은 03보다 앞에 옵니다.
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
	// TreeNode 플래그를 설정합니다.
	UINT Flags = ImGuiTreeNodeFlags_SpanFullWidth	// 클릭 판정 범위를 넓힙니다.
		| ImGuiTreeNodeFlags_OpenOnDoubleClick		// 더블 클릭으로 열 수 있도록 합니다.
		| ImGuiTreeNodeFlags_OpenOnArrow;			// 화살표를 클릭해서 열 수 있도록 합니다.

	// 자식 노드가 없으면 Leaf 플래그를 추가합니다.
	if (Children.empty()) Flags |= ImGuiTreeNodeFlags_Leaf;

	if (Owner->GetSelected() == this) Flags |= ImGuiTreeNodeFlags_Selected;

	if (Framed) Flags |= ImGuiTreeNodeFlags_Framed;

	std::string nodeName = Str + Key;
	if (Framed && Children.empty()) nodeName = "      " + nodeName;

	// 트리 노드에 등록한 문자열을 키로 사용하여 표시합니다.
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
	// 드래그를 처리합니다.
	if (ImGui::BeginDragDropSource()) {
		// 드래그 블록에서 Text를 그리면 텍스트가 마우스 위치를 따라 표시됩니다.
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
			// 드래그 앤 드롭에 성공한 경우입니다.
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

	// 드래그 중인 노드를 지정된 노드에 놓습니다.
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

	// 최상위 노드로 추가합니다.
	if (parent == nullptr)
		mNodes.push_back(newNode);
	// 지정된 노드의 자식으로 추가합니다.
	else
		parent->AddChildNode(newNode);

	return newNode;
}

void TreeUI::RegisterSelected(Ptr<TreeNode> node) { 
	mSelectedNode = node; 

	if (mSelectedNode != nullptr && mpSelectedUI && mSelectedFunc) (mpSelectedUI->*mSelectedFunc)(node->Data);
}

void TreeUI::RegisterDragged(Ptr<TreeNode> node) { mDraggedNode = node; }

void TreeUI::RegisterDropped(Ptr<TreeNode> node) { mDropTargetNode = node; }

Ptr<TreeNode> TreeUI::FindNodeByName(const std::string& name) const {
	for (const auto& node : mNodes) 
		if (node->Str == name) return node;

	return nullptr;
}

std::string TreeUI::GetSelectedNodeName() const {
	if (mSelectedNode != nullptr) return mSelectedNode->Str;

	return "";
}