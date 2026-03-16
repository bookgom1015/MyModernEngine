#pragma once

#include <mutex>

struct LogFile {
	HANDLE Handle;
	std::mutex Mutex;
};