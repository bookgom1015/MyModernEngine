#include "pch.h"
#include "BoxColliderUI.hpp"

BoxColliderUI::BoxColliderUI()
	: ComponentUI{ EComponent::Type::E_BoxCollider, "BoxColliderUI"} {}

BoxColliderUI::~BoxColliderUI() {}	

void BoxColliderUI::DrawUI() {
	OutputTitle("BoxCollider");

	if (ImGui::BeginTable("BoxColliderUITable"
		, 2
		, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		OffsetPanel();

		ImGui::TableNextRow();
		TriggerPanel();

		ImGui::EndTable();
	}
}

void BoxColliderUI::OffsetPanel() {
	Ptr<CBoxCollider> boxCollider = GetTarget()->BoxCollider();
	if (boxCollider == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Offset");

	ImGui::TableSetColumnIndex(1);
	{
		auto offset = boxCollider->GetOffset();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::DragFloat3("##Offset", offset.data(), 0.1f)) 
			boxCollider->SetOffset(offset);
	}
}

void BoxColliderUI::TriggerPanel() {
	Ptr<CBoxCollider> boxCollider = GetTarget()->BoxCollider();
	if (boxCollider == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Is Trigger");

	ImGui::TableSetColumnIndex(1);
	{
		auto isTrigger = boxCollider->IsTrigger();
		if (ImGui::Checkbox("##IsTrigger", &isTrigger)) 
			boxCollider->SetTrigger(isTrigger);
	}
}