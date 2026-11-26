#pragma once

template <typename T>
class Ptr {
public:
	Ptr();
	Ptr(T* ptr);
	Ptr(const Ptr<T>& other);
	virtual ~Ptr();

public:
	Ptr& operator=(T* ptr);
	Ptr& operator=(const Ptr<T>& other);

	T* operator->() const;

	bool operator==(T* ptr) const;
	bool operator==(const Ptr<T>& other) const;

	bool operator!=(T* ptr) const;
	bool operator!=(const Ptr<T>& other) const;

	explicit operator bool() const {
		return mpPtr != nullptr;
	}

	operator T* () const {
		return mpPtr;
	}

public:
	T* Get() const;
	T** GetAddressOf();

private:
	T* mpPtr;
};

#include "Ptr.inl"