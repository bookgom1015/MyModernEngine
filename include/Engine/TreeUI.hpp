#pragma once

#include "Engine/EditorUI.hpp"

struct TreeNode : public Entity {
public:
	using DrawMenu = std::function<void(TreeNode* const)>;

public:
	TreeNode();
	virtual ~TreeNode();

public:
	bool Draw(DrawMenu drawMenu);

public:
	void AddChildNode(Ptr<TreeNode> node);

	void SetFramed(bool frame);
	void BeginRename(std::function<void(const std::string&)> rename);

public:
	__forceinline const std::string& GetDisplayName() const noexcept;
	__forceinline const std::string& GetKey() const noexcept;

private:
	void ClickCheck();
	void DragCheck();
	void DropCheck();
	void DrawRenameInput();

	std::function<void(const std::string&)> mRename;
	std::string mRenameBuffer;
	bool mFocusRenameInput = false;

public:
	std::string DisplayName;
	std::string Key;

	DWORD_PTR Data;

	TreeNode* Parent;
	std::vector<Ptr<TreeNode>> Children;

	bool Framed;

	class TreeUI* Owner;
};

class TreeUI : public EditorUI {
public:
	TreeUI(TreeNode::DrawMenu drawMenu = nullptr);
	virtual ~TreeUI();

public:
	virtual void DrawUI() override;

public:
	void AddDynamicSelect(EditorUI* pInst, DELEGATE_1 func);
	void AddDynamicDragDrop(EditorUI* pInst, DELEGATE_2 func);

	void Clear();
	Ptr<TreeNode> AddItem(Ptr<TreeNode> parent, std::string name, DWORD_PTR data = 0);

	void RegisterSelected(Ptr<TreeNode> node);
	void RegisterDragged(Ptr<TreeNode> node);
	void RegisterDropped(Ptr<TreeNode> node);

	Ptr<TreeNode> FindNodeByName(const std::string& name) const;
	Ptr<TreeNode> FindNodeByData(DWORD_PTR data) const;
	std::string GetSelectedNodeName() const;

public:
	__forceinline Ptr<TreeNode> GetSelected() const noexcept;

	__forceinline void SetDropKey(const std::string& dropKey) noexcept;
	__forceinline const std::string& GetDropKey() const noexcept;

private:
	std::vector<Ptr<TreeNode>> mNodes;
	Ptr<TreeNode> mSelectedNode;
	Ptr<TreeNode> mDraggedNode;
	Ptr<TreeNode> mDropTargetNode;

	EditorUI* mpSelectedUI;
	DELEGATE_1 mSelectedFunc;

	std::string mDropKey;

	EditorUI* mpDragDropedUI;
	DELEGATE_2 mDragDropedFunc;

	std::optional<TreeNode::DrawMenu> mDrawMenu;
};

#include "Engine/TreeUI.inl"
