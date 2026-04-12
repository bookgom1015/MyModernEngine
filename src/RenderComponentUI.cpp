#include "pch.h"
#include "RenderComponentUI.hpp"

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

RenderComponentUI::RenderComponentUI(EComponent::Type type, const std::string& name)
	: ComponentUI{ type, name }
	, mSelectedMaterialIdx{ -1 } {}

RenderComponentUI::~RenderComponentUI() {}

void RenderComponentUI::TargetChanged() {
	mSelectedMaterialIdx = -1;
}

void RenderComponentUI::MeshPanel() {
	Ptr<CRenderComponent> renderComp = GetTarget()->GetRenderComponent();
	Ptr<AMesh> mesh = renderComp->GetMesh();

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
					renderComp->SetMesh(dynamic_cast<AMesh*>(pAsset.Get()));
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("##MeshBtn", ButtonSize)) {
		auto& io = ImGui::GetIO();

		if (io.KeyAlt) {
			GetTarget()->GetRenderComponent()->SetMesh(nullptr);
		}
		else {
			Ptr<ListUI> listUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
			assert(listUI.Get());

			listUI->SetUIName("Mesh List");

			std::vector<std::wstring> meshNames{};
			ASSET_MANAGER->GetAssetNames(EAsset::E_Mesh, meshNames);

			listUI->AddString(meshNames);
			listUI->AddDelegate(this, (DELEGATE_1)&RenderComponentUI::SelectMesh);
			listUI->SetActive(true);
		}
	}
}

void RenderComponentUI::MaterialSlotPanel() {
	Ptr<CRenderComponent> renderComp = GetTarget()->GetRenderComponent();

	UINT primCount = 0;
	auto mesh = renderComp->GetMesh();
	if (mesh != nullptr) primCount = mesh->GetStaticPrimitiveCount()
		+ mesh->GetSkinnedPrimitiveCount();

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Materials");

	ImGui::TableSetColumnIndex(1);
	ImGui::BeginChild("MaterialChild", ImVec2(0, 0), ImGuiChildFlags_Borders);

	for (UINT i = 0; i < primCount; ++i) {
		Ptr<AMaterial> mat = renderComp->GetMaterial(i);

		std::string name = std::format("{}##{}", WStrToStr(mat->GetKey()), i);
		if (ImGui::Selectable(name.c_str(), mSelectedMaterialIdx == i)) {
			mSelectedMaterialIdx = i;
		}
	}

	ImGui::EndChild();
}

void RenderComponentUI::MaterialPanel() {
	if (mSelectedMaterialIdx == -1) return;

	UINT index = static_cast<UINT>(mSelectedMaterialIdx);

	Ptr<CRenderComponent> renderComp = GetTarget()->GetRenderComponent();
	Ptr<AMaterial> mat = renderComp->GetMaterial(index);

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
					renderComp->SetMaterial(index, dynamic_cast<AMaterial*>(pAsset.Get()));
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("##MtrlBtn", ButtonSize)) {
		Ptr<ListUI> listUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
		assert(listUI.Get());

		listUI->SetUIName("Material List");

		std::vector<std::wstring> matNames{};
		ASSET_MANAGER->GetAssetNames(EAsset::E_Material, matNames);

		listUI->AddString(matNames);
		listUI->AddDelegate(this, (DELEGATE_1)&RenderComponentUI::SelectMaterial);
		listUI->SetActive(true);
	}

	if (mat != nullptr) {
		TexturePanel();

		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Albedo");

			ImGui::TableSetColumnIndex(1);
			auto albedo = renderComp->GetAlbedo(index);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::ColorPicker3("##Albedo", albedo.data())) {
				renderComp->SetAlbedo(index, albedo);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Roughness");

			ImGui::TableSetColumnIndex(1);
			auto roughness = renderComp->GetRoughness(index);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::SliderFloat("##Roughness", &roughness, 0.f, 1.f)) {
				roughness = std::clamp(roughness, 0.f, 1.f);
				renderComp->SetRoughness(index, roughness);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Metalic");

			ImGui::TableSetColumnIndex(1);
			auto metalic = renderComp->GetMetalic(index);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::SliderFloat("##Metalic", &metalic, 0.f, 1.f)) {
				metalic = std::clamp(metalic, 0.f, 1.f);
				renderComp->SetMetalic(index, metalic);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Specular");

			ImGui::TableSetColumnIndex(1);
			auto specular = GetTarget()->GetRenderComponent()->GetSpecular(index);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::SliderFloat("##Specular", &specular, 0.f, 1.f)) {
				specular = std::clamp(specular, 0.f, 1.f);
				renderComp->SetSpecular(index, specular);
			}
		}
	}
}

void RenderComponentUI::TexturePanel() {
	if (mSelectedMaterialIdx == -1) return;

	UINT index = static_cast<UINT>(mSelectedMaterialIdx);

	Ptr<CRenderComponent> renderComp = GetTarget()->GetRenderComponent();
	Ptr<AMaterial> mat = renderComp->GetMaterial(index);

	// AlbedoMap
	ImGui::TableNextRow();
	{
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("Albedo Map");

		ImGui::TableSetColumnIndex(1);
		auto albedoMap = renderComp->GetAlbedoMap(index);
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
					renderComp->SetAlbedoMap(index, dynamic_cast<ATexture*>(pAsset.Get()));
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
			pUI->AddDelegate(this, (DELEGATE_1)&RenderComponentUI::SelectTexture);
			pUI->SetActive(true);
		}
	}
	// NormalMap
	ImGui::TableNextRow();
	{
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("Normal Map");

		ImGui::TableSetColumnIndex(1);
		auto normalMap = GetTarget()->GetRenderComponent()->GetNormalMap(index);
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
					renderComp->SetNormalMap(index, dynamic_cast<ATexture*>(pAsset.Get()));
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
			pUI->AddDelegate(this, (DELEGATE_1)&RenderComponentUI::SelectTexture);
			pUI->SetActive(true);
		}
	}
}

void RenderComponentUI::SelectMesh(DWORD_PTR ptr) {
	Ptr<ListUI> listUI = reinterpret_cast<ListUI*>(ptr);

	auto key = StrToWStr(listUI->GetSelectedString());

	Ptr<AMesh> pMesh = FIND(AMesh, key);

	GetTarget()->GetRenderComponent()->SetMesh(pMesh);
}

void RenderComponentUI::SelectMaterial(DWORD_PTR ptr) {
	Ptr<ListUI> listUI = reinterpret_cast<ListUI*>(ptr);
	
	auto key = StrToWStr(listUI->GetSelectedString());
	
	Ptr<AMaterial> pMtrl = FIND(AMaterial, key);
	
	GetTarget()->GetRenderComponent()->SetMaterial(static_cast<UINT>(mSelectedMaterialIdx), pMtrl);
}

void RenderComponentUI::SelectTexture(DWORD_PTR ptr) {
	Ptr<ListUI> listUI = reinterpret_cast<ListUI*>(ptr);
	
	auto key = StrToWStr(listUI->GetSelectedString());
	
	Ptr<ATexture> pTexture = FIND(ATexture, key);

	const auto index = static_cast<UINT>(mSelectedMaterialIdx);
	
	switch (gTextureType) {
		case ETexture::E_Albedo:
			GetTarget()->GetRenderComponent()->SetAlbedoMap(index, pTexture);
			break;
		case ETexture::E_Normal:
			GetTarget()->GetRenderComponent()->SetNormalMap(index, pTexture);
			break;
	}
}