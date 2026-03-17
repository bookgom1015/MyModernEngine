#ifndef __SINGLETON_INL__
#define __SINGLETON_INL__

template<typename T>
T* Singleton<T>::mpThis = nullptr;

template <typename T>
Singleton<T>::Singleton() {
	atexit(Destroy);
}

template <typename T>
void Singleton<T>::Destroy() {
	if (!mpThis) return;

	delete mpThis;
	mpThis = nullptr;
}

template <typename T>
T* Singleton<T>::GetInstance() {
	if (!mpThis) mpThis = new T;

	return mpThis;
}

#endif // __SINGLETON_INL__