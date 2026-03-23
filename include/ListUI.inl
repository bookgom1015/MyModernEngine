#ifndef __LISTUI_INL__
#define __LISTUI_INL__

void ListUI::AddString(const std::string& str) { mItems.push_back(str); }

void ListUI::AddString(const std::wstring& wstr) { mItems.push_back(WStrToStr(wstr)); }

void ListUI::AddString(const std::vector<std::string>& vecStr) { 
	mItems.insert(mItems.end(), vecStr.begin(), vecStr.end()); }

void ListUI::AddString(const std::vector<std::wstring>& vecWStr) { 
	for (const auto& wstr : vecWStr) 
		mItems.push_back(WStrToStr(wstr));
}

void ListUI::AddDelegate(EditorUI* pInst, DELEGATE_1 memFunc) {
	mpInstance = pInst;
	mMemberFunc = memFunc;
}

const std::string& ListUI::GetSelectedString() const noexcept { return mSelectedItem; }

#endif	// __LISTUI_INL__