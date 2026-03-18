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