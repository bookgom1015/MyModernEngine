#pragma once

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

class Renderer {
public:
	Renderer();
	virtual ~Renderer();

public: 
	RendererAPI virtual bool Initialize(
		LogFile* const pLogFile,
		unsigned width, unsigned height) = 0;

	RendererAPI virtual bool Update(float deltaTime) = 0;
	RendererAPI virtual bool Draw() = 0;

	RendererAPI virtual bool OnResize(unsigned width, unsigned height) = 0;

protected:
	LogFile* mpLogFile;
};