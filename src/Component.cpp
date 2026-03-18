#include "pch.h"
#include "Component.hpp"

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

bool Component::Update(float dt) { (void)dt; return true; }

bool Component::FixedUpdate(float dt) { (void)dt; return true; }

bool Component::LateUpdate(float dt) { (void)dt; return true; }
