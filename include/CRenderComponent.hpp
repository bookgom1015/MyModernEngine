#pragma once

#include "Component.hpp"

class CRenderComponent : public Component {
public:
	CRenderComponent(EComponent::Type type);
	CRenderComponent(const CRenderComponent& other);
	virtual ~CRenderComponent();
};