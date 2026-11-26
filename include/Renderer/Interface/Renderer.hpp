#pragma once

#include "Renderer/Interface/RenderMeshDesc.hpp"
#include "Renderer/Interface/RenderObjectDesc.hpp"
#include "Renderer/Interface/RenderCameraDesc.hpp"
#include "Renderer/Interface/ReflectionProbeDesc.hpp"
#include "Renderer/Interface/DebugCollisionShape.hpp"
#include "Engine/LightData.h"

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
	virtual bool AddMesh(const std::wstring& key, const RenderMeshDesc& desc);

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
	void SetShaderDirectory(const std::wstring& directory) { mShaderDirectory = directory; }
	void UpdateLights(const std::vector<LightData>& lights) { mLights = lights; }
	void SetEditorCallbacks(std::function<void(const std::string&, std::uint64_t)> displayTexture, std::function<bool()> draw) {
		mDisplayTexture = std::move(displayTexture);
		mDrawEditor = std::move(draw);
	}
	void ClearRenderObjects();
	void UpdateRenderObjects(ERenderObjectDomain::Type domain, const std::vector<RenderObjectDesc>& objects);

public:
	__forceinline void UpdateCamera(const RenderCameraDesc& desc) noexcept;

protected:
	bool mbCleanedUp;

protected:
	std::wstring mShaderDirectory;
	std::vector<LightData> mLights;
	std::function<void(const std::string&, std::uint64_t)> mDisplayTexture;
	std::function<bool()> mDrawEditor;
	void AddDisplayTexture(const std::string& name, std::uint64_t id) {
		if (mDisplayTexture) mDisplayTexture(name, id);
	}
	RenderCameraDesc mCamera{};
	std::array<std::vector<RenderObjectDesc>, ERenderObjectDomain::Count> mRenderObjectDescs;

};

#include "Engine/Renderer.inl"