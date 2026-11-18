#pragma once

// 자료 구조
#include <array>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

// 텍스트 서식
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

// 스레드
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

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
#include "Engine/global.h"

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

// 그래픽스 API
#pragma comment(lib, "vulkan-1.lib")

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>