#include "pch.h"
#include "Preference.hpp"

#include "AssetManager.hpp"
#include "EditorManager.hpp"
#include RENDERER_HEADER

#include "ATexture.hpp"

#include "ListUI.hpp"

namespace {
	const ImVec2 ButtonSize = ImVec2(20.f, 20.f);
}

Preference::Preference() : EditorUI{ "Preference" } {}

Preference::~Preference() {}

void Preference::DrawUI() {
	if (ImGui::BeginTable("PreferenceTable"
		, 2
		, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		ImGui::Text("Environment");

		ImGui::TableSetColumnIndex(1);
		EnvironmentPanel();

		ImGui::EndTable();
	}
}

void Preference::EnvironmentPanel() {
	if (ImGui::BeginTable("EnvironmentTable"
		, 2
		, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		DiffuseIrradianceInput();

		ImGui::TableNextRow();
		SpecularIrradianceInput();

		ImGui::EndTable();
	}
}

void Preference::DiffuseIrradianceInput() {
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Diffuse Irradiance Map");

	ImGui::TableSetColumnIndex(1);
	{
		float avail = ImGui::GetContentRegionAvail().x;

		float inputWidth = avail - ButtonSize.x - ImGui::GetStyle().ItemSpacing.x;
		if (inputWidth < 50.f) inputWidth = 50.f;

		std::string envMapKey = WStrToStr(RENDERER->GetGlobalDiffuseIrradianceMapPath());

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##DiffuseIrradianceMap", envMapKey.data(), envMapKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
			if (PayLoad) {
				DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
				Ptr<Asset> pAsset = reinterpret_cast<Asset*>(data);

				if (pAsset->GetType() == EAsset::E_Texture)
					RENDERER->SetGlobalDiffuseIrradianceMap(pAsset->GetKey());
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("##DiffuseIrradianceMapBtn", ButtonSize)) {
		Ptr<ListUI> listUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
		assert(listUI.Get());

		listUI->SetUIName("Texture List");
		std::vector<std::wstring> cubeMapNames{};
		ASSET_MANAGER->GetAssetNames(EAsset::E_Texture, cubeMapNames);

		listUI->AddString(cubeMapNames);
		listUI->AddDelegate(this, (DELEGATE_1)&Preference::SelectDiffuseIrradianceTexture);
		listUI->SetActive(true);
	}
}

void Preference::SpecularIrradianceInput() {
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Specular Irradiance Map");

	ImGui::TableSetColumnIndex(1);
	{
		float avail = ImGui::GetContentRegionAvail().x;

		float inputWidth = avail - ButtonSize.x - ImGui::GetStyle().ItemSpacing.x;
		if (inputWidth < 50.f) inputWidth = 50.f;

		std::string envMapKey = WStrToStr(RENDERER->GetGlobalSpecularIrradianceMapPath());

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##SpecularIrradianceMap", envMapKey.data(), envMapKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
			if (PayLoad) {
				DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
				Ptr<Asset> pAsset = reinterpret_cast<Asset*>(data);

				if (pAsset->GetType() == EAsset::E_Texture)
					RENDERER->SetGlobalSpecularIrradianceMap(pAsset->GetKey());
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("##SpecularIrradianceMapBtn", ButtonSize)) {
		Ptr<ListUI> listUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
		assert(listUI.Get());

		listUI->SetUIName("Texture List");
		std::vector<std::wstring> cubeMapNames{};
		ASSET_MANAGER->GetAssetNames(EAsset::E_Texture, cubeMapNames);

		listUI->AddString(cubeMapNames);
		listUI->AddDelegate(this, (DELEGATE_1)&Preference::SelectSpecularIrradianceTexture);
		listUI->SetActive(true);
	}
}

void Preference::SelectDiffuseIrradianceTexture(DWORD_PTR ptr) {
	//Ptr<Asset> pAsset = reinterpret_cast<Asset*>(ptr);
	//if (pAsset->GetType() == EAsset::E_Texture) {
	//	renderComp->SetEnvironmentCubeMap(dynamic_cast<ATexture*>(pAsset.Get()));
	//}
}

void Preference::SelectSpecularIrradianceTexture(DWORD_PTR ptr) {
	//Ptr<Asset> pAsset = reinterpret_cast<Asset*>(ptr);
	//if (pAsset->GetType() == EAsset::E_Texture) {
	//	renderComp->SetEnvironmentCubeMap(dynamic_cast<ATexture*>(pAsset.Get()));
	//}
}