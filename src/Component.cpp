#include "pch.h"
#include "Component.hpp"

#include "GameObject.hpp"

#define GET_OTHER_COMPONENT_BODY(COM_NAME) C##COM_NAME* Component::COM_NAME() { \
	return GetOwner()->COM_NAME().Get();										\
}


Component::Component(EComponent::Type type)
	: mType{ type }
	, mpOwner{ nullptr } {}

Component::Component(const Component& other)
	: Entity(other)
	, mType{ other.mType }
	, mpOwner{ nullptr } {}

Component::~Component() {}

bool Component::Initialize() { return true; }

bool Component::Begin() { return true; }

bool Component::Update(float dt) { 


	return true; 
}

bool Component::FixedUpdate(float dt) { (void)dt; return true; }

bool Component::LateUpdate(float dt) { (void)dt; return true; }

GET_OTHER_COMPONENT_BODY(Transform);

GET_OTHER_COMPONENT_BODY(MeshRender);