#pragma once

#include "EditorUI.hpp"
//#include "AssetUI.hpp"
#include "ComponentUI.hpp"

#include "GameObject.hpp"

class Inspector : public EditorUI {
public:
    Inspector();
    virtual ~Inspector();

public:
    virtual void DrawUI() override;

public:
    void SetTargetObject(Ptr<GameObject> target);

    void NeedToResetTarget();

public:
    __forceinline Ptr<GameObject> GetTargetObject() const noexcept;

private:
    void CreateChildUIs();

private:
    Ptr<GameObject> mTargetObject;
    Ptr<ComponentUI> mComponentUIs[EComponent::Count];
    //Ptr<ComponentUI> m_AddCompBtn;
    //
    //Ptr<Asset> m_TargetAsset;
    //Ptr<AssetUI> m_arrAssetUI[EAsset::Count];
    //
    //Ptr<ScriptUI> m_arrScriptUI[SCRIPT_TYPE::Count];
};

#include "Inspector.inl"