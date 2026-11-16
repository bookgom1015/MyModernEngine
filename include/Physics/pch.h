#pragma once

// 코어
#ifdef _DEBUG
	#pragma comment(lib, "Core_Debug.lib")
#else
	#pragma comment(lib, "Core_Release.lib")
#endif

#include "Core/cores.h"
#include "Core/MathUtil.hpp"