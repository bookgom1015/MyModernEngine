#pragma once

#ifdef _DEBUG
	#define NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
	#define NEW new
#endif

#define SINGLETON(__type)				\
	friend class Singleton<__type>;		\
										\
private:								\
	__type();							\
	__type(const __type& ref) = delete;	\
	virtual ~__type();					\

#define CLONE(Type) Type* Clone() { return NEW Type(*this); }

#define MAX_LAYER 32

typedef size_t Hash;
inline Hash HashCombine(Hash seed, Hash value) {
	return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}

#if defined(_D3D12)
	#define RENDERER_HEADER "Renderer/D3D12/D3D12Renderer.hpp"
#elif defined(_D3D11)
	#define RENDERER_HEADER "Renderer/D3D11/D3D11Renderer.hpp"
#endif