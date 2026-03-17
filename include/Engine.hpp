#pragma once

struct LogFile;

class InputManager;
class EditorManager;

class Engine : public Singleton<Engine> {
	SINGLETON(Engine);

public:
	bool Initialize(
		LogFile* const pLogFile, 
		HINSTANCE hInst, 
		unsigned width,
		unsigned height);
	bool Run();

public:
	LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	__forceinline HWND GetMainWndHandle() const;

private:
	bool InitializeWindow();

	bool OnResize(unsigned width, unsigned height);

	bool Input();
	bool Update();
	bool Draw();

private:
	LogFile* mpLogFile;

	HINSTANCE mhInst;
	HWND mhMainWnd;
	Uint2 mResolution;

	bool mbAppPaused;
	bool mbMinimized;
	bool mbMaximized;
	bool mbResizing;
	bool mbFullscreenState;
	bool mbDestroyed;

	std::unique_ptr<InputManager> mInputManager;
	std::unique_ptr<EditorManager> mEditorManager;

	Processor mProcessor;
};

#include "Engine.inl"