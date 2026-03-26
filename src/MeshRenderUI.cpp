#include "pch.h"
#include "MeshRenderUI.hpp"

#include "ListUI.hpp"

#include "AssetManager.hpp"
#include "EditorManager.hpp"

namespace {
	const ImVec2 ButtonSize = ImVec2(20.f, 20.f);
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

		// Mesh
		{
			ImGui::TableSetColumnIndex(0);
			// Mesh Name
			ImGui::Text("Mesh");

			ImGui::TableSetColumnIndex(1);
			// Mesh Name InputText
			{
				// 현재 줄에서 남은 가용 폭
				float avail = ImGui::GetContentRegionAvail().x;

				// 버튼을 위한 공간을 빼고 InputText 폭 계산
				float inputWidth = avail - buttonSize - spacing;
				if (inputWidth < 50.f) inputWidth = 50.f; // 최소 폭 보장

				Ptr<AMesh> mesh = meshRender->GetMesh();
				std::string meshKey{};
				if (mesh != nullptr) meshKey = WStrToStr(mesh->GetKey());
				ImGui::SetNextItemWidth(inputWidth);
				ImGui::InputText("##MeshName", meshKey.data(), meshKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

				// 특정 위젯에서 드래그가 발생했고, 해당 위젯 위에 마우스가 호버링 중인지
				if (ImGui::BeginDragDropTarget()) {
					const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
					if (PayLoad) {
						DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
						Ptr<Asset> pAsset = (Asset*)data;

						if (pAsset->GetType() == EAsset::E_Mesh)
							meshRender->SetMesh((AMesh*)pAsset.Get());
					}

					ImGui::EndDragDropTarget();
				}				
			}

			ImGui::SameLine();

			// Mesh List Button
			if (ImGui::Button("##MeshBtn", ButtonSize)) {
				// 버튼이 눌리면, 리스트UI 를 찾아서 활성화 시키고, 출력시키고 싶은 문자열을 ListUI 에 등록시킨다.
				Ptr<ListUI> pUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
				assert(pUI.Get());

				pUI->SetUIName("Mesh List");

				std::vector<std::wstring> meshNames{};
				ASSET_MANAGER->GetAssetNames(EAsset::E_Mesh, meshNames);
				pUI->AddString(meshNames);
				pUI->AddDelegate(this, (DELEGATE_1)&MeshRenderUI::SelectMesh);
				pUI->SetActive(true);
			}
		}


		ImGui::TableNextRow();

		// ========
		// Material
		// ========
		{
			ImGui::TableSetColumnIndex(0);
			// Material Name
			ImGui::Text("Material");

			Ptr<AMaterial> mat = meshRender->GetMaterial();

			ImGui::TableSetColumnIndex(1);
			// Material Name InputText
			{
				// 현재 줄에서 남은 가용 폭
				float avail = ImGui::GetContentRegionAvail().x;

				// 버튼을 위한 공간을 빼고 InputText 폭 계산
				float inputWidth = avail - buttonSize - spacing;
				if (inputWidth < 50.f) inputWidth = 50.f; // 최소 폭 보장

				std::string matKey{};
				if (mat != nullptr) matKey = WStrToStr(mat->GetKey());
				ImGui::SetNextItemWidth(inputWidth);
				ImGui::InputText("##MtrlName", matKey.data(), matKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

				// 특정 위젯에서 드래그가 발생했고, 해당 위젯 위에 마우스가 호버링 중인지
				if (ImGui::BeginDragDropTarget()) {
					const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
					if (PayLoad) {
						DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
						Ptr<Asset> pAsset = (Asset*)data;

						if (pAsset->GetType() == EAsset::E_Material)
							meshRender->SetMaterial((AMaterial*)pAsset.Get());
					}

					ImGui::EndDragDropTarget();
				}
			}

			ImGui::SameLine();

			// Material List Button
			if (ImGui::Button("##MtrlBtn", ButtonSize))
			{
				// 버튼이 눌리면, 리스트UI 를 찾아서 활성화 시키고, 출력시키고 싶은 문자열을 ListUI 에 등록시킨다.
				Ptr<ListUI> pUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
				assert(pUI.Get());

				pUI->SetUIName("Material List");

				std::vector<std::wstring> matNames{};
				ASSET_MANAGER->GetAssetNames(EAsset::E_Material, matNames);
				pUI->AddString(matNames);
				pUI->AddDelegate(this, (DELEGATE_1)&MeshRenderUI::SelectMaterial);
				pUI->SetActive(true);
			}
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
		}

		ImGui::EndTable();
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