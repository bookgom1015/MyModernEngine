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
#include <ranges>

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

#include "MathUtil.hpp"
#include "global.h"

#include "Singleton.hpp"
#include "Ptr.hpp"

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
#pragma comment(lib, "vulkan-1.lib")

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>