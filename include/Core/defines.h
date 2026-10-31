#ifndef __CORE_DEFINES_H__
#define __CORE_DEFINES_H__

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
#define DISALLOW_COPY_AND_MOVE(__type)				\
	    __type(const __type&) = delete;				\
	    __type& operator=(const __type&) = delete;	\
	    __type(__type&&) = delete;					\
	    __type& operator=(__type&&) = delete;
#endif // DISALLOW_COPY_AND_MOVE

typedef size_t Hash;

#endif // __CORE_DEFINES_H__