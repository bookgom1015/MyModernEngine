#pragma once

struct LogFile;

class CCamera;

class Renderer {
public:
	Renderer();
	virtual ~Renderer();

public: 
	virtual bool Initialize(
		HWND hMainWnd,
		unsigned width,
		unsigned height) = 0;
	virtual void CleanUp() = 0;

	virtual bool Update(float deltaTime) = 0;
	virtual bool Draw() = 0;

	virtual bool OnResize(unsigned width, unsigned height) = 0;

public:
	__forceinline void SetCamera(CCamera* pCamera) noexcept;
	__forceinline CCamera* GetCamera() const noexcept;

	__forceinline void SetEditorCamera(CCamera* pCamera) noexcept;
	__forceinline CCamera* GetEditorCamera() const noexcept;

	CCamera* GetActiveCamera() const;

protected:
	bool mbCleanedUp;

private:
	CCamera* mpCamera;
	CCamera* mpEditorCamera;

};

#include "Renderer.inl"