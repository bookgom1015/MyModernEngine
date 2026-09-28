#include "pch.h"
#include "Renderer/Renderer.hpp"

#include "LevelManager.hpp"

#include "AMesh.hpp"

Renderer::Renderer() 
	: mbCleanedUp{} {}

Renderer::~Renderer() {}

CCamera* Renderer::GetActiveCamera() const {
	return LEVEL_MANAGER->GetCurrentLevelState() 
		== ELevelState::E_Playing ? mpCamera : mpEditorCamera;
}

bool Renderer::AddTexture(const std::wstring& filePath, const std::wstring& key) { return true; }

bool Renderer::AddMesh(const std::wstring& key, AMesh* pMesh) { return true; }

const std::wstring& Renderer::GetGlobalDiffuseIrradianceMapPath() const { throw std::exception{}; }

void Renderer::SetGlobalDiffuseIrradianceMap(const std::wstring& key) {}

const std::wstring& Renderer::GetGlobalSpecularIrradianceMapPath() const { throw std::exception{}; }

void Renderer::SetGlobalSpecularIrradianceMap(const std::wstring& key) {}

bool Renderer::BakeReflectionProbes() { return true; }

ReflectionProbeID Renderer::AddReflectionProbe(const ReflectionProbeDesc& desc) { return ReflectionProbeID{}; }

bool Renderer::HasReflectionProbe(const ReflectionProbeID& id) const { return true; }

void Renderer::UpdateReflectionProbe(ReflectionProbeID id, const ReflectionProbeDesc& desc) {}

void Renderer::RemoveReflectionProbe(const ReflectionProbeID& id) {}

void Renderer::AddDebugColliderShape(const DebugColliderShape& shape) {}