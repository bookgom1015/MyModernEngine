#include "pch.h"
#include "CapsuleColliderUI.hpp"

CapsuleColliderUI::CapsuleColliderUI() 
	: ComponentUI{ EComponent::Type::E_CapsuleCollider, "CapsuleColliderUI" } {}

CapsuleColliderUI::~CapsuleColliderUI() {}

void CapsuleColliderUI::DrawUI() {
	OutputTitle("CapsuleCollider");

	if (ImGui::BeginTable("CapsuleColliderUITable"
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

void CapsuleColliderUI::OffsetPanel() {
	Ptr<CCapsuleCollider> capsuleCollider = GetTarget()->CapsuleCollider();
	if (capsuleCollider == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Offset");

	ImGui::TableSetColumnIndex(1);
	{
		auto offset = capsuleCollider->GetOffset();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::DragFloat3("##Offset", offset.data(), 0.1f))
			capsuleCollider->SetOffset(offset);
	}
}

void CapsuleColliderUI::TriggerPanel() {
	Ptr<CCapsuleCollider> capsuleCollider = GetTarget()->CapsuleCollider();
	if (capsuleCollider == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Is Trigger");

	ImGui::TableSetColumnIndex(1);
	{
		auto isTrigger = capsuleCollider->IsTrigger();
		if (ImGui::Checkbox("##IsTrigger", &isTrigger))
			capsuleCollider->SetTrigger(isTrigger);
	}
}