#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <wrl.h>
#include <Windows.h>

#include <functional>

#ifdef _DLLEXPORT
	#ifndef RendererAPI
	#define RendererAPI __declspec(dllexport)
	#endif
#else
	#ifndef RendererAPI
	#define RendererAPI __declspec(dllimport)
	#endif
#endif

struct LogFile;

using DrawEditorCallback = std::function<void()>;

class Renderer {
public:
	Renderer();
	virtual ~Renderer();

public: 
	RendererAPI virtual bool Initialize(
		LogFile* const pLogFile,
		HWND hMainWnd,
		unsigned width, unsigned height,
		DrawEditorCallback callback) = 0;

	RendererAPI virtual bool Update(float deltaTime) = 0;
	RendererAPI virtual bool Draw() = 0;

	RendererAPI virtual bool OnResize(unsigned width, unsigned height) = 0;

protected:
	LogFile* mpLogFile;
};