#ifndef __FUNC_INL__
#define __FUNC_INL__

std::wstring StringToWString(const std::string& str) {
	if (str.empty()) return std::wstring();

	int size = MultiByteToWideChar(
		CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	std::wstring result(size, 0);

	MultiByteToWideChar(
		CP_UTF8, 0, str.data(), (int)str.size(), &result[0], size);

	return result;
}

std::string WStringToString(const std::wstring& wstr) {
	if (wstr.empty()) return std::string();

	int size = WideCharToMultiByte(
		CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string result(size, 0);

	WideCharToMultiByte(
		CP_UTF8, 0, wstr.data(), (int)wstr.size(), &result[0], size, nullptr, nullptr);

	return result;
}

constexpr uint64_t HashString(std::string_view str) {
    uint64_t hash = 14695981039346656037ull; // offset basis
    for (char c : str) {
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
        hash *= 1099511628211ull; // prime
    }
    return hash;
}

constexpr uint64_t HashWString(std::wstring_view str) {
    uint64_t hash = 14695981039346656037ull;
    for (wchar_t c : str) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ull;
    }
    return hash;
}

Hash HashCombine(Hash seed, Hash value) {
	return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

#endif // __FUNC_INL__