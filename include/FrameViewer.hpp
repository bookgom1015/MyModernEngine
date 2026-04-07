#pragma once

#include "EditorUI.hpp"

class FrameViewer : public EditorUI {
public:
    using DisplayTexture = std::map<std::string, ImTextureID>;

    struct TextureDesc {
        ImTextureID Id;

    };

public:
    FrameViewer();
    virtual ~FrameViewer();

public:
    virtual void DrawUI() override;

public:
    void AddDisplayTexture(const std::string& name, ImTextureID id);

private:
    DisplayTexture mDisplayTextures;
    std::string mSelectedTexture;
};