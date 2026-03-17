#pragma once

#include "EditorUI.hpp"

class Inspector : public EditorUI {
public:
    Inspector();
    virtual ~Inspector();

public:
    virtual void DrawUI() override;
};