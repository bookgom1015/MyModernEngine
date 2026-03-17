#pragma once

#include "EditorUI.hpp"

class Outliner : public EditorUI {
public:
    Outliner();
    virtual ~Outliner();

public:
    virtual void DrawUI() override;
};