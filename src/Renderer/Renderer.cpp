#include "pch.h"
#include "Renderer/Renderer.hpp"

#include "LevelManager.hpp"

Renderer::Renderer() {}

Renderer::~Renderer() {}

Ptr<CCamera> Renderer::GetActiveCamera() const {
	return LEVEL_MANAGER->GetCurrentLevelState() 
		== ELevelState::E_Playing ? mpCamera : mpEditorCamera;
}