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