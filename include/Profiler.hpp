#pragma once

#include "EditorUI.hpp"

class Profiler : public EditorUI {
public:
    Profiler();
    virtual ~Profiler();

public:
    virtual void DrawUI() override;

private:
    float mFrameTimes[3000]{};
    unsigned mFrameOffset{};
};