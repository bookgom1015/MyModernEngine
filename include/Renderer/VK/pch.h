#pragma once

// 코어
#ifdef _DEBUG
	#pragma comment(lib, "Core_Debug.lib")
#else
	#pragma comment(lib, "Core_Release.lib")
#endif

#include "Core/minimum.h"

// 렌더러 인터페이스 | 코어
#ifdef _DEBUG
	#pragma comment(lib, "RenderInterface_Debug.lib")
	#pragma comment(lib, "Renderer.Core_Debug.lib")
#else
	#pragma comment(lib, "RenderInterface_Release.lib")
	#pragma comment(lib, "Renderer.Core_Release.lib")
#endif

#include "Renderer/Interface/RenderCameraDesc.hpp"
#include "Renderer/Interface/RenderMeshDesc.hpp"
#include "Renderer/Interface/RenderObjectDesc.hpp"
#include "Renderer/Interface/ReflectionProbeDesc.hpp"
#include "Renderer/Interface/DebugCollisionShape.hpp"

// 전역 정의
#include "Engine/LightData.h"

// 그래픽스 API
#pragma comment(lib, "vulkan-1.lib")

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>