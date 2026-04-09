#pragma once

#include "ComponentUI.hpp"

class MeshColliderUI : public ComponentUI {
public:
	MeshColliderUI();
	virtual ~MeshColliderUI();

public:
	virtual void DrawUI() override;

public:
	void OffsetPanel();
	void TriggerPanel();
};