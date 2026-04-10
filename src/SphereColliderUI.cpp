#include "pch.h"
#include "SphereColliderUI.hpp"

SphereColliderUI::SphereColliderUI()
	: ComponentUI{ EComponent::Type::E_SphereCollider, "SphereColliderUI" } {}

SphereColliderUI::~SphereColliderUI() {}

void SphereColliderUI::DrawUI() {
	OutputTitle("SphereCollider");

	if (ImGui::BeginTable("SphereColliderUITable"
		, 2
		, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		OffsetPanel();

		ImGui::TableNextRow();
		RadiusPanel();

		ImGui::TableNextRow();
		TriggerPanel();

		ImGui::EndTable();
	}
}

void SphereColliderUI::OffsetPanel() {
	Ptr<CSphereCollider> sphereCollider = GetTarget()->SphereCollider();
	if (sphereCollider == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Offset");

	ImGui::TableSetColumnIndex(1);
	{
		auto offset = sphereCollider->GetOffset();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::DragFloat3("##Offset", offset.data(), 0.01f)) 
			sphereCollider->SetOffset(offset);
	}
}

void SphereColliderUI::RadiusPanel() {
	Ptr<CSphereCollider> sphereCollider = GetTarget()->SphereCollider();
	if (sphereCollider == nullptr) return;
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Radius");
	ImGui::TableSetColumnIndex(1);
	{
		auto radius = sphereCollider->GetRadius();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::DragFloat("##Radius", &radius, 0.01f, 0.f, FLT_MAX)) 
			sphereCollider->SetRadius(radius);
	}
}

void SphereColliderUI::TriggerPanel() {
	Ptr<CSphereCollider> sphereCollider = GetTarget()->SphereCollider();
	if (sphereCollider == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Is Trigger");

	ImGui::TableSetColumnIndex(1);
	{
		auto isTrigger = sphereCollider->IsTrigger();
		if (ImGui::Checkbox("##IsTrigger", &isTrigger))
			sphereCollider->SetTrigger(isTrigger);
	}
}