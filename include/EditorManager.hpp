#pragma once

#include "EditorUI.hpp"

class EditorManager {
public:
	EditorManager();
	virtual ~EditorManager();

public:
	bool Initialize();

	void Render();

public:
	void AddUI(const std::string& name, Ptr<EditorUI> ui);
	Ptr<EditorUI> FindUI(const std::string& name);

private:
	void CreateEditorUI();

private:
	std::map<std::string, Ptr<EditorUI>> mUIs;
};