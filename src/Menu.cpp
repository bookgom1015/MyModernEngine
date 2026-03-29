#include "pch.h"
#include "Menu.hpp"

#include "Engine.hpp"

#include "EditorManager.hpp"
#include "LevelManager.hpp"
#include "AssetManager.hpp"
#include "ShaderArgumentManager.hpp"

#include "ALevel.hpp"

#include "CTransform.hpp"

namespace {
	struct WindowDragState {
		bool Dragging;
		POINT StartMouseScreen;
		RECT StartWindowRect;
	};

	static WindowDragState gDragState;
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
			if (LEVEL_MANAGER->GetCurrentLevel() != nullptr) {
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
			}
			else {
				LOG_WARNING("No level is currently loaded. Please create or load a level before creating game objects.");
			}
		}
		ImGui::EndMenu();
	}
}

void Menu::LightMenu() {
	if (ImGui::BeginMenu("Light")) {
		if (ImGui::MenuItem("Create Point Light")) {
			if (LEVEL_MANAGER->GetCurrentLevel() != nullptr) {
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
			}
			else {
				LOG_WARNING("No level is currently loaded. Please create or load a level before creating lights.");
			}
		}
		if (ImGui::MenuItem("Create Directional Light")) {
			if (LEVEL_MANAGER->GetCurrentLevel() != nullptr) {
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
			}
			else {
				LOG_WARNING("No level is currently loaded. Please create or load a level before creating lights.");
			}
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
		if (ImGui::BeginMenu("Tonemapping")) {
			ImGui::Text("Type");
			ImGui::SameLine();
			if (ImGui::Combo(
				"##Type",
				reinterpret_cast<int*>(&SHADER_ARGUMENT_MANAGER->ToneMapping.Type),
				SHADER_ARGUMENT_MANAGER->ToneMapping.TypeNames,
				SHADER_ARGUMENT_MANAGER->ToneMapping.MaxType));

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Gamma Correction")) {
			ImGui::MenuItem("Enabled", NULL, &SHADER_ARGUMENT_MANAGER->GammaCorrection.Enabled);

			ImGui::Dummy(ImVec2(0.f, 2.f));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0.f, 2.f));

			ImGui::Text("Gamma");
			ImGui::SameLine();
			ImGui::SliderFloat(
				"##Gamma"
				, &SHADER_ARGUMENT_MANAGER->GammaCorrection.Gamma
				, SHADER_ARGUMENT_MANAGER->GammaCorrection.MinGamma
				, SHADER_ARGUMENT_MANAGER->GammaCorrection.MaxGamma);

			ImGui::EndMenu();
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