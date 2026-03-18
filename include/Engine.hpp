#pragma once

struct LogFile;

class Engine : public Singleton<Engine> {
	SINGLETON(Engine);

public:
	bool Initialize(
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
	HINSTANCE mhInst;
	HWND mhMainWnd;
	Uint2 mResolution;

	bool mbAppPaused;
	bool mbMinimized;
	bool mbMaximized;
	bool mbResizing;
	bool mbFullscreenState;

	Processor mProcessor;
};

#include "Engine.inl"