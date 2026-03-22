#pragma once

#include "EditorUI.hpp"
#include "TreeUI.hpp"

#include "GameObject.hpp"

class Outliner : public EditorUI {
public:
    Outliner();
    virtual ~Outliner();

public:
    virtual void DrawUI() override;

public:
    void Renew();

public:
    __forceinline void NeedToResetTarget() noexcept;

private:
    void AddGameObject(Ptr<TreeNode> parent, Ptr<GameObject> obj);
    void SelectGameObject(DWORD_PTR obj);

    void AddChild(DWORD_PTR src, DWORD_PTR dst);

private:
    Ptr<TreeUI> mTree;

	bool mbNeedToResetTarget;
};

#include "Outliner.inl"