#pragma once

#include "EditorUI.hpp"

class Menu : public EditorUI {
public:
    Menu();
    virtual ~Menu();

public:
    virtual void Draw() override;
    virtual void DrawUI() override;

private:
    void FileMenu();
    void ViewMenu();
    void GameObjectMenu();
    void LightMenu();
    void AssetMenu();
    void RenderMenu();
    void ProjectMenu();

    void DragBar();

    void CloseButton();
};

#ifndef YOU_MUST_HAVE_CURR_LEVEL
#define YOU_MUST_HAVE_CURR_LEVEL(__func, __msg)              \
if (LEVEL_MANAGER->GetCurrentLevel() != nullptr) { __func }  \
else { LOG_WARNING(__msg); }                                    
#endif // YOU_MUST_HAVE_CURR_LEVEL