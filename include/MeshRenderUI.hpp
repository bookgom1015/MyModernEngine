#pragma once

#include "ComponentUI.hpp"

class MeshRenderUI : public ComponentUI {
public:
	MeshRenderUI();
	virtual ~MeshRenderUI();

public:
	virtual void DrawUI() override;

private:
	void SelectMesh(DWORD_PTR ptr);
	void SelectMaterial(DWORD_PTR ptr);
};