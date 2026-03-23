#pragma once

#include "Entity.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
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

	void SetActive(bool state) noexcept;

public:
	__forceinline constexpr bool IsActive() const noexcept;

	__forceinline const std::string& GetUIName() const noexcept;
	__forceinline void SetUIName(const std::string& name) noexcept;

	__forceinline Ptr<EditorUI> GetParentUI() const noexcept;

	__forceinline void SetModal(bool modal) noexcept;
	__forceinline void SetNeedToSeperator(bool need) noexcept;

	__forceinline void SetUpperDampSize(float dampSize) noexcept;
	__forceinline void SetLowerDampSize(float dampSize) noexcept;

protected:
	__forceinline void SetUIKey(const std::string& key) noexcept;

private:
	void CheckFocus();

private:
	std::string mUIName;
	std::string mUIKey;

	bool mbIsModal;
	bool mbActivated;
	bool mbNeedSeperator;

	EditorUI* mpParentUI;
	std::vector<Ptr<EditorUI>> mChildUIs;

	float mUpperDampSize;
	float mLowerDampSize;
};

typedef void(EditorUI::* DELEGATE_0)(void);
typedef void(EditorUI::* DELEGATE_1)(DWORD_PTR);
typedef void(EditorUI::* DELEGATE_2)(DWORD_PTR, DWORD_PTR);

#include "EditorUI.inl"