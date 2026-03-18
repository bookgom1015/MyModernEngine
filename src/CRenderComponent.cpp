#include "pch.h"
#include "CRenderComponent.hpp"

CRenderComponent::CRenderComponent(EComponent::Type type) : Component{ type } {}

CRenderComponent::CRenderComponent(const CRenderComponent& other) 
	: Component{ other }
	{}

CRenderComponent::~CRenderComponent() {}