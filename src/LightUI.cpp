#include "pch.h"
#include "LightUI.hpp"

LightUI::LightUI() 
	: ComponentUI{ EComponent::E_Light, "LightUI" } {}

LightUI::~LightUI() {}

void LightUI::DrawUI() {
	OutputTitle("Light");

	auto pLight2D = GetTarget()->Light();

	auto type = pLight2D->GetLightType();

	Vec3 lightColor = pLight2D->GetLightColor();

	if (ImGui::BeginTable("LightTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		{
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Color");

			ImGui::TableSetColumnIndex(1);
			if (ImGui::ColorPicker3("##LightColor", lightColor.data()))
				pLight2D->SetLightColor(lightColor);
		}
		if (type == ELight::E_Directional) {
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Ambient");

			Vec3 ambient = pLight2D->GetAmbient();
			ImGui::TableSetColumnIndex(1);
			if (ImGui::ColorPicker3("##AmbientColor", ambient.data()))
				pLight2D->SetAmbient(ambient);
		}
		{
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Intensity");
			
			ImGui::TableSetColumnIndex(1);
			auto intensity = pLight2D->GetIntensity();
			if (ImGui::DragFloat("##LightIntensity", &intensity, 0.1f))
				pLight2D->SetIntensity(std::max(intensity, 1e-6f));
		}
		{
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted("Attenuation\nRadius");

			ImGui::TableSetColumnIndex(1);
			auto attenuationRadius = pLight2D->GetAttenuationRadius();
			if (ImGui::DragFloat("##LightAttenuationRadius", &attenuationRadius, 0.1f))
				pLight2D->SetAttenuationRadius(std::max(attenuationRadius, 0.f));
		}
		if (type == ELight::E_Point) {
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Radius");

			ImGui::TableSetColumnIndex(1);
			auto radius = pLight2D->GetRadius();
			if (ImGui::DragFloat("##LightRadius", &radius, 0.1f))
				pLight2D->SetRadius(std::max(radius, 0.f));
		}

		ImGui::EndTable();
	}

	//if (type == ELight::E_Spot) {
	//	ImGui::Text("Angle");
	//	ImGui::SameLine(100.f);
	//
	//	float angle = pLight2D->GetAngle() * RadToDeg;
	//	if (ImGui::DragFloat("##LightAngle", &angle, 0.1f))
	//		pLight2D->SetAngle(min(max(angle, 10.f), 180.f - 1e-6f) * DegToRad);
	//}
}