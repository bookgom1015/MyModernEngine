#pragma once

#include "EditorUI.hpp"

class ListUI : public EditorUI {
public:
	ListUI();
	virtual ~ListUI();

public:
	virtual void DrawUI() override;
	virtual void OnActivated() override;
	virtual void OnDeactivated() override;

public:
	__forceinline void AddString(const std::string& _Str);
	__forceinline void AddString(const std::wstring& _WStr);
	__forceinline void AddString(const std::vector<std::string>& _vecStr);
	__forceinline void AddString(const std::vector<std::wstring>& _vecWStr);

	__forceinline void AddDelegate(EditorUI* _Inst, DELEGATE_1 _MemFunc);

	__forceinline const std::string& GetSelectedString() const noexcept;

private:
	std::vector<std::string> mItems;
	std::string mSelectedItem;
	int mSelectedIndex;

	EditorUI* mpInstance;
	DELEGATE_1 mMemberFunc;
};

#include "ListUI.inl"