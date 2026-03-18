#pragma once

#include "Entity.hpp"

class GameObject;

class Component : public Entity {
	friend class GameObject;

public:
	Component(EComponent::Type type);
	Component(const Component& other);
	virtual ~Component();

public:
	virtual bool Initialize();

	virtual bool Begin();

	virtual bool Update(float dt);
	virtual bool FixedUpdate(float dt);
	virtual bool LateUpdate(float dt);

	virtual bool Final() = 0;

public:
	virtual Component* Clone() = 0;

	virtual bool SaveToLevelFile(FILE* const pFile) = 0;
	virtual bool LoadFromLevelFile(FILE* const pFile) = 0;

public:
	__forceinline constexpr EComponent::Type GetType() const noexcept;

	__forceinline GameObject* GetOwner() const noexcept;

private:
	const EComponent::Type mType;

	GameObject* mpOwner;
};

#include "Component.inl"