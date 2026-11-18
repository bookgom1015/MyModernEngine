#pragma once

#include "Engine/ComponentUI.hpp"

class TransformUI : public ComponentUI {
public:
	TransformUI();
	virtual ~TransformUI();

public:
	virtual void DrawUI() override;
};