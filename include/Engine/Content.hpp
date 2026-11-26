#pragma once

#include "Engine/EditorUI.hpp"
#include "Engine/GridUI.hpp"
#include "Engine/ListUI.hpp"

class Content : public EditorUI {
public:
	Content();

	void DrawUI() override;
	void Renew();

protected:
	void DrawChildren() override;

private:
	Ptr<ListUI> mAssetTypeList;
	Ptr<GridUI> mAssetGrid;
	int mDisplayedAssetType = -1;
	bool mbNeedsRenew = true;
};
