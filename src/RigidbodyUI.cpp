#include "pch.h"
#include "RigidbodyUI.hpp"

RigidbodyUI::RigidbodyUI() 
	: ComponentUI{ EComponent::E_Rigidbody, "RigidbodyUI"} {}

RigidbodyUI::~RigidbodyUI() {}

void RigidbodyUI::DrawUI() {
	OutputTitle("Rigidbody");

	if (ImGui::BeginTable("RigidbodyUITable"
		, 2
		, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		RigidbodyTypePanel();

		ImGui::TableNextRow();
		MassPanel();

		ImGui::TableNextRow();
		RestitutionPanel();

		ImGui::TableNextRow();
		UseGravityPanel();

		ImGui::EndTable();
	}
}

void RigidbodyUI::RigidbodyTypePanel() {
	Ptr<CRigidbody> rigidbody = GetTarget()->Rigidbody();
	if (rigidbody == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Type");

	ImGui::TableSetColumnIndex(1);
	{
		int type = static_cast<int>(rigidbody->GetRigidbodyType());
		if (ImGui::Combo("##RigidbodyType"
			, &type, ERigidbody::TypeNames
			, _countof(ERigidbody::TypeNames)))
			rigidbody->SetType(static_cast<ERigidbody::Type>(type));
	}
}

void RigidbodyUI::MassPanel() {
	Ptr<CRigidbody> rigidbody = GetTarget()->Rigidbody();
	if (rigidbody == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Mass");

	ImGui::TableSetColumnIndex(1);
	{
		float mass = rigidbody->GetMass();
		if (ImGui::DragFloat("##Mass", &mass, 0.01f, 0.f, FLT_MAX)) {
			rigidbody->SetMass(mass);
		}
	}	
}

void RigidbodyUI::RestitutionPanel() {
	Ptr<CRigidbody> rigidbody = GetTarget()->Rigidbody();
	if (rigidbody == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Restitution");

	ImGui::TableSetColumnIndex(1);
	{
		float restitution = rigidbody->GetRestitution();
		if (ImGui::DragFloat("##Restitution", &restitution, 0.01f, 0.f, 1.f)) {
			rigidbody->SetRestitution(restitution);
		}
	}
}

void RigidbodyUI::UseGravityPanel() {
	Ptr<CRigidbody> rigidbody = GetTarget()->Rigidbody();
	if (rigidbody == nullptr) return;

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Use Gravity");

	ImGui::TableSetColumnIndex(1);
	{
		auto useGravity = rigidbody->GetUseGravity();
		if (ImGui::Checkbox("##UseGravity", &useGravity))
			rigidbody->SetUseGravity(useGravity);
	}
}