#pragma once

struct LogFile;

class CCamera;

class AMesh;

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
	virtual bool AddTexture(const std::wstring& filePath, const std::wstring& key);
	virtual bool AddMesh(const std::wstring& key, AMesh* pMesh);

	virtual const std::wstring& GetGlobalDiffuseIrradianceMapPath() const;
	virtual void SetGlobalDiffuseIrradianceMap(const std::wstring& key);

	virtual const std::wstring& GetGlobalSpecularIrradianceMapPath() const;
	virtual void SetGlobalSpecularIrradianceMap(const std::wstring& key);

	virtual bool BakeReflectionProbes();
	virtual ReflectionProbeID AddReflectionProbe(const ReflectionProbeDesc& desc);
	virtual bool HasReflectionProbe(const ReflectionProbeID& id) const;
	virtual void UpdateReflectionProbe(ReflectionProbeID id, const ReflectionProbeDesc& desc);
	virtual void RemoveReflectionProbe(const ReflectionProbeID& id);

	virtual void AddDebugColliderShape(const DebugColliderShape& shape);

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