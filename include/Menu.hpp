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

    void DragBar();

    void CloseButton();
};

