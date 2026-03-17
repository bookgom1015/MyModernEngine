#include "pch.h"
#include "EditorManager.hpp"

#include "Menu.hpp"

EditorManager::EditorManager()
	: mUIs{} {}

EditorManager::~EditorManager() {}

bool EditorManager::Initialize() {    
	CreateEditorUI();

	return true;
}

void EditorManager::Render() {
	for (auto& [name, ui] : mUIs) {
		if (ui->IsActive()) 
			ui->Render();
	}
}

void EditorManager::AddUI(const std::string& name, Ptr<EditorUI> ui) {
	Ptr<EditorUI> pUI = FindUI(name);
	assert(pUI == nullptr);

	mUIs.insert(std::make_pair(name, ui));
}

Ptr<EditorUI> EditorManager::FindUI(const std::string& name) {
	auto it = mUIs.find(name);
	if (it != mUIs.end()) return it->second;

	return nullptr;
}

void EditorManager::CreateEditorUI() {
	Ptr<EditorUI> pUI{};

	pUI = new Menu;
	AddUI(pUI->GetUIName(), pUI);
}