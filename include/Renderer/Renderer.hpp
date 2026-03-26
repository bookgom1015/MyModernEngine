#pragma once

#include "CCamera.hpp"

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

public:
	__forceinline void SetCamera(Ptr<CCamera> pCamera) noexcept;
	__forceinline Ptr<CCamera> GetCamera() const noexcept;

	__forceinline void SetEditorCamera(Ptr<CCamera> pCamera) noexcept;
	__forceinline Ptr<CCamera> GetEditorCamera() const noexcept;

	Ptr<CCamera> GetActiveCamera() const;

private:
	Ptr<CCamera> mpCamera;
	Ptr<CCamera> mpEditorCamera;
};

#include "Renderer.inl"