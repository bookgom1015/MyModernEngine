#pragma once

#include "Entity.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#if defined(_D3D12)
	#include <imgui/backends/imgui_impl_dx12.h>
#endif

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
	void AddChildUI(Ptr<EditorUI> child);

public:
	__forceinline constexpr bool IsActive() const noexcept;
	__forceinline void SetActive(bool state) noexcept;

	__forceinline const std::string& GetUIName() const noexcept;

	__forceinline Ptr<EditorUI> GetParentUI() const noexcept;

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

typedef void(EditorUI::* DELEGATE_0)(void);
typedef void(EditorUI::* DELEGATE_1)(DWORD_PTR);
typedef void(EditorUI::* DELEGATE_2)(DWORD_PTR, DWORD_PTR);

constexpr bool EditorUI::IsActive() const noexcept { return mbActivated; }

void EditorUI::SetActive(bool state) noexcept { mbActivated = state; }

const std::string& EditorUI::GetUIName() const noexcept { return mUIName; }

Ptr<EditorUI> EditorUI::GetParentUI() const noexcept { return mpParentUI; }