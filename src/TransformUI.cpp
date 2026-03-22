#include "pch.h"
#include "TransformUI.hpp"

using namespace DirectX;

TransformUI::TransformUI() 
	: ComponentUI{ EComponent::E_Transform, "TransformUI" } {}

TransformUI::~TransformUI() {}

void TransformUI::DrawUI() {
	OutputTitle("Transform");

	Vec3 pos = GetTarget()->Transform()->GetRelativePosition();
	Vec3 scale = GetTarget()->Transform()->GetRelativeScale();
	Vec3 rot = GetTarget()->Transform()->GetRelativeRotation();

	ImGui::Text("Position");
	ImGui::SameLine(100);
	if (ImGui::DragFloat3("##POSITION", pos.data()))
		GetTarget()->Transform()->SetRelativePosition(pos);

	ImGui::Text("Scale");
	ImGui::SameLine(100);
	if (ImGui::DragFloat3("##SCALE", scale.data()))
		GetTarget()->Transform()->SetRelativeScale(scale);

	ImGui::Text("Rotation");
	ImGui::SameLine(100);
	Vec3 vDegree = rot * RadToDeg;
	if (ImGui::DragFloat3("##ROTATION", vDegree.data())) {
		rot = vDegree * (XM_PI / 180.f);
		GetTarget()->Transform()->SetRelativeRotation(rot);
	}

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::Text("Independent Scale");
	ImGui::SameLine();

	//bool Independent = GetTarget()->Transform()->IsIndependentScale();
	//if (ImGui::Checkbox("##Independent", &Independent))
	//	GetTarget()->Transform()->SetIndependentScale(Independent);
}