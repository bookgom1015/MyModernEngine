#pragma once

#include "EditorUI.hpp"

class Content : public EditorUI {
public:
    Content();
    virtual ~Content();

public:
    virtual void DrawUI() override;
};

