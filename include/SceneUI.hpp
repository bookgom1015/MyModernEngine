#pragma once

#include "EditorUI.hpp"

namespace EGizmoState {
    enum Type {
        E_Trans,
        E_Rotate,
        E_Scale,
        Count
    };
}

class SceneUI : public EditorUI {
public:
    SceneUI();
    virtual ~SceneUI();

public:
    virtual void DrawUI() override;

private:
    void LevelControl();
    void GizmoControl();
    void Scene();

private:
    ImVec2 mSceneSize;
    ImVec2 mSceneMin;
    ImVec2 mSceneMax;

    bool mbSceneHovered;

	EGizmoState::Type mGizmoState;
};