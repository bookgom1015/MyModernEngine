#pragma once

#include "EditorUI.hpp"

class LogUI : public EditorUI {
public:
    LogUI();
    virtual ~LogUI();

public:
    virtual void DrawUI() override;
};