#pragma once

#include "EditorUI.hpp"
#include "TreeUI.hpp"

class Content : public EditorUI {
public:
    Content();
    virtual ~Content();

public:
    virtual void DrawUI() override;

public:
    void Renew();

private:
    void SelectAsset(DWORD_PTR asset);

private:
	Ptr<TreeUI> mTree;
};

