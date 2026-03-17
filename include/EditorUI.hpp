#pragma once

#include "Entity.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>

class EditorUI : public Entity {
public:
	EditorUI(const std::string& name);
	virtual ~EditorUI();

public:
	virtual void Draw();
	virtual void DrawUI() = 0;

	virtual void OnActivated() {};
	virtual void OnDeactivated() {};

public:
	__forceinline constexpr bool IsActive() const noexcept;
	__forceinline void SetActive(bool state) noexcept;

	__forceinline const std::string& GetUIName() const noexcept;

private:
	void CheckFocus();

private:
	std::string mUIName;
	std::string mUIKey;

	bool mbIsModal;
	bool mbActivated;

	EditorUI* mpParentUI;
	std::vector<Ptr<EditorUI>> mChildUIs;

	Vec2 mDampSize;
};

constexpr bool EditorUI::IsActive() const noexcept { return mbActivated; }

void EditorUI::SetActive(bool state) noexcept { mbActivated = state; }

const std::string& EditorUI::GetUIName() const noexcept { return mUIName; }