#pragma once

template <typename T>
class Singleton {
protected:
	Singleton();

public:
	static void Destroy();

public:
	static T* GetInstance();

private:
	static T* mpThis;
};

#include "Singleton.inl"

// Compatibility macro: some headers use SINGLETON(Type) to mark singleton classes.
// If not defined elsewhere, define it as a no-op to avoid compilation errors.
#ifndef SINGLETON
#define SINGLETON(T) /* no-op */
#endif
