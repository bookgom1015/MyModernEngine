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
		RadiusPanel();

		ImGui::TableNextRow();
		HalfSegmentPanel();

		ImGui::TableNextRow();
		TriggerPanel();

		ImGui::EndTable();
	}
}

void CapsuleColliderUI::RadiusPanel() {
	Ptr<CCapsuleCollider> capsuleCollider = GetTarget()->CapsuleCollider();
	if (capsuleCollider == nullptr) return;
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Radius");
	ImGui::TableSetColumnIndex(1);
	{
		auto radius = capsuleCollider->GetRadius();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::DragFloat("##Radius", &radius, 0.01f, 0.f, FLT_MAX)) 
			capsuleCollider->SetRadius(radius);
	}
}

void CapsuleColliderUI::HalfSegmentPanel() {
	Ptr<CCapsuleCollider> capsuleCollider = GetTarget()->CapsuleCollider();
	if (capsuleCollider == nullptr) return;
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Half Segment");
	ImGui::TableSetColumnIndex(1);
	{
		auto halfSegment = capsuleCollider->GetHalfSegment();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::DragFloat("##HalfSegment", &halfSegment, 0.01f, 0.f, FLT_MAX)) 
			capsuleCollider->SetHalfSegment(halfSegment);
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
		if (ImGui::DragFloat3("##Offset", offset.data(), 0.01f))
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