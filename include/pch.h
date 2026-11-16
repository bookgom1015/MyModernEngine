#pragma once

// 코어
#ifdef _DEBUG
	#pragma comment(lib, "Core_Debug.lib")
#else
	#pragma comment(lib, "Core_Release.lib")
#endif

#include "Core/minimum.h"

// 렌더러 인터페이스
#ifdef _DEBUG
	#pragma comment(lib, "RenderInterface_Debug.lib")
#else
	#pragma comment(lib, "RenderInterface_Release.lib")
#endif

#include "Renderer/Interface/RenderCameraDesc.hpp"
#include "Renderer/Interface/RenderMeshDesc.hpp"
#include "Renderer/Interface/RenderObjectDesc.hpp"
#include "Renderer/Interface/ReflectionProbeDesc.hpp"
#include "Renderer/Interface/DebugCollisionShape.hpp"

// 전역 정의
#include "global.h"

// FMOD
#ifdef _DEBUG
	#pragma comment(lib, "fmodL_vc.lib")
#else
	#pragma comment(lib, "fmod_vc.lib")
#endif

#include <fmod.h>
#include <fmod.hpp>
#include <fmod_codec.h>

// 그래픽스 API
#if defined(_D3D12)
	#pragma comment(lib, "dxguid.lib")
	#pragma comment(lib, "dxgi.lib")
	#pragma comment(lib, "d3d12.lib")
	#pragma comment(lib, "d3dcompiler.lib")
	#pragma comment(lib, "dxcompiler.lib")
	#pragma comment(lib, "DirectXTex.lib")
	#pragma comment(lib, "DirectXTK12.lib")
	
	#include <d3dx12/d3dx12.h>
	#include <DirectXTex.h>
	#include <dxgidebug.h>
	#include <dxgi1_6.h>
	#include <d3dcompiler.h>
	#include <dxcapi.h>
	#include <DirectXColors.h>
#elif defined(_VK)
	#pragma comment(lib, "vulkan-1.lib")

	#ifndef VK_USE_PLATFORM_WIN32_KHR
	#define VK_USE_PLATFORM_WIN32_KHR
	#endif
	#include <vulkan/vulkan.h>
#endif