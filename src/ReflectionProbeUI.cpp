#include "pch.h"
#include "ReflectionProbeUI.hpp"

ReflectionProbeUI::ReflectionProbeUI() 
	: ComponentUI{ EComponent::E_ReflectionProbe, "ReflectionProbeUI" } {}

ReflectionProbeUI::~ReflectionProbeUI() {}

void ReflectionProbeUI::DrawUI() {
	OutputTitle("ReflectionProbe");

	if (ImGui::BeginTable("ReflectionProbeTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		auto probe = GetTarget()->ReflectionProbe();
		auto desc = probe->GetReflectionProbeDesc();

		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Probe Type");
			ImGui::TableSetColumnIndex(1);
			{
				ImGui::Text("...");
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Box Extents");
			ImGui::TableSetColumnIndex(1);
			{
				auto extents = desc.BoxExtents;
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::DragFloat3("##BoxExtents", extents.data(), 0.01f, 0.f, FLT_MAX)) 
					probe->SetBoxExtents(extents);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Sphere Radius");
			ImGui::TableSetColumnIndex(1);
			{
				auto radius = desc.Radius;
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::DragFloat("##SphereRadius", &radius, 0.01f, 0.f, FLT_MAX))
					probe->SetRadius(radius);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Priority");
			ImGui::TableSetColumnIndex(1);
			{
				auto priority = desc.Priority;
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::SliderInt("##Priority", &priority, 0, 31))
					probe->SetPriority(priority);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Blend Distance");
			ImGui::TableSetColumnIndex(1);
			{
				auto blendDistance = desc.BlendDistance;
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::DragFloat("##BlendDistance", &blendDistance, 0.01f, 0.f, FLT_MAX))
					probe->SetBlendDistance(blendDistance);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Enabled");
			ImGui::TableSetColumnIndex(1);
			{
				auto enabled = desc.Enabled;
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::Checkbox("##Enabled", &enabled))
					probe->SetEnabled(enabled);
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Use\nBox Projection");
			ImGui::TableSetColumnIndex(1);
			{
				auto useBoxProjection = desc.UseBoxProjection;
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::Checkbox("##UseBoxProjection", &useBoxProjection))
					probe->SetUseBoxProjection(useBoxProjection);
			}
		}

		ImGui::EndTable();
	}
}