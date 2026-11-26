#ifndef __EDITORUI_INL__
#define __EDITORUI_INL__

constexpr bool EditorUI::IsActive() const noexcept { return mbActivated; }

const std::string& EditorUI::GetUIName() const noexcept { return mUIName; }

void EditorUI::SetUIName(const std::string& name) noexcept { mUIName = name; }

Ptr<EditorUI> EditorUI::GetParentUI() const noexcept { return mpParentUI; }

void EditorUI::SetModal(bool modal) noexcept { mbIsModal = modal; }

void EditorUI::SetNeedToSeperator(bool need) noexcept { mbNeedSeperator = need; }

void EditorUI::SetUIKey(const std::string& key) noexcept { mUIKey = key; }

void EditorUI::SetUpperDampSize(float dampSize) noexcept { mUpperDampSize = dampSize; }

void EditorUI::SetLowerDampSize(float dampSize) noexcept { mLowerDampSize = dampSize; }

#endif // __EDITORUI_INL__