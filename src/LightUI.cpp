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
		if (type == ELight::E_Point || type == ELight::E_Spot) {
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
			ImGui::Text("Light Radius");

			ImGui::TableSetColumnIndex(1);
			auto radius = pLight2D->GetRadius();
			if (ImGui::DragFloat("##LightRadius", &radius, 0.1f))
				pLight2D->SetRadius(std::max(radius, 0.f));
		}
		if (type == ELight::E_Spot) {
			{
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Inner Angle");

				ImGui::TableSetColumnIndex(1);
				float angle = pLight2D->GetInnerAngle();
				float outerAngle = pLight2D->GetOuterAngle();
				if (ImGui::DragFloat("##LightInnerAngle", &angle, 0.1f, 0.f, outerAngle))
					pLight2D->SetInnerAngle(angle);
			}
			{
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Outer Angle");

				ImGui::TableSetColumnIndex(1);
				float angle = pLight2D->GetOuterAngle();
				float innerAngle = pLight2D->GetInnerAngle();
				if (ImGui::DragFloat("##LightOuterAngle", &angle, 0.1f, innerAngle, 90.f))
					pLight2D->SetOuterAngle(angle);
			}
		}

		ImGui::EndTable();
	}
}