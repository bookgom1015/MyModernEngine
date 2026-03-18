#ifndef __COMPONENT_INL__
#define __COMPONENT_INL__

constexpr EComponent::Type Component::GetType() const noexcept { return mType; }

GameObject* Component::GetOwner() const noexcept { return mpOwner; }

#endif // __COMPONENT_INL__