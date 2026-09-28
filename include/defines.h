#pragma once

#ifdef _DEBUG
	#define NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#else
	#define NEW new
#endif

#define SINGLETON(__type)							\
	friend class Singleton<__type>;					\
													\
private:											\
	__type();										\
	__type(const __type& ref) = delete;				\
	__type& operator=(const __type& ref) = delete;	\
	__type(__type&& rval) = delete;					\
	__type& operator=(__type&& rval) = delete;		\
	virtual ~__type();								\

#define CLONE(Type) Type* Clone() { return NEW Type(*this); }

#define MAX_LAYER 32

#ifndef DISALLOW_COPY_AND_MOVE
	#define DISALLOW_COPY_AND_MOVE(__type)			\
	    __type(const __type&) = delete;				\
	    __type& operator=(const __type&) = delete;	\
	    __type(__type&&) = delete;					\
	    __type& operator=(__type&&) = delete;
#endif

typedef size_t Hash;

#if defined(_D3D12)
	#define RENDERER_HEADER "Renderer/D3D12/D3D12Renderer.hpp"
#elif defined(_D3D11)
	#define RENDERER_HEADER "Renderer/D3D11/D3D11Renderer.hpp"
#elif defined(_VK)
	#define RENDERER_HEADER "Renderer/VK/VKRenderer.hpp"
#else 
	#error No graphics API defined for the renderer
#endif

using ScriptFactory = std::function<class CScript*()>;