#include "pch.h"
#include "EditorManager.hpp"

#include "Menu.hpp"
#include "SceneUI.hpp"
#include "Outliner.hpp"
#include "Inspector.hpp"
#include "LogUI.hpp"

EditorManager::EditorManager()
	: mUIs{} {}

EditorManager::~EditorManager() {}

bool EditorManager::Initialize() {    
	CreateEditorUI();

	return true;
}

void EditorManager::Draw() {
	for (auto& [name, ui] : mUIs) {
		if (ui->IsActive()) 
			ui->Draw();
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

	pUI = new SceneUI;
	AddUI(pUI->GetUIName(), pUI);

	pUI = new Outliner;
	AddUI(pUI->GetUIName(), pUI);

	pUI = new Inspector;
	AddUI(pUI->GetUIName(), pUI);

	pUI = new LogUI;
	AddUI(pUI->GetUIName(), pUI);
}