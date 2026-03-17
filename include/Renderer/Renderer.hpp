#pragma once

struct LogFile;

class Renderer {
public:
	using DrawEditorFunc = std::function<void()>;

public:
	Renderer();
	virtual ~Renderer();

public: 
	virtual bool Initialize(
		LogFile* const pLogFile,
		HWND hMainWnd,
		unsigned width,
		unsigned height) = 0;

	virtual bool Update(float deltaTime) = 0;
	virtual bool Draw() = 0;
	virtual bool DrawEditor(DrawEditorFunc func) = 0;

	virtual bool OnResize(unsigned width, unsigned height) = 0;

protected:
	LogFile* mpLogFile;
};