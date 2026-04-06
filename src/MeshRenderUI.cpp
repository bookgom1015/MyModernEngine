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
	: RenderComponentUI{EComponent::E_MeshRender, "MeshRenderUI"} {}

MeshRenderUI::~MeshRenderUI() {}

void MeshRenderUI::DrawUI() {
	OutputTitle("MeshRender");

	Ptr<CMeshRender> meshRender = GetTarget()->MeshRender();
	
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
		MaterialPanel();

		ImGui::EndTable();
	}
}