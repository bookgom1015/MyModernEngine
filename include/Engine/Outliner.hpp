#pragma once

#include "Engine/EditorUI.hpp"
#include "Engine/TreeUI.hpp"

#include "Engine/GameObject.hpp"

class Outliner : public EditorUI {
public:
    Outliner();
    virtual ~Outliner();

public:
    virtual void DrawUI() override;

public:
    void Renew();
    void RestoreSelection() ;

private:
    void AddGameObject(Ptr<TreeNode> parent, Ptr<GameObject> obj);
    void SelectGameObject(DWORD_PTR obj);

    void AddChild(DWORD_PTR src, DWORD_PTR dst);

private:
    Ptr<TreeUI> mTree;
};

#include "Engine/Outliner.inl"