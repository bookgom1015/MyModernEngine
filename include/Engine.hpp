#pragma once

struct LogFile;

class Engine : public Singleton<Engine> {
	SINGLETON(Engine);

public:
	bool Initialize(
		HINSTANCE hInst, 
		unsigned width,
		unsigned height);
	void CleanUp();

	bool Run();

public:
	LRESULT CALLBACK MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	__forceinline HWND GetMainWndHandle() const;

	__forceinline const Uint2& GetResolution() const noexcept;

private:
	bool InitializeWindow();

	bool OnResize(unsigned width, unsigned height);

	bool Input();
	bool Update();
	bool Draw();

	bool BeforeBegin();

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

#ifndef ENGINE
#define ENGINE Engine::GetInstance()
#endif // ENGINE