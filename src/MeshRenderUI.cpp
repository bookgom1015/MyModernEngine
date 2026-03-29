#include "pch.h"
#include "MeshRenderUI.hpp"

#include "ListUI.hpp"

#include "AssetManager.hpp"
#include "EditorManager.hpp"

namespace {
	const ImVec2 ButtonSize = ImVec2(20.f, 20.f);

	namespace ETexture {
		enum Type {
			E_None,
			E_Albedo,
			E_Normal
		};
	};
	ETexture::Type gTextureType = ETexture::E_None;
}

MeshRenderUI::MeshRenderUI() 
	: ComponentUI{EComponent::E_MeshRender, "MeshRenderUI"} {}

MeshRenderUI::~MeshRenderUI() {}

void MeshRenderUI::DrawUI() {
	OutputTitle("MeshRender");

	Ptr<CMeshRender> meshRender = GetTarget()->MeshRender();
	
	float buttonSize = ButtonSize.x;
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	
	if (ImGui::BeginTable("MeshRenderUITable"
		, 2
		, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		MeshPanel();

		ImGui::TableNextRow();
		MaterialPanel();

		ImGui::EndTable();
	}
}

void MeshRenderUI::MeshPanel() {
	Ptr<CMeshRender> meshRender = GetTarget()->MeshRender();
	Ptr<AMesh> mesh = meshRender->GetMesh();

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Mesh");

	ImGui::TableSetColumnIndex(1);
	{
		float avail = ImGui::GetContentRegionAvail().x;
		
		// 버튼을 위한 공간을 빼고 InputText 폭 계산
		float inputWidth = avail - ButtonSize.x - ImGui::GetStyle().ItemSpacing.x;
		if (inputWidth < 50.f) inputWidth = 50.f; // 최소 폭 보장

		std::string meshKey{};
		if (mesh != nullptr) meshKey = WStrToStr(mesh->GetKey());

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##MeshName", meshKey.data(), meshKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
			if (PayLoad) {
				DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
				Ptr<Asset> pAsset = reinterpret_cast<Asset*>(data);

				if (pAsset->GetType() == EAsset::E_Mesh)
					meshRender->SetMesh(dynamic_cast<AMesh*>(pAsset.Get()));
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("##MeshBtn", ButtonSize)) {
		Ptr<ListUI> listUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
		assert(listUI.Get());

		listUI->SetUIName("Mesh List");

		std::vector<std::wstring> meshNames{};
		ASSET_MANAGER->GetAssetNames(EAsset::E_Mesh, meshNames);

		listUI->AddString(meshNames);
		listUI->AddDelegate(this, (DELEGATE_1)&MeshRenderUI::SelectMesh);
		listUI->SetActive(true);
	}
}

void MeshRenderUI::MaterialPanel() {
	Ptr<CMeshRender> meshRender = GetTarget()->MeshRender();
	Ptr<AMaterial> mat = meshRender->GetMaterial();

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Material");

	ImGui::TableSetColumnIndex(1);
	{
		float avail = ImGui::GetContentRegionAvail().x;

		float inputWidth = avail - ButtonSize.x - ImGui::GetStyle().ItemSpacing.x;
		if (inputWidth < 50.f) inputWidth = 50.f;

		std::string matKey{};
		if (mat != nullptr) matKey = WStrToStr(mat->GetKey());

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##MtrlName", matKey.data(), matKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
			if (PayLoad) {
				DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
				Ptr<Asset> pAsset = reinterpret_cast<Asset*>(data);

				if (pAsset->GetType() == EAsset::E_Material)
					meshRender->SetMaterial(dynamic_cast<AMaterial*>(pAsset.Get()));
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("##MtrlBtn", ButtonSize))	{
		Ptr<ListUI> listUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
		assert(listUI.Get());

		listUI->SetUIName("Material List");

		std::vector<std::wstring> matNames{};
		ASSET_MANAGER->GetAssetNames(EAsset::E_Material, matNames);

		listUI->AddString(matNames);
		listUI->AddDelegate(this, (DELEGATE_1)&MeshRenderUI::SelectMaterial);
		listUI->SetActive(true);
	}

	if (mat != nullptr) {
		TexturePanel();

		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Albedo");

			ImGui::TableSetColumnIndex(1);
			auto albedo = GetTarget()->MeshRender()->GetAlbedo();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::ColorPicker3("##Albedo", albedo.data())) {
				GetTarget()->MeshRender()->SetAlbedo(albedo);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Roughness");

			ImGui::TableSetColumnIndex(1);
			auto roughness = GetTarget()->MeshRender()->GetRoughness();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::SliderFloat("##Roughness", &roughness, 0.f, 1.f)) {
				roughness = std::clamp(roughness, 0.f, 1.f);
				GetTarget()->MeshRender()->SetRoughness(roughness);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Metalic");

			ImGui::TableSetColumnIndex(1);
			auto metalic = GetTarget()->MeshRender()->GetMetalic();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::SliderFloat("##Metalic", &metalic, 0.f, 1.f)) {
				metalic = std::clamp(metalic, 0.f, 1.f);
				GetTarget()->MeshRender()->SetMetalic(metalic);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Specular");

			ImGui::TableSetColumnIndex(1);
			auto specular = GetTarget()->MeshRender()->GetSpecular();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::SliderFloat("##Specular", &specular, 0.f, 1.f)) {
				specular = std::clamp(specular, 0.f, 1.f);
				GetTarget()->MeshRender()->SetSpecular(specular);
			}
		}
	}
}

void MeshRenderUI::TexturePanel() {
	Ptr<CMeshRender> meshRender = GetTarget()->MeshRender();
	Ptr<AMaterial> mat = meshRender->GetMaterial();

	// AlbedoMap
	ImGui::TableNextRow();
	{
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("Albedo Map");

		ImGui::TableSetColumnIndex(1);
		auto albedoMap = GetTarget()->MeshRender()->GetAlbedoMap();
		std::string albedoMapKey{};
		if (albedoMap != nullptr) albedoMapKey = WStrToStr(albedoMap->GetKey());

		float avail = ImGui::GetContentRegionAvail().x;

		float inputWidth = avail - ButtonSize.x - ImGui::GetStyle().ItemSpacing.x;
		if (inputWidth < 50.f) inputWidth = 50.f;

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText(
			"##Albedo Map",
			albedoMapKey.data(),
			albedoMapKey.length() + 1,
			ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
			if (PayLoad) {
				DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
				Ptr<Asset> pAsset = reinterpret_cast<Asset*>(data);

				if (pAsset->GetType() == EAsset::E_Texture)
					meshRender->SetAlbedoMap(dynamic_cast<ATexture*>(pAsset.Get()));
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		// Texture List Button
		if (ImGui::Button("##AlbedoMapBtn", ButtonSize)) {
			Ptr<ListUI> pUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
			assert(pUI.Get());

			pUI->SetUIName("Texture List");

			std::vector<std::wstring> texNames{};
			ASSET_MANAGER->GetAssetNames(EAsset::E_Texture, texNames);

			gTextureType = ETexture::E_Albedo;

			pUI->AddString(texNames);
			pUI->AddDelegate(this, (DELEGATE_1)&MeshRenderUI::SelectTexture);
			pUI->SetActive(true);
		}
	}
	// NormalMap
	ImGui::TableNextRow();
	{
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("Normal Map");

		ImGui::TableSetColumnIndex(1);
		auto normalMap = GetTarget()->MeshRender()->GetNormalMap();
		std::string normalMapKey{};
		if (normalMap != nullptr) normalMapKey = WStrToStr(normalMap->GetKey());

		float avail = ImGui::GetContentRegionAvail().x;

		float inputWidth = avail - ButtonSize.x - ImGui::GetStyle().ItemSpacing.x;
		if (inputWidth < 50.f) inputWidth = 50.f;

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText(
			"##Normal Map",
			normalMapKey.data(),
			normalMapKey.length() + 1,
			ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
			if (PayLoad) {
				DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
				Ptr<Asset> pAsset = reinterpret_cast<Asset*>(data);

				if (pAsset->GetType() == EAsset::E_Texture)
					meshRender->SetNormalMap(dynamic_cast<ATexture*>(pAsset.Get()));
			}

			ImGui::EndDragDropTarget();
		}

		ImGui::SameLine();

		// Texture List Button
		if (ImGui::Button("##NormalMapBtn", ButtonSize)) {
			Ptr<ListUI> pUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
			assert(pUI.Get());

			pUI->SetUIName("Texture List");

			std::vector<std::wstring> texNames{};
			ASSET_MANAGER->GetAssetNames(EAsset::E_Texture, texNames);

			gTextureType = ETexture::E_Normal;

			pUI->AddString(texNames);
			pUI->AddDelegate(this, (DELEGATE_1)&MeshRenderUI::SelectTexture);
			pUI->SetActive(true);
		}
	}
}

void MeshRenderUI::SelectMesh(DWORD_PTR ptr) {
	Ptr<ListUI> listUI = reinterpret_cast<ListUI*>(ptr);

	auto key = StrToWStr(listUI->GetSelectedString());

	Ptr<AMesh> pMesh = FIND(AMesh, key);

	GetTarget()->MeshRender()->SetMesh(pMesh);
}

void MeshRenderUI::SelectMaterial(DWORD_PTR ptr) {
	Ptr<ListUI> listUI = reinterpret_cast<ListUI*>(ptr);

	auto key = StrToWStr(listUI->GetSelectedString());

	Ptr<AMaterial> pMtrl = FIND(AMaterial, key);

	GetTarget()->MeshRender()->SetMaterial(pMtrl);
}

void MeshRenderUI::SelectTexture(DWORD_PTR ptr) {
	Ptr<ListUI> listUI = reinterpret_cast<ListUI*>(ptr);

	auto key = StrToWStr(listUI->GetSelectedString());

	Ptr<ATexture> pTexture = FIND(ATexture, key);

	switch (gTextureType) {
		case ETexture::E_Albedo:
			GetTarget()->MeshRender()->SetAlbedoMap(pTexture);
			break;
		case ETexture::E_Normal:
			GetTarget()->MeshRender()->SetNormalMap(pTexture);
			break;
	}
}