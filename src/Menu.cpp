#include "pch.h"
#include "Menu.hpp"

#include "Engine.hpp"

#include "ALevel.hpp"

#include "EditorManager.hpp"
#include "LevelManager.hpp"

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
			Ptr<ALevel> level = NEW ALevel;
			//LEVEL_MANAGER->
			//LevelManager::GetInstance()->SetCurrentLevel(level);
		}

		if (ImGui::MenuItem("Save Level")) {
			
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