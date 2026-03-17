#pragma once

#include "EditorUI.hpp"

class SceneUI : public EditorUI {
public:
    SceneUI();
    virtual ~SceneUI();

public:
    virtual void DrawUI() override;

private:
    void Scene();

private:
    ImVec2 mSceneSize;
    ImVec2 mSceneMin;
    ImVec2 mSceneMax;

    bool mbSceneHovered;
};