#ifndef __ENTITY_INL__
#define __ENTITY_INL__

void Entity::AddRef() noexcept { ++mRefCount; }

void Entity::Release() { if (--mRefCount == 0) delete this; }

constexpr unsigned Entity::GetInstanceID() const noexcept { return mInstanceID; }

void Entity::SetName(const std::wstring& name) { mName = name; }

const std::wstring& Entity::GetName() const { return mName; }

#endif // __ENTITY_INL__