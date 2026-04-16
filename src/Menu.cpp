#include "pch.h"
#include "Menu.hpp"

#include "Engine.hpp"
#include RENDERER_HEADER

#include "EditorManager.hpp"
#include "LevelManager.hpp"
#include "AssetManager.hpp"
#include "ShaderArgumentManager.hpp"

#include "ALevel.hpp"

#include "CTransform.hpp"
#include "CSkySphereRender.hpp"
#include "CReflectionProbe.hpp"

#include "Preference.hpp"

#ifndef YOU_MUST_HAVE_CURR_LEVEL
#define YOU_MUST_HAVE_CURR_LEVEL(__func, __msg)             \
if (LEVEL_MANAGER->GetCurrentLevel() != nullptr) { __func } \
else {                                                      \
    WARNING_SOUND                                           \
    LOG_WARNING(__msg);                                     \
}                                    
#endif // YOU_MUST_HAVE_CURR_LEVEL

namespace {
	struct WindowDragState {
		bool Dragging;
		POINT StartMouseScreen;
		RECT StartWindowRect;
	};

	static WindowDragState gDragState;

	const CHAR* const STR_WARN_NO_CURR_LEVEL = "No level is currently loaded. Please create or load a level before creating game objects.";

	const float ITEM_WIDTH = 240.f;
}

Menu::Menu() : EditorUI("Menu") {}

Menu::~Menu() {}

void Menu::Draw() {
	if (ImGui::BeginMainMenuBar()) {
		FileMenu();
		ViewMenu();
		GameObjectMenu();
		LightMenu();
		AssetMenu();
		RenderMenu();
		ProjectMenu();

		DragBar();

		CloseButton();

		ImGui::EndMainMenuBar();
	}
}

void Menu::DrawUI() {

}

void Menu::FileMenu() {
	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("New Level")) {
			auto level = NEW ALevel;
			level->SetName(L"New Level");
			
			ChangeNewLevel(level);
		}

		if (ImGui::MenuItem("Save Level")) {
			Ptr<ALevel> level = LEVEL_MANAGER->GetCurrentLevel();
			if (level != nullptr) 
				if (LEVEL_MANAGER->GetCurrentLevelState() == ELevelState::E_Playing) {
					LOG_WARNING("Cannot save the level while it is playing. Please stop the level before saving.");
				}
				else {
					level->Save(format(L"{}Level\\{}.lv", CONTENT_PATH, level->GetName()));
				}
		}

		if (ImGui::MenuItem("Exit"))
			::PostMessage(Engine::GetInstance()->GetMainWndHandle(), WM_CLOSE, 0, 0);

		ImGui::EndMenu();
	}
}

void Menu::ViewMenu() {
	if (ImGui::BeginMenu("View")) {
		Ptr<EditorUI> pScene = EditorManager::GetInstance()->FindUI("Scene");
		bool SceneActive = pScene->IsActive();
		if (ImGui::MenuItem("Scene", nullptr, &SceneActive))
			pScene->SetActive(SceneActive);

		Ptr<EditorUI> pInspector = EditorManager::GetInstance()->FindUI("Inspector");
		bool InspectorActive = pInspector->IsActive();
		if (ImGui::MenuItem("Inspector", nullptr, &InspectorActive))
			pInspector->SetActive(InspectorActive);

		Ptr<EditorUI> pOutliner = EditorManager::GetInstance()->FindUI("Outliner");
		bool OutlinerActive = pOutliner->IsActive();
		if (ImGui::MenuItem("Outliner", nullptr, &OutlinerActive))
			pOutliner->SetActive(OutlinerActive);

		Ptr<EditorUI> pContent = EditorManager::GetInstance()->FindUI("Content");
		bool ContentActive = pContent->IsActive();
		if (ImGui::MenuItem("Content", nullptr, &ContentActive))
			pContent->SetActive(ContentActive);

		Ptr<EditorUI> pLog = EditorManager::GetInstance()->FindUI("Log");
		bool LogActive = pLog->IsActive();
		if (ImGui::MenuItem("Log", nullptr, &LogActive))
			pLog->SetActive(LogActive);

		ImGui::EndMenu();
	}
}

void Menu::GameObjectMenu() {
	if (ImGui::BeginMenu("GameObject")) {
		if (ImGui::MenuItem("Create Empty")) {			
			YOU_MUST_HAVE_CURR_LEVEL({
				auto obj = NEW GameObject;

				obj->AddComponent(NEW CTransform);

				unsigned i = 1;
				std::wstring name{};
				while (true) {
					name = std::format(L"New GameObject {}", i++);

					auto found = LEVEL_MANAGER->FindObjectByName(name);
					if (found == nullptr) break;
				}

				obj->SetName(name);
				CreateGameObject(obj, ELevelLayer::E_Default);
			}, STR_WARN_NO_CURR_LEVEL);
		}
		if (ImGui::MenuItem("Create Sky Sphere")) {
			YOU_MUST_HAVE_CURR_LEVEL({
				auto obj = NEW GameObject;

				obj->AddComponent(NEW CTransform);

				auto skySphereRender = NEW CSkySphereRender;
				skySphereRender->SetMesh(LOAD(AMesh, L"Sphere"));
				skySphereRender->SetMaterial(0, LOAD(AMaterial, L"Sky Sphere Material"));
				obj->AddComponent(skySphereRender);

				obj->Transform()->SetRelativeScale(Vec3(1000.f));

				unsigned i = 1;
				std::wstring name{};
				while (true) {
					name = std::format(L"New Sky Sphere {}", i++);

					auto found = LEVEL_MANAGER->FindObjectByName(name);
					if (found == nullptr) break;
				}

				obj->SetName(name);
				CreateGameObject(obj, ELevelLayer::E_Default);
			}, STR_WARN_NO_CURR_LEVEL);
		}
		if (ImGui::MenuItem("Create Reflection Probe")) {
			YOU_MUST_HAVE_CURR_LEVEL({
				auto obj = NEW GameObject;

				obj->AddComponent(NEW CTransform);

				auto reflectionProbe = NEW CReflectionProbe;
				obj->AddComponent(reflectionProbe);

				unsigned i = 1;
				std::wstring name{};
				while (true) {
					name = std::format(L"New Reflection Probe {}", i++);

					auto found = LEVEL_MANAGER->FindObjectByName(name);
					if (found == nullptr) break;
				}

				obj->SetName(name);
				CreateGameObject(obj, ELevelLayer::E_ReflectionProbe);
			}, STR_WARN_NO_CURR_LEVEL);
		}

		ImGui::EndMenu();
	}
}

void Menu::LightMenu() {
	if (ImGui::BeginMenu("Light")) {
		if (ImGui::MenuItem("Create Point Light")) {
			YOU_MUST_HAVE_CURR_LEVEL({
				auto obj = NEW GameObject;
				obj->AddComponent(NEW CTransform);

				auto light = NEW CLight;
				light->SetLightType(ELight::E_Point);
				light->SetLightColor(Vec3(1.f));
				light->SetRadius(1.f);
				light->SetAttenuationRadius(10.f);
				light->SetIntensity(1.f);
				obj->AddComponent(light);

				unsigned i = 1;
				std::wstring name{};
				while (true) {
					name = std::format(L"Point Light {}", i++);

					auto found = LEVEL_MANAGER->FindObjectByName(name);
					if (found == nullptr) break;
				}

				obj->SetName(name);

				CreateGameObject(obj, ELevelLayer::E_Light);
			}, STR_WARN_NO_CURR_LEVEL);
		}
		if (ImGui::MenuItem("Create Spot Light")) {
			YOU_MUST_HAVE_CURR_LEVEL({
				auto obj = NEW GameObject;
				obj->AddComponent(NEW CTransform);

				auto light = NEW CLight;
				light->SetLightType(ELight::E_Spot);
				light->SetLightColor(Vec3(1.f));
				light->SetAttenuationRadius(10.f);
				light->SetIntensity(1.f);
				light->SetLightDirection(Vec3(0.f, -1.f, 0.f));
				light->SetOuterAngle(90.f);
				light->SetInnerAngle(30.f);
				obj->AddComponent(light);

				unsigned i = 1;
				std::wstring name{};
				while (true) {
					name = std::format(L"Spot Light {}", i++);

					auto found = LEVEL_MANAGER->FindObjectByName(name);
					if (found == nullptr) break;
				}

				obj->SetName(name);

				CreateGameObject(obj, ELevelLayer::E_Light);
			}, STR_WARN_NO_CURR_LEVEL);
		}
		if (ImGui::MenuItem("Create Directional Light")) {
			YOU_MUST_HAVE_CURR_LEVEL({
				auto obj = NEW GameObject;
				obj->AddComponent(NEW CTransform);

				auto light = NEW CLight;
				light->SetLightType(ELight::E_Directional);
				light->SetLightColor(Vec3(1.f));
				light->SetIntensity(1.f);
				light->SetLightDirection(Vec3(0.f, -1.f, 0.f));
				light->SetAmbient(Vec3(0.05f));
				obj->AddComponent(light);

				unsigned i = 1;
				std::wstring name{};
				while (true) {
					name = std::format(L"Directional Light {}", i++);

					auto found = LEVEL_MANAGER->FindObjectByName(name);
					if (found == nullptr) break;
				}

				obj->SetName(name);

				CreateGameObject(obj, ELevelLayer::E_Light);
			}, STR_WARN_NO_CURR_LEVEL);
		}

		ImGui::EndMenu();
	}
}

void Menu::AssetMenu() {
	if (ImGui::BeginMenu("Asset")) {
		ImGui::EndMenu();
	}
}

void Menu::RenderMenu() {
	if (ImGui::BeginMenu("Render")) {
		ImGui::MenuItem("Raytracing Enabled", NULL, &SHADER_ARGUMENT_MANAGER->RaytracingEnabled);

		ImGui::Dummy(ImVec2(0.f, 2.f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.f, 2.f));

		PostProcessMenu();
		EnvironmentMenu();

		ImGui::EndMenu();
	}
}

void Menu::PostProcessMenu() {
	if (ImGui::BeginMenu("Post Process")) {
		if (ImGui::BeginMenu("Tonemapping")) {
			if (ImGui::BeginTable("TonemappingTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Type");

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(ITEM_WIDTH);
				if (ImGui::Combo(
					"##Type",
					reinterpret_cast<int*>(&SHADER_ARGUMENT_MANAGER->ToneMapping.Type),
					SHADER_ARGUMENT_MANAGER->ToneMapping.TypeNames,
					SHADER_ARGUMENT_MANAGER->ToneMapping.MaxType)) {
				}

				ImGui::EndTable();
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Gamma Correction")) {
			ImGui::MenuItem("Enabled", NULL, &SHADER_ARGUMENT_MANAGER->GammaCorrection.Enabled);

			ImGui::Dummy(ImVec2(0.f, 2.f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.f, 2.f));

			if (ImGui::BeginTable("GammaCorrectionTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Gamma");

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(ITEM_WIDTH);
				ImGui::SliderFloat(
					"##Gamma"
					, &SHADER_ARGUMENT_MANAGER->GammaCorrection.Gamma
					, SHADER_ARGUMENT_MANAGER->GammaCorrection.MinGamma
					, SHADER_ARGUMENT_MANAGER->GammaCorrection.MaxGamma);

				ImGui::EndTable();
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("TAA")) {
			ImGui::MenuItem("Enabled", NULL, &SHADER_ARGUMENT_MANAGER->TAA.Enabled);

			ImGui::Dummy(ImVec2(0.f, 2.f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.f, 2.f));

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Bloom")) {
			ImGui::MenuItem("Enabled", NULL, &SHADER_ARGUMENT_MANAGER->Bloom.Enabled);

			ImGui::Dummy(ImVec2(0.f, 2.f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.f, 2.f));

			if (ImGui::BeginTable("BloomTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Sharpness");

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(ITEM_WIDTH);
				ImGui::SliderFloat(
					"##Sharpness"
					, &SHADER_ARGUMENT_MANAGER->Bloom.Sharpness
					, SHADER_ARGUMENT_MANAGER->Bloom.MinSharpness
					, SHADER_ARGUMENT_MANAGER->Bloom.MaxSharpness);

				ImGui::EndTable();
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Vignette")) {
			ImGui::MenuItem("Enabled", NULL, &SHADER_ARGUMENT_MANAGER->Vignette.Enabled);

			ImGui::Dummy(ImVec2(0.f, 2.f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.f, 2.f));

			if (ImGui::BeginTable("VignetteTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Strength");
				
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(ITEM_WIDTH);
				ImGui::SliderFloat(
					"##Strength"
					, &SHADER_ARGUMENT_MANAGER->Vignette.Strength
					, SHADER_ARGUMENT_MANAGER->Vignette.MinStrength
					, SHADER_ARGUMENT_MANAGER->Vignette.MaxStrength);

				ImGui::EndTable();
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Volumetric Light")) {
			ImGui::MenuItem("Enabled", NULL, &SHADER_ARGUMENT_MANAGER->VolumetricLight.Enabled);
			ImGui::MenuItem(
				"Tricubic Sampling",
				NULL,
				&SHADER_ARGUMENT_MANAGER->VolumetricLight.TricubicSamplingEnabled);

			ImGui::Dummy(ImVec2(0.f, 2.f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.f, 2.f));

			if (ImGui::BeginTable("VolumetricLightTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Anisotropic Coefficient");

					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(ITEM_WIDTH);
					ImGui::SliderFloat(
						"##AnisotropicCoefficient"
						, &SHADER_ARGUMENT_MANAGER->VolumetricLight.AnisotropicCoefficient
						, SHADER_ARGUMENT_MANAGER->VolumetricLight.MinAnisotropicCoefficient
						, SHADER_ARGUMENT_MANAGER->VolumetricLight.MaxAnisotropicCoefficient);
				}
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Uniform Density");

					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(ITEM_WIDTH);
					ImGui::SliderFloat(
						"##UniformDensity"
						, &SHADER_ARGUMENT_MANAGER->VolumetricLight.UniformDensity
						, SHADER_ARGUMENT_MANAGER->VolumetricLight.MinUniformDensity
						, SHADER_ARGUMENT_MANAGER->VolumetricLight.MaxUniformDensity);
				}
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Density Scale");
					
					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(ITEM_WIDTH);
					ImGui::SliderFloat(
						"##DensityScale"
						, &SHADER_ARGUMENT_MANAGER->VolumetricLight.DensityScale
						, SHADER_ARGUMENT_MANAGER->VolumetricLight.MinDensityScale
						, SHADER_ARGUMENT_MANAGER->VolumetricLight.MaxDensityScale);
				}

				ImGui::EndTable();
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("AO")) {
			ImGui::MenuItem("Enabled", NULL, &SHADER_ARGUMENT_MANAGER->AOEnabled);

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}
}

void Menu::EnvironmentMenu() {
	if (ImGui::BeginMenu("Environment")) {
		if (ImGui::MenuItem("Bake Reflection Probes")) {
			auto currLevel = LEVEL_MANAGER->GetCurrentLevel();
			if (currLevel != nullptr) {
				auto status = RENDERER->BakeReflectionProbes();
				if (status) LOG_INFO("Reflection probes baked successfully.");
				else LOG_ERROR("Failed to bake reflection probes.");
			}
			else {
				LOG_WARNING("No level is currently loaded. Please create or load a level before baking reflection probes.");
			}
		}

		ImGui::EndMenu();
	}
}

void Menu::ProjectMenu() {
	if (ImGui::BeginMenu("Project")) {
		if (ImGui::MenuItem("Preference")) {
			Ptr<Preference> preference = dynamic_cast<Preference*>(EDITOR_MANAGER->FindUI("Preference").Get());
			assert(preference.Get());

			preference->SetUIName("Preference");
			preference->SetActive(true);
		}

		ImGui::EndMenu();
	}
}

void Menu::DragBar() {
	bool emptySpaceHovered = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered();

	auto hWnd = Engine::GetInstance()->GetMainWndHandle();

	// 드래그 시작
	if (!gDragState.Dragging 
		&& emptySpaceHovered 
		&& ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		gDragState.Dragging = true;

		::GetCursorPos(&gDragState.StartMouseScreen);
		::GetWindowRect(hWnd, &gDragState.StartWindowRect);
	}

	// 드래그 중
	if (gDragState.Dragging) {
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			POINT curMouse{};
			::GetCursorPos(&curMouse);

			int dx = curMouse.x - gDragState.StartMouseScreen.x;
			int dy = curMouse.y - gDragState.StartMouseScreen.y;

			int newX = gDragState.StartWindowRect.left + dx;
			int newY = gDragState.StartWindowRect.top + dy;

			::SetWindowPos(
				hWnd,
				nullptr,
				newX, newY,
				0, 0,
				SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		else {
			gDragState.Dragging = false;
		}
	}
}

void Menu::CloseButton() {
	// 메뉴바 높이(버튼 높이)
	const float bar_h = ImGui::GetFrameHeight();

	// 아이콘처럼 보이게: "X" 버튼 폭은 높이 정도로 맞추면 자연스러움
	const float btn_w = bar_h;

	// 현재 메뉴바 윈도우의 오른쪽 끝으로 커서를 이동
	float right = ImGui::GetWindowWidth();
	ImGui::SetCursorPosX(right - btn_w);

	// 스타일 조금: 패딩 줄여서 꽉 찬 느낌
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

	// (선택) hover 시 색 강조를 원하면 PushStyleColor로 커스텀 가능

	if (ImGui::Button("X", ImVec2(btn_w, bar_h))) {
		// 가장 안전: 메시지로 닫기(윈도우 스레드가 처리)
		::PostMessage(Engine::GetInstance()->GetMainWndHandle(), WM_CLOSE, 0, 0);
	}

	ImGui::PopStyleVar();
}