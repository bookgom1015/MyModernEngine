#pragma once

// Core
#ifdef _DEBUG
	#pragma comment(lib, "Core_Debug.lib")
#else
	#pragma comment(lib, "Core_Release.lib")
#endif

#include "Core/minimum.h"

// Global
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