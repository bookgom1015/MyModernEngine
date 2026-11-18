#include "Engine/pch.h"
#include "Engine/Content.hpp"

#include "Engine/AssetManager.hpp"

#include <algorithm>
#include <filesystem>
#include <utility>

namespace {
	constexpr char AssetPayloadType[] = "Content";

	class AssetGridNode : public GridNode {
	public:
		explicit AssetGridNode(const Ptr<Asset>& asset) : mAsset(asset) {}

	protected:
		void DrawDragSource() const override {
			if (ImGui::BeginDragDropSource()) {
				const auto payload = reinterpret_cast<DWORD_PTR>(mAsset.Get());
				ImGui::SetDragDropPayload(AssetPayloadType, &payload, sizeof(payload));
				ImGui::TextUnformatted(DisplayName.c_str());
				ImGui::EndDragDropSource();
			}
		}

	private:
		Ptr<Asset> mAsset;
	};
}

Content::Content() : EditorUI("Content") {
	mAssetTypeList = NEW ListUI(true);
	mAssetGrid = NEW GridUI;

	for (int i = 0; i < EAsset::Count; ++i)
		mAssetTypeList->AddString(AssetTypeToString(static_cast<EAsset::Type>(i)));

	mAssetTypeList->SetSelectedIndex(0);
	AddChildUI(mAssetTypeList.Get());
	AddChildUI(mAssetGrid.Get());
}

void Content::DrawUI() {
	if (ASSET_MANAGER->IsChanged())
		mbNeedsRenew = true;
}

void Content::DrawChildren() {
	const float height = ImGui::GetContentRegionAvail().y - 2.0f * ImGui::GetStyle().CellPadding.y;
	if (height <= 0.0f)
		return;

	constexpr ImGuiTableFlags flags =
		ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
	if (!ImGui::BeginTable("##ContentLayout", 2, flags))
		return;

	ImGui::TableSetupColumn("Asset Types", ImGuiTableColumnFlags_WidthFixed, 160.0f);
	ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableNextRow();

	if (ImGui::TableSetColumnIndex(0) && mAssetTypeList->IsActive()) {
		if (ImGui::BeginChild("##AssetTypes", ImVec2(0.0f, height)))
			mAssetTypeList->Draw();
		ImGui::EndChild();
	}

	if (mbNeedsRenew || mDisplayedAssetType != mAssetTypeList->GetSelectedIndex())
		Renew();

	if (ImGui::TableSetColumnIndex(1) && mAssetGrid->IsActive()) {
		if (ImGui::BeginChild("##Assets", ImVec2(0.0f, height)))
			mAssetGrid->Draw();
		ImGui::EndChild();
	}

	ImGui::EndTable();
}

void Content::Renew() {
	// 페이로드를 받는 UI가 에셋 주소를 사용할 때까지 현재 목록을 유지합니다.
	const auto* payload = ImGui::GetCurrentContext() ? ImGui::GetDragDropPayload() : nullptr;
	if (payload && payload->IsDataType(AssetPayloadType)) {
		mbNeedsRenew = true;
		return;
	}

	const int selectedIndex = mAssetTypeList->GetSelectedIndex();
	const auto type = static_cast<EAsset::Type>(selectedIndex);
	std::vector<std::wstring> keys;
	if (selectedIndex >= 0 && selectedIndex < EAsset::Count)
		ASSET_MANAGER->GetAssetNames(type, keys);

	std::sort(keys.begin(), keys.end());
	std::vector<Ptr<GridNode>> items;
	items.reserve(keys.size());
	const auto keyPrefix = std::to_string(selectedIndex) + ":";
	for (const auto& key : keys) {
		const auto asset = ASSET_MANAGER->FindAsset(type, key);
		if (asset == nullptr)
			continue;

		Ptr<GridNode> node = NEW AssetGridNode(asset);
		node->Key = keyPrefix + WStrToStr(key);
		node->DisplayName = WStrToStr(std::filesystem::path(key).filename().wstring());
		items.push_back(node);
	}

	// DisplayName을 기준으로 자연스러운 순서로 정렬합니다. ex) "Asset 1", "Asset 2", "Asset 10" 순으로 정렬
	std::sort(items.begin(), items.end(), [](const Ptr<GridNode>& a, const Ptr<GridNode>& b) {
		return NaturalLess(a->DisplayName, b->DisplayName);
	});

	mAssetGrid->SetItems(std::move(items));
	mDisplayedAssetType = selectedIndex;
	mbNeedsRenew = false;
}
