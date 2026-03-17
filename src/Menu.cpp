#include "pch.h"
#include "Menu.hpp"

Menu::Menu() : EditorUI("Menu") {}

Menu::~Menu() {}

void Menu::Render() {
	if (ImGui::BeginMainMenuBar()) {
		FileMenu();
		ViewMenu();
		GameObjectMenu();
		AssetMenu();
		Render();

		//bool emptySpaceHovered = ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered();
		//if (emptySpaceHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		//	ReleaseCapture();
		//	SendMessage(Engine::GetInst()->GetMainWndHwnd(), WM_NCLBUTTONDOWN, HTCAPTION, 0);
		//}
		//
		//CloseButton();

		ImGui::EndMainMenuBar();
	}
}

void Menu::RenderUI() {

}

void Menu::FileMenu() {

}

void Menu::ViewMenu() {

}

void Menu::GameObjectMenu() {

}

void Menu::AssetMenu() {

}

void Menu::RenderMenu() {

}

void Menu::CloseButton() {

}