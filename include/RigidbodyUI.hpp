#pragma once

#include "ComponentUI.hpp"

class RigidbodyUI : public ComponentUI {
public:
	RigidbodyUI();
	virtual ~RigidbodyUI();

public:
	virtual void DrawUI() override;
};