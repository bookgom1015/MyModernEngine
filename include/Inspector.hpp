#pragma once

#include "EditorUI.hpp"

#include "GameObject.hpp"

class Inspector : public EditorUI {
public:
    Inspector();
    virtual ~Inspector();

public:
    virtual void DrawUI() override;

private:
    Ptr<GameObject> mTargetObject;
};