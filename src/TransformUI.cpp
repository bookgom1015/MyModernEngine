#include "pch.h"
#include "TransformUI.hpp"

#include "EditorManager.hpp"

using namespace DirectX;

TransformUI::TransformUI() 
	: ComponentUI{ EComponent::E_Transform, "TransformUI" } {}

TransformUI::~TransformUI() {}

void TransformUI::DrawUI() {
	OutputTitle("Transform");

	Vec3 pos = GetTarget()->Transform()->GetRelativePosition();
	Vec3 scale = GetTarget()->Transform()->GetRelativeScale();
	Vec3 rot = GetTarget()->Transform()->GetRelativeRotation();

	if (ImGui::BeginTable("TransformUITable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		{
			ImGui::TableNextRow();

			//

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Position");

			//

			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat3("##POSITION", pos.data()))
				GetTarget()->Transform()->SetRelativePosition(pos);
		}
		{
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Scale");

			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::DragFloat3("##SCALE", scale.data())) {
				scale.x = std::max(scale.x, 0.f);
				scale.y = std::max(scale.y, 0.f);
				scale.z = std::max(scale.z, 0.f);
				GetTarget()->Transform()->SetRelativeScale(scale);
			}
		}
		{
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Rotation");

			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			Vec3 degree = rot * RadToDeg;
			if (ImGui::DragFloat3("##ROTATION", degree.data())) {
				rot = degree * DegToRad;
				GetTarget()->Transform()->SetRelativeRotation(rot);
			}
		}

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Dummy(ImVec2(0.f, 20.f));

		{
			ImGui::TableNextRow();

			{
				ImGui::TableSetColumnIndex(0);

				ImGui::Text("Dependency");
			}
			{
				ImGui::TableSetColumnIndex(1);

				if (ImGui::BeginTable("IndependentScaleTable", 3,
					ImGuiTableFlags_SizingStretchSame))
				{
					ImGui::TableSetupColumn("Scale");
					ImGui::TableSetupColumn("Rotation");
					ImGui::TableSetupColumn("Translation");

					// 헤더 행
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted("Scale");
					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted("Rotation");
					ImGui::TableSetColumnIndex(2);
					ImGui::TextUnformatted("Translation");

					// 체크박스 행
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					bool bScaleDependency{};
					ImGui::Checkbox("##Scale", &bScaleDependency);

					ImGui::TableSetColumnIndex(1);
					bool bRotationDependency{};
					ImGui::Checkbox("##Rotation", &bRotationDependency);

					ImGui::TableSetColumnIndex(2);
					bool bTranslationDependency{};
					ImGui::Checkbox("##Translation", &bTranslationDependency);

					ImGui::EndTable();
				}
			}
		}

		ImGui::EndTable();
	}
}