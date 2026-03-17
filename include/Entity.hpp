#pragma once

class Entity {
	template<typename T>
	friend class Ptr;

public:
	Entity();
	Entity(const Entity& other);
	virtual ~Entity();

public:
	__forceinline void AddRef() noexcept;
	__forceinline void Release();

public:
	__forceinline constexpr unsigned GetInstanceID() const noexcept;

	__forceinline void SetName(const std::wstring& name);
	__forceinline const std::wstring& GetName() const;

private:
	static unsigned sNextInstanceID;

private:
	const unsigned mInstanceID;
	std::wstring mName;
	unsigned mRefCount;
};

#include "Entity.inl"