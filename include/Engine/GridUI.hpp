#pragma once

#include "Engine/EditorUI.hpp"

#include <optional>
#include <string>
#include <vector>

struct GridNode : public Entity {
public:
	GridNode();
	virtual ~GridNode() = default;

public:
	bool Draw(const ImVec2& tileSize, bool selected) const;

public:
	std::string DisplayName;
	std::string Key; // 같은 그리드 안에서 고유해야 합니다.

	ImTextureID Thumbnail;
	ImVec2 ThumbnailSize;

protected:
	virtual void DrawDragSource() const {}
};

class GridUI : public EditorUI {
public:
	GridUI();
	virtual ~GridUI() = default;

public:
	void DrawUI() override;

public:
	void SetItems(std::vector<Ptr<GridNode>> items);
	Ptr<GridNode> GetSelected() const;

	__forceinline const std::optional<std::string>& GetSelectedKey() const noexcept;

private:
	std::vector<Ptr<GridNode>> mNodes;
	std::optional<std::string> mSelectedKey;
};

const std::optional<std::string>& GridUI::GetSelectedKey() const noexcept {	return mSelectedKey; }
