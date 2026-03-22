#pragma once

// Data Structures
#include <array>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

// Text formatting
#include <string>
#include <sstream>
#include <format>
#include <string_view>
#include <cwchar>

#include <algorithm>
#include <utility>

#include <memory>

#include <typeindex>

// Thread
#include <thread>
#include <mutex>
#include <condition_variable>

#include <iostream>
#include <fstream>
#include <filesystem>

#include <functional>

#include <random>

#include <exception>

#include <optional>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <wrl.h>
#include <Windows.h>
#include <windowsx.h>

#include <DirectXColors.h>

#include "global.h"

#include "MathUtil.hpp"

#include "Singleton.hpp"
#include "Ptr.hpp"

#if defined(_D3D12)
	#pragma comment(lib, "dxguid.lib")
	#pragma comment(lib, "dxgi.lib")
	#pragma comment(lib, "d3d12.lib")
	#pragma comment(lib, "d3dcompiler.lib")
	#pragma comment(lib, "dxcompiler.lib")
	#pragma comment(lib, "DirectXTex.lib")
	
	#include <d3dx12/d3dx12.h>
	#include <DirectXTex.h>
	#include <dxgidebug.h>
	#include <dxgi1_6.h>
	#include <d3dcompiler.h>
	#include <dxcapi.h>
#endif