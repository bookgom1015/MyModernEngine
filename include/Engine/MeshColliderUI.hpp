#pragma once

#include "Engine/ComponentUI.hpp"

class MeshColliderUI : public ComponentUI {
public:
	MeshColliderUI();
	virtual ~MeshColliderUI();

public:
	virtual void DrawUI() override;

public:
	void OffsetPanel();
	void ScalePanel();
	void TriggerPanel();
};