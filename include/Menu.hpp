#pragma once

#include "EditorUI.hpp"

class Menu : public EditorUI {
public:
    Menu();
    virtual ~Menu();

public:
    virtual void Render() override;
    virtual void RenderUI() override;

private:
    void FileMenu();
    void ViewMenu();
    void GameObjectMenu();
    void AssetMenu();
    void RenderMenu();

private:
    void CloseButton();
};

