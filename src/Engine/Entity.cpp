#include "Engine/pch.h"
#include "Engine/Entity.hpp"

unsigned Entity::sNextInstanceID = 0;

Entity::Entity() 
	: mInstanceID{ sNextInstanceID++ }
	, mName{}
	, mRefCount{} {}

Entity::Entity(const Entity& other) 
	: mInstanceID{ sNextInstanceID++ }
	, mName{ other.mName }
	, mRefCount{} {}

Entity::~Entity() {}