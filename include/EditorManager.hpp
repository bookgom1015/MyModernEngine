#pragma once

#include "ASound.hpp"

#include "EditorUI.hpp"
#include "FrameViewer.hpp"
#include "LogUI.hpp"

class EditorManager : public Singleton<EditorManager> {
	SINGLETON(EditorManager);

public:
	bool Initialize();
	void CleanUp();

	bool Update();
	bool Draw();

public:
	__forceinline Ptr<ASound> GetWarningSound() const noexcept { return mWarningSound; }

public:
	void AddUI(const std::string& name, Ptr<EditorUI> ui);
	Ptr<EditorUI> FindUI(const std::string& name);

	void AddLog(const LogEntry& entry);
	void AddInfoLog(const std::string& msg);
	void AddWarningLog(const std::string& msg);
	void AddErrorLog(const std::string& msg);

	void RegisterFocusedUI(Ptr<EditorUI> ui);

	void AddDisplayTexture(const std::string& name, ImTextureID id);

	bool SetSystemSounds();

public:
	static float CalcItemSize(std::string_view text);
	static void RightAlignNextItem(const std::initializer_list<std::string_view>& text);

	static void AcceptAssetDragDrop(
		std::string_view sender,
		EAsset::Type type,
		const std::function<void(Ptr<Asset>)>& func);

private:
	bool InitializeImGui();
	void CreateEditorUI();
	void CreateEditorObjects();

	void BeginFrame();
	void EndFrame();

	void SetDarkTheme();

private:
	bool mbCleanedUp;

	std::map<std::string, Ptr<EditorUI>> mUIs;

	std::vector<Ptr<GameObject>> mEditorObjects;

	Ptr<EditorUI> mFocusedUI;

	Ptr<FrameViewer> mFrameViewer;
	Ptr<LogUI> mLogUI;

	Ptr<ASound> mWarningSound;
};

#ifndef EDITOR_MANAGER
#define EDITOR_MANAGER EditorManager::GetInstance()
#endif // EDITOR_MANAGER

#ifndef LOG_INFO
#define LOG_INFO(__msg) EDITOR_MANAGER->AddInfoLog((__msg))
#endif // LOG_INFO

#ifndef LOG_INFO_FORMAT
#define LOG_INFO_FORMAT(__msg, ...) EDITOR_MANAGER->AddInfoLog(std::format(__msg, __VA_ARGS__))
#endif // LOG_INFO

#ifndef LOG_WARNING
#define LOG_WARNING(__msg) EDITOR_MANAGER->AddWarningLog((__msg))
#endif // LOG_WARNING

#ifndef LOG_WARNING_FORMAT
#define LOG_WARNING_FORMAT(__msg, ...) EDITOR_MANAGER->AddWarningLog(std::format(__msg, __VA_ARGS__))
#endif // LOG_WARNING

#ifndef LOG_ERROR
#define LOG_ERROR(__msg) EDITOR_MANAGER->AddErrorLog((__msg))
#endif // LOG_ERROR

#ifndef LOG_ERROR_FORMAT
#define LOG_ERROR_FORMAT(__msg, ...) EDITOR_MANAGER->AddErrorLog(std::format(__msg, __VA_ARGS__))
#endif // LOG_ERROR

#ifndef WARNING_SOUND
#define WARNING_SOUND {                                                             \
    auto warningSound = EDITOR_MANAGER->GetWarningSound();                          \
    if (warningSound) warningSound->Play(0, 1.f, true, EAudioChannel::E_Editor);    \
    else LOG_ERROR("Failed to play warning sound: Sound asset not found.");         \
}
#endif // WARNING_SOUND