#pragma once

// Core
#ifdef _DEBUG
	#pragma comment(lib, "Core_Debug.lib")
#else
	#pragma comment(lib, "Core_Release.lib")
#endif

#include "Core/minimum.h"

// Render Iterface
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

// Global
#include "LightData.h"

// Graphics API
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