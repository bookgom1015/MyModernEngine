#ifndef __FUNC_INL__
#define __FUNC_INL__

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