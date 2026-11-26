#include "Engine/pch.h"
#include "Engine/TreeUI.hpp"

TreeNode::TreeNode()
	: Data{}
	, Parent{}
	, Owner{}
	, Framed{} {}

TreeNode::~TreeNode() {}

bool TreeNode::Draw(DrawMenu drawMenu) {
	// TreeNode 플래그를 설정합니다.
	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_SpanFullWidth	// 클릭 판정 범위를 넓힙니다.
		| ImGuiTreeNodeFlags_OpenOnDoubleClick		// 더블 클릭으로 열 수 있도록 합니다.
		| ImGuiTreeNodeFlags_OpenOnArrow;			// 화살표를 클릭해서 열 수 있도록 합니다.

	// 자식 노드가 없으면 Leaf 플래그를 추가합니다.
	if (Children.empty()) Flags |= ImGuiTreeNodeFlags_Leaf;

	if (Owner->GetSelected() == this) Flags |= ImGuiTreeNodeFlags_Selected;

	if (Framed) Flags |= ImGuiTreeNodeFlags_Framed;

	std::string nodeName = DisplayName + Key;
	if (Framed && Children.empty()) nodeName = "      " + nodeName;

	// 객체 주소를 ID 범위에 포함해 같은 이름의 항목을 구분합니다.
	if (Data != 0)
		ImGui::PushID(reinterpret_cast<const void*>(Data));

	const bool renaming = static_cast<bool>(mRename);
	if (renaming)
		Flags &= ~ImGuiTreeNodeFlags_SpanFullWidth;

	const bool open = Data != 0 
		? ImGui::TreeNodeEx("node", Flags, "%s", renaming ? "" : nodeName.c_str()) 
		: ImGui::TreeNodeEx(nodeName.c_str(), Flags);

	if (renaming) {
		DrawRenameInput();
	}
	else {
		if (drawMenu)
			drawMenu(this);

		ClickCheck();
		DropCheck();
		DragCheck();
	}

	if (open) {
		for (const auto& child : Children)
			child->Draw(drawMenu);

		ImGui::TreePop();
	}

	if (Data != 0)
		ImGui::PopID();

	return true;
}

void TreeNode::AddChildNode(Ptr<TreeNode> node) {
	Children.push_back(node);
	node->Parent = this;

	std::sort(Children.begin(), Children.end(), [](const Ptr<TreeNode>& a, const Ptr<TreeNode>& b) {
		return NaturalLess(a->GetDisplayName(), b->GetDisplayName());
	});
}

void TreeNode::SetFramed(bool frame) { Framed = frame; }

void TreeNode::BeginRename(std::function<void(const std::string&)> rename) {
	mRenameBuffer = DisplayName;
	mRename = std::move(rename);
	mFocusRenameInput = true;
}

void TreeNode::DrawRenameInput() {
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-FLT_MIN);

	const bool requestingFocus = std::exchange(mFocusRenameInput, false);
	if (requestingFocus)
		ImGui::SetKeyboardFocusHere();

	constexpr auto flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
	const bool submitted = ImGui::InputText("##Rename", &mRenameBuffer, flags);
	if (!submitted && !ImGui::IsItemDeactivated())
		return;

	if (!submitted && ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
		mRename = nullptr;
		return;
	}

	const bool validName = mRenameBuffer.find_first_not_of(" \t\r\n") != std::string::npos;
	if (submitted && !validName) {
		mFocusRenameInput = true;
		return;
	}

	if (validName) {
		mRename(mRenameBuffer);
		DisplayName = std::move(mRenameBuffer);
	}
	mRename = nullptr;
}

void TreeNode::ClickCheck() {
	if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)
		&& !ImGui::GetDragDropPayload())
		Owner->RegisterSelected(this);
}

void TreeNode::DragCheck() {
	if (Data == 0)
		return;

	const auto parent = Owner->GetParentUI();
	const auto& payloadType = parent != nullptr ? parent->GetUIName() : Owner->GetUIName();
	if (ImGui::BeginDragDropSource()) {
		ImGui::SetDragDropPayload(payloadType.c_str(), &Data, sizeof(Data));
		ImGui::TextUnformatted(DisplayName.c_str());
		Owner->RegisterDragged(this);
		ImGui::EndDragDropSource();
	}
}

void TreeNode::DropCheck() {
	const auto& dropKey = Owner->GetDropKey();
	if (!dropKey.empty() && ImGui::BeginDragDropTarget()) {
		if (ImGui::AcceptDragDropPayload(dropKey.c_str()))
			Owner->RegisterDropped(this);

		ImGui::EndDragDropTarget();
	}
}

TreeUI::TreeUI(TreeNode::DrawMenu drawMenu)
	: EditorUI("TreeUI")
	, mNodes{}
	, mSelectedNode{}
	, mDraggedNode{}
	, mDropTargetNode{}
	, mpSelectedUI{}
	, mSelectedFunc{}
	, mDropKey{}
	, mpDragDropedUI{}
	, mDragDropedFunc{}
	, mDrawMenu{ drawMenu } {}

TreeUI::~TreeUI() {}

void TreeUI::DrawUI() {
	{
		ImGui::PushID(this);

		for (const auto& node : mNodes)
			node->Draw(mDrawMenu.has_value() ? mDrawMenu.value() : nullptr);

		ImGui::PopID();
	}

	if (mDraggedNode != nullptr) {
		if (mDropTargetNode != nullptr || (ImGui::GetDragDropPayload() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))) {
			// 콜백이 트리를 갱신하더라도 전달하는 노드는 호출이 끝날 때까지 유지합니다.
			const auto draggedNode = mDraggedNode;
			const auto dropTargetNode = mDropTargetNode;
			mDraggedNode = nullptr;
			mDropTargetNode = nullptr;

			if (mpDragDropedUI && mDragDropedFunc)
				(mpDragDropedUI->*mDragDropedFunc)(
					reinterpret_cast<DWORD_PTR>(draggedNode.Get()),
					reinterpret_cast<DWORD_PTR>(dropTargetNode.Get()));
		}
		else if (!ImGui::GetDragDropPayload()) {
			mDraggedNode = nullptr;
		}
	}

	mDropTargetNode = nullptr;
}

void TreeUI::AddDynamicSelect(EditorUI* pInst, DELEGATE_1 func) {
	mpSelectedUI = pInst; mSelectedFunc = func;
}

void TreeUI::AddDynamicDragDrop(EditorUI* pInst, DELEGATE_2 func) {
	mpDragDropedUI = pInst; mDragDropedFunc = func;
}

void TreeUI::Clear() {
	mSelectedNode = nullptr;
	mDraggedNode = nullptr;
	mDropTargetNode = nullptr;
	mNodes.clear();
}

Ptr<TreeNode> TreeUI::AddItem(Ptr<TreeNode> parent, std::string name, DWORD_PTR data) {
	Ptr<TreeNode> newNode = NEW TreeNode;
	newNode->DisplayName = std::move(name);
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

	if (mSelectedNode != nullptr && mpSelectedUI && mSelectedFunc) 
		(mpSelectedUI->*mSelectedFunc)(node->Data);
}

void TreeUI::RegisterDragged(Ptr<TreeNode> node) { mDraggedNode = node; }

void TreeUI::RegisterDropped(Ptr<TreeNode> node) { mDropTargetNode = node; }

Ptr<TreeNode> TreeUI::FindNodeByName(const std::string& name) const {
	for (const auto& node : mNodes) 
		if (node->DisplayName == name) return node;

	return nullptr;
}

Ptr<TreeNode> TreeUI::FindNodeByData(DWORD_PTR data) const {
	if (data == 0)
		return nullptr;

	const auto find = [data](const auto& self, const auto& nodes) -> Ptr<TreeNode> {
		for (const auto& node : nodes) {
			if (node->Data == data)
				return node;

			if (auto child = self(self, node->Children); child != nullptr)
				return child;
		}

		return nullptr;
	};

	return find(find, mNodes);
}

std::string TreeUI::GetSelectedNodeName() const {
	if (mSelectedNode != nullptr) return mSelectedNode->DisplayName;

	return "";
}
