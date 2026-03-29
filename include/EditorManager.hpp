#pragma once

#include "EditorUI.hpp"
#include "FrameViewer.hpp"
#include "LogUI.hpp"

class EditorManager : public Singleton<EditorManager> {
	SINGLETON(EditorManager);

public:
	bool Initialize();

	bool Update();
	bool Draw();

public:
	void AddUI(const std::string& name, Ptr<EditorUI> ui);
	Ptr<EditorUI> FindUI(const std::string& name);

	void AddLog(const LogEntry& entry);
	void AddInfoLog(const std::string& msg);
	void AddWarningLog(const std::string& msg);
	void AddErrorLog(const std::string& msg);

	void RegisterFocusedUI(Ptr<EditorUI> ui);

	void AddDisplayTexture(const std::string& name, ImTextureID id);

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
	void CreateEditorObjects();

	void BeginFrame();
	void EndFrame();

private:
	std::map<std::string, Ptr<EditorUI>> mUIs;

	std::vector<Ptr<GameObject>> mEditorObjects;

	Ptr<EditorUI> mFocusedUI;

	Ptr<FrameViewer> mFrameViewer;
	Ptr<LogUI> mLogUI;
};

#ifndef EDITOR_MANAGER
#define EDITOR_MANAGER EditorManager::GetInstance()
#endif // EDITOR_MANAGER

#ifndef LOG_INFO
#define LOG_INFO(__msg) EDITOR_MANAGER->AddInfoLog((__msg))
#endif // LOG_INFO

#ifndef LOG_WARNING
#define LOG_WARNING(__msg) EDITOR_MANAGER->AddWarningLog((__msg))
#endif // LOG_WARNING

#ifndef LOG_ERROR
#define LOG_ERROR(__msg) EDITOR_MANAGER->AddErrorLog((__msg))
#endif // LOG_ERROR
