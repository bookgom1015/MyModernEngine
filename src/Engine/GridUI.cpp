#include "Engine/pch.h"
#include "Engine/GridUI.hpp"

#include <imgui/imgui_internal.h>

#include <algorithm>
#include <utility>

namespace {
	// 그리드 UI의 타일 크기와 간격을 정의합니다.	
	constexpr float TileWidth = 240.0f;
	// 그리드 UI의 타일 미리보기 영역의 높이를 정의합니다.
	constexpr float PreviewHeight = 160.0f;
	// 타일 내부의 여백을 정의합니다.
	constexpr float Padding = 12.0f;
	// 타일 간의 간격을 정의합니다.
	constexpr float Gap = 8.0f;
}

GridNode::GridNode() 
	: Entity() 
	, DisplayName{}
	, Key{}
	, Thumbnail(ImTextureID_Invalid)
	, ThumbnailSize{} {}

bool GridNode::Draw(const ImVec2& tileSize, bool selected) const {
	ImGui::PushID(Key.c_str());
	const bool clicked = ImGui::InvisibleButton("##tile", tileSize);
	ImGui::PopID();

	// 화면 밖 타일도 배치에는 참여하지만 그리기 명령은 추가하지 않습니다.
	if (!ImGui::IsItemVisible())
		return clicked;

	const bool hovered = ImGui::IsItemHovered();
	const ImVec2 min = ImGui::GetItemRectMin();
	const ImVec2 max = ImGui::GetItemRectMax();

	auto draw = ImGui::GetWindowDrawList();
	const auto background = selected ? ImGuiCol_HeaderActive
		: hovered ? ImGuiCol_HeaderHovered : ImGuiCol_FrameBg;
	draw->AddRectFilled(min, max, ImGui::GetColorU32(background), 5.0f);

	const ImVec2 previewMin(min.x + Padding, min.y + Padding);
	const ImVec2 previewMax(max.x - Padding, previewMin.y + PreviewHeight);
	if (previewMax.x > previewMin.x)
		draw->AddRectFilled(previewMin, previewMax, ImGui::GetColorU32(ImGuiCol_WindowBg), 3.0f);

	if (Thumbnail != ImTextureID_Invalid && ThumbnailSize.x > 0.0f
		&& ThumbnailSize.y > 0.0f
		&& previewMax.x > previewMin.x) {

		const float previewWidth = previewMax.x - previewMin.x;
		const float scale = std::min(
			previewWidth / ThumbnailSize.x,
			PreviewHeight / ThumbnailSize.y);

		const ImVec2 size(
			ThumbnailSize.x * scale,
			ThumbnailSize.y * scale);

		const ImVec2 imageMin(
			previewMin.x + (previewWidth - size.x) * 0.5f,
			previewMin.y + (PreviewHeight - size.y) * 0.5f);

		draw->AddImage(
			ImTextureRef(Thumbnail),
			imageMin,
			ImVec2(imageMin.x + size.x, imageMin.y + size.y));
	}

	if (previewMax.x > previewMin.x) {
		const ImVec2 textMin(min.x + Padding, previewMax.y + Padding);
		const ImVec2 textMax(max.x - Padding, max.y - Padding);

		draw->PushClipRect(textMin, textMax, true);
		ImGui::RenderTextEllipsis(
			draw,
			textMin,
			textMax,
			textMax.x,
			DisplayName.c_str(),
			DisplayName.c_str() + DisplayName.size(),
			nullptr);
		draw->PopClipRect();
	}

	DrawDragSource();
	if (hovered && !ImGui::GetDragDropPayload())
		ImGui::SetTooltip("%s", DisplayName.c_str());
	return clicked;
}

GridUI::GridUI() : EditorUI("GridUI") {}

void GridUI::DrawUI() {
	if (mNodes.empty() || ImGui::IsWindowCollapsed())
		return;

	const float availableWidth = ImGui::GetContentRegionAvail().x;
	if (availableWidth <= 0.0f)
		return;

	const ImVec2 tileSize(
		std::min(TileWidth, availableWidth),
		PreviewHeight + ImGui::GetTextLineHeight() + Padding * 3.0f);
	const int columns = std::max(
		1, static_cast<int>((availableWidth + Gap) / (tileSize.x + Gap)));

	ImGui::PushID(this);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(Gap, Gap));

	// 모든 타일에 같은 선택 상태를 적용한 뒤 클릭 결과를 반영합니다.
	const GridNode* clickedNode = nullptr;
	int column = 0;
	for (const auto& node : mNodes) {
		if (column != 0)
			ImGui::SameLine();

		if (node->Draw(tileSize, mSelectedKey == node->Key))
			clickedNode = node.Get();

		if (++column == columns)
			column = 0;
	}

	ImGui::PopStyleVar();
	ImGui::PopID();

	if (clickedNode)
		mSelectedKey = clickedNode->Key;
}

void GridUI::SetItems(std::vector<Ptr<GridNode>> items) {
	std::erase_if(items, [](const Ptr<GridNode>& node) { return node.Get() == nullptr; });
	mNodes = std::move(items);

	if (mSelectedKey && GetSelected() == nullptr)
		mSelectedKey.reset();
}

Ptr<GridNode> GridUI::GetSelected() const {
	if (!mSelectedKey)
		return nullptr;

	for (const auto& node : mNodes) {
		if (node->Key == *mSelectedKey)
			return node;
	}

	return nullptr;
}
