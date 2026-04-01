#pragma once

#include "MeshRenderUI.hpp"

class SkeletalMeshRenderUI : public RenderComponentUI {
public:
	SkeletalMeshRenderUI();
	virtual ~SkeletalMeshRenderUI();

public:
	virtual void DrawUI() override;

private:
	void SkeletonPanel();
	void AnimationPanel();

private:
	void SelectSkeleton(DWORD_PTR ptr);
	void SelectAnimation(DWORD_PTR ptr);
};