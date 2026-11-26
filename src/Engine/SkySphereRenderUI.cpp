#include "Engine/pch.h"
#include "Engine/SkySphereRenderUI.hpp"

#include "Engine/ListUI.hpp"

#include "Engine/AssetManager.hpp"
#include "Engine/EditorManager.hpp"

namespace {
	const ImVec2 ButtonSize = ImVec2(20.f, 20.f);
}

SkySphereRenderUI::SkySphereRenderUI() 
	: RenderComponentUI{ EComponent::E_SkySphereRender, "SkySphereRenderUI" } {}

SkySphereRenderUI::~SkySphereRenderUI() {}

void SkySphereRenderUI::DrawUI() {
	OutputTitle("SkySphereRender");

	Ptr<CSkySphereRender> meshRender = GetTarget()->SkySphereRender();

	if (ImGui::BeginTable("MeshRenderUITable"
		, 2
		, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		MeshPanel();

		ImGui::TableNextRow();
		MaterialSlotPanel();

		ImGui::TableNextRow();
		TexturePanel();

		ImGui::EndTable();
	}
}

void SkySphereRenderUI::TexturePanel() {
	Ptr<CSkySphereRender> renderComp = GetTarget()->SkySphereRender();

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Environment\nCube Map");

	ImGui::TableSetColumnIndex(1);
	{
		float avail = ImGui::GetContentRegionAvail().x;

		// 버튼 공간을 제외한 너비로 InputText의 크기를 계산합니다.
		float inputWidth = avail - ButtonSize.x - ImGui::GetStyle().ItemSpacing.x;
		if (inputWidth < 50.f) inputWidth = 50.f; // 최소 너비를 보장합니다.

		std::string cubeMapKey{};
		if (renderComp->GetEnvironmentCubeMap() != nullptr) cubeMapKey = WStrToStr(renderComp->GetEnvironmentCubeMap()->GetKey());

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##CubeMapName", cubeMapKey.data(), cubeMapKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
			if (PayLoad) {
				DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
				Ptr<Asset> pAsset = reinterpret_cast<Asset*>(data);

				if (pAsset->GetType() == EAsset::E_Texture)
					renderComp->SetEnvironmentCubeMap(dynamic_cast<ATexture*>(pAsset.Get()));
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("##CubeMapBtn", ButtonSize)) {
		Ptr<ListUI> listUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
		assert(listUI.Get());

		listUI->SetUIName("Texture List");
		std::vector<std::wstring> cubeMapNames{};
		ASSET_MANAGER->GetAssetNames(EAsset::E_Texture, cubeMapNames);

		listUI->AddString(cubeMapNames);
		listUI->AddDelegate(this, (DELEGATE_1)&SkySphereRenderUI::SelectEnvironmentCubeMap);
		listUI->SetActive(true);
	}
}

void SkySphereRenderUI::SelectEnvironmentCubeMap(DWORD_PTR ptr) {
	Ptr<ListUI> listUI = reinterpret_cast<ListUI*>(ptr);

	auto key = StrToWStr(listUI->GetSelectedString());

	Ptr<ATexture> pTexture = FIND(ATexture, key);
	GetTarget()->SkySphereRender()->SetEnvironmentCubeMap(dynamic_cast<ATexture*>(pTexture.Get()));
}