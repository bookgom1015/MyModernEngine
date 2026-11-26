#ifndef __HEADERS_H__
#define __HEADERS_H__

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

// 파일 시스템
#include <iostream>
#include <fstream>
#include <filesystem>

#include <cmath>
#include <random>

#include <functional>

#include <exception>

#include <optional>

#include <chrono>


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#include <wrl.h>
#include <Windows.h>
#include <windowsx.h>

#endif // __HEADERS_H__