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
				ImGui::Text("...");
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Near Clip");
			ImGui::TableSetColumnIndex(1);
			{
				ImGui::Text("...");
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Sphere Radius");
			ImGui::TableSetColumnIndex(1);
			{
				ImGui::Text("...");
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Priority");
			ImGui::TableSetColumnIndex(1);
			{
				ImGui::Text("...");
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Blend Distance");
			ImGui::TableSetColumnIndex(1);
			{
				ImGui::Text("...");
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Enabled");
			ImGui::TableSetColumnIndex(1);
			{
				ImGui::Text("...");
			}
		}
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Use\nBox Projection");
			ImGui::TableSetColumnIndex(1);
			{
				ImGui::Text("...");
			}
		}

		ImGui::EndTable();
	}
}