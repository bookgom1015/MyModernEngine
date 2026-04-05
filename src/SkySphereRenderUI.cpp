#include "pch.h"
#include "SkySphereRenderUI.hpp"

#include "ListUI.hpp"

#include "AssetManager.hpp"
#include "EditorManager.hpp"

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

		ImGui::EndTable();
	}
}