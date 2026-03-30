#pragma once

#include "ComponentUI.hpp"

class MeshRenderUI : public ComponentUI {
public:
	MeshRenderUI();
	virtual ~MeshRenderUI();

public:
	virtual void DrawUI() override;

protected:
	virtual void TargetChanged() override;

public:
	void MeshPanel();
	void MaterialSlotPanel();
	void MaterialPanel();
	void TexturePanel();

private:
	void SelectMesh(DWORD_PTR ptr);
	void SelectMaterial(DWORD_PTR ptr);
	void SelectTexture(DWORD_PTR ptr);

private:
	int mSelectedMaterialIdx;
};