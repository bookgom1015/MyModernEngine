#ifndef __PTR_INL__
#define __PTR_INL__

template <typename T>
Ptr<T>::Ptr() : mpPtr{} {}

template <typename T>
Ptr<T>::Ptr(T* ptr) : mpPtr{ ptr } { if (mpPtr) mpPtr->AddRef(); }

template <typename T>
Ptr<T>::Ptr(const Ptr<T>& other) : mpPtr{ other.mpPtr } {
	if (mpPtr) mpPtr->AddRef();
}

template <typename T>
Ptr<T>::~Ptr() { if (mpPtr) mpPtr->Release(); }

template <typename T>
Ptr<T>& Ptr<T>::operator=(T* ptr) {
	if (mpPtr) mpPtr->Release();

	mpPtr = ptr;
	if (mpPtr) mpPtr->AddRef();

	return *this;
}

template <typename T>
Ptr<T>& Ptr<T>::operator=(const Ptr<T>& other) {
	if (mpPtr) mpPtr->Release();

	mpPtr = other.mpPtr;
	if (mpPtr) mpPtr->AddRef();

	return *this;
}

template <typename T>
T* Ptr<T>::operator->() const { return mpPtr; }

template <typename T>
bool Ptr<T>::operator==(T* ptr) const { return mpPtr == ptr; }

template <typename T>
bool Ptr<T>::operator==(const Ptr<T>& other) const { return mpPtr == other.mpPtr; }

template <typename T>
bool Ptr<T>::operator!=(T* ptr) const { return mpPtr != ptr; }

template <typename T>
bool Ptr<T>::operator!=(const Ptr<T>& other) const { return mpPtr != other.mpPtr; }

template <typename T>
T* Ptr<T>::Get() const { return mpPtr; }

template <typename T>
T** Ptr<T>::GetAddressOf() { return &mpPtr; }

#endif // __PTR_INL__