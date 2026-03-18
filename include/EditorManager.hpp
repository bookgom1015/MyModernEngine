#pragma once

#include "EditorUI.hpp"
#include "LogUI.hpp"

class EditorManager : public Singleton<EditorManager> {
	SINGLETON(EditorManager);

public:
	bool Initialize();

	void Draw();

public:
	void AddUI(const std::string& name, Ptr<EditorUI> ui);
	Ptr<EditorUI> FindUI(const std::string& name);

	void AddLog(const LogEntry& entry);
	void AddInfoLog(const std::string& msg);

public:
	static float CalcItemSize(std::string_view text);
	static void RightAlignNextItem(const std::initializer_list<std::string_view>& text);

	static void AcceptAssetDragDrop(
		std::string_view sender,
		EAsset::Type type,
		const std::function<void(Ptr<Asset>)>& func);

private:
	void InitializeImGui();
	void CreateEditorUI();

	void BeginFrame();
	void EndFrame();

private:
	std::map<std::string, Ptr<EditorUI>> mUIs;

	Ptr<LogUI> mLogUI;
};

#ifndef LOG_INFO
#define LOG_INFO(__msg) EditorManager::GetInstance()->AddInfoLog((__msg))
#endif // LOG_INFO