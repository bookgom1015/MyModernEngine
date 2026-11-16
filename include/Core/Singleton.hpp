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

// 호환성 매크로: 일부 헤더에서는 SINGLETON(Type)을 사용하여 싱글톤 클래스를 표시합니다.
// 다른 곳에서 정의되지 않은 경우, 컴파일 오류를 방지하기 위해 no-op으로 정의합니다.
#ifndef SINGLETON
#define SINGLETON(T) /* 아무 동작도 하지 않습니다. */
#endif
