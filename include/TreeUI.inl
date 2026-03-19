#ifndef __TREEUI_INL__
#define __TREEUI_INL__

const std::string& TreeNode::GetStr() const noexcept { return Str; }

const std::string& TreeNode::GetKey() const noexcept { return Key; }

Ptr<TreeNode> TreeUI::GetSelected() const noexcept { return mSelectedNode; }

void TreeUI::SetDropKey(const std::string& dropKey) noexcept { mDropKey = dropKey; }

const std::string& TreeUI::GetDropKey() const noexcept { return mDropKey; }

#endif // __TREEUI_INL__