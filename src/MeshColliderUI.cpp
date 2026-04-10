#include "pch.h"
#include "MeshColliderUI.hpp"

MeshColliderUI::MeshColliderUI() 
	: ComponentUI{ EComponent::Type::E_MeshCollider, "MeshColliderUI"} {}

MeshColliderUI::~MeshColliderUI() {}

void MeshColliderUI::DrawUI() {
	OutputTitle("MeshCollider");

	if (ImGui::BeginTable("MeshColliderUITable"
		, 2
		, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		OffsetPanel();

		ImGui::TableNextRow();
		ScalePanel();

		ImGui::TableNextRow();
		TriggerPanel();

		ImGui::EndTable();
	}
}

void MeshColliderUI::OffsetPanel() {
	Ptr<CMeshCollider> meshCollider = GetTarget()->MeshCollider();
	if (meshCollider == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Offset");

	ImGui::TableSetColumnIndex(1);
	{
		auto offset = meshCollider->GetOffset();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::DragFloat3("##Offset", offset.data(), 0.01f))
			meshCollider->SetOffset(offset);
	}
}

void MeshColliderUI::ScalePanel() {
	Ptr<CMeshCollider> meshCollider = GetTarget()->MeshCollider();
	if (meshCollider == nullptr) return;
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Scale");
	ImGui::TableSetColumnIndex(1);
	{
		auto scale = meshCollider->GetScale();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::DragFloat3("##Scale", scale.data(), 0.01f))
			meshCollider->SetScale(scale);
	}
}

void MeshColliderUI::TriggerPanel() {
	Ptr<CMeshCollider> meshCollider = GetTarget()->MeshCollider();
	if (meshCollider == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Is Trigger");

	ImGui::TableSetColumnIndex(1);
	{
		auto isTrigger = meshCollider->IsTrigger();
		if (ImGui::Checkbox("##IsTrigger", &isTrigger))
			meshCollider->SetTrigger(isTrigger);
	}
}