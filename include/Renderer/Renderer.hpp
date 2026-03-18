#pragma once

struct LogFile;

class Renderer {
public:
	Renderer();
	virtual ~Renderer();

public: 
	virtual bool Initialize(
		HWND hMainWnd,
		unsigned width,
		unsigned height) = 0;

	virtual bool Update(float deltaTime) = 0;
	virtual bool Draw() = 0;

	virtual bool OnResize(unsigned width, unsigned height) = 0;
};