#include "pch.h"
#include "AddComponentButton.hpp"

#include "EditorManager.hpp"

#include "Inspector.hpp"
#include "ListUI.hpp"

AddComponentButton::AddComponentButton() : ComponentUI(EComponent::E_CompButton, "AddComponentButton") {}

AddComponentButton::~AddComponentButton() {}

void AddComponentButton::DrawUI() {
    auto& style = ImGui::GetStyle();

    float spacing = style.ItemSpacing.x;
    float windowWidth = ImGui::GetContentRegionAvail().x;

    // 버튼 width 계산 (이전 프레임 값 사용)
    ImGui::SetCursorPosX((windowWidth - mButtonsWidthAccum) * 0.5f);

    mButtonsWidthAccum = 0.f;

	if (ImGui::BeginTable("AddComponentTable", 1, ImGuiTableFlags_SizingFixedFit)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        
        if (ImGui::Button("Add Component"))
            AddComponent();
        mButtonsWidthAccum += ImGui::GetItemRectSize().x;

        ImGui::SameLine(0.f, spacing);
        mButtonsWidthAccum += spacing;

        if (ImGui::Button("Add Script"))
            AddScript();
        mButtonsWidthAccum += ImGui::GetItemRectSize().x;

        ImGui::EndTable();
    }    
}

void AddComponentButton::SelectComponent(DWORD_PTR ptr) {
    Ptr<ListUI> listUI = reinterpret_cast<ListUI*>(ptr);

    std::wstring key = StrToWStr(listUI->GetSelectedString());
    LOG_INFO(std::format("{} 선택", WStrToStr(key)));

    auto type = EComponent::ComponentStringToType(WStrToStr(key));
    auto component = EComponent::GetComponent(type);

    auto ui = EDITOR_MANAGER->FindUI("Inspector");
    auto inspector = static_cast<Inspector*>(ui.Get());

    auto target = inspector->GetTargetObject();
    target->AddComponent(component);

    auto compUI = inspector->GetComponentUI(type);
    compUI->SetActive(true);
}

void AddComponentButton::SelectScript(DWORD_PTR listUI) {

}

void AddComponentButton::AddComponent() {
    Ptr<ListUI> pUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
    assert(pUI.Get());

    pUI->SetUIName("Components");

    std::vector<std::wstring> components{};

    for (int i = 0; i < EComponent::Count; ++i)
        components.push_back(StrToWStr(EComponent::ComponentTypeToString(
            static_cast<EComponent::Type>(i))));

    pUI->AddString(components);
    pUI->AddDelegate(this, (DELEGATE_1)&AddComponentButton::SelectComponent);
    pUI->SetActive(true);
}

void AddComponentButton::AddScript() {

}