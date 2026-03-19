#include "pch.h"
#include "SceneUI.hpp"

#include "Engine.hpp"

#include "LevelManager.hpp"

#if defined(_D3D12)
    #include "Renderer/D3D12/D3D12Renderer.hpp"
#endif

SceneUI::SceneUI() : EditorUI("Scene") {}

SceneUI::~SceneUI() {}

void SceneUI::DrawUI() {
    LevelControl();
    GizmoControl();
    Scene();
}

void SceneUI::LevelControl() {
    float windowWidth = ImGui::GetContentRegionAvail().x;
    float childWidth = 240.0f;

    // Level Name
    {
        auto level = LEVEL_MANAGER->GetCurrentLevel();

        std::string levelName{};
        if (level != nullptr) levelName = WStrToStr(level->GetName());

        ImGui::SetNextItemWidth(140.f);
        if (ImGui::InputText("##LEVEL_NAME", &levelName
            , level != nullptr ? 0 : ImGuiInputTextFlags_ReadOnly))
            level->SetName(StrToWStr(levelName));

        ImGui::SameLine();
    }

    ImGui::SetCursorPosX((windowWidth - childWidth) * 0.5f);
    ImGui::BeginChild("Buttons", ImVec2(childWidth, 42.f));

    auto state = LEVEL_MANAGER->GetLevelState();

    auto buttonSize = ImVec2(64.f, 38.f);

    // Play Button
    {
        bool playing = state == ELevelState::E_Playing;
        if (playing) ImGui::PushStyleColor(
            ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button("Play", buttonSize))
            ;// if (level != nullptr) ChangeLevelState(ELevelState::E_Playing);
        if (playing) ImGui::PopStyleColor();
        ImGui::SameLine();
    }
	// Pause Button
    {
        bool paused = state == ELevelState::E_Paused;
        if (paused) ImGui::PushStyleColor(
            ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button("Pause", buttonSize))
            ;// if (level != nullptr) ChangeLevelState(ELevelState::E_Paused);
        if (paused) ImGui::PopStyleColor();
        ImGui::SameLine();
    }
	// Stop Button
    {
        bool stopped = state == ELevelState::E_Stopped;
        if (stopped) ImGui::PushStyleColor(
            ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        if (ImGui::Button("Stop", buttonSize))
            ;// if (level != nullptr) ChangeLevelState(ELevelState::E_Stopped);
        if (stopped) ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::Separator();
}

void SceneUI::GizmoControl() {
    ImGui::BeginChild("GizmoButtons", ImVec2(40.f, 0.f));

    float windowWidth = ImGui::GetContentRegionAvail().x;
    float buttonWidth = 35.0f;
    float margin = 5.f;

    float dpiScale = 1.f; GetDpiForWindow(Engine::GetInstance()->GetMainWndHandle()) / 96.0f;

    auto buttonSize = ImVec2(buttonWidth * dpiScale, buttonWidth * dpiScale);
    auto dummyMargin = ImVec2(0.f, margin);

    ImGui::Dummy(dummyMargin);
    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    bool trans = mGizmoState == EGizmoState::E_Trans;
    if (trans) ImGui::PushStyleColor(
        ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    if (ImGui::Button("T", buttonSize)) mGizmoState = EGizmoState::E_Trans;
    ImGui::Dummy(dummyMargin);
    if (trans) ImGui::PopStyleColor();

    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    bool rotate = mGizmoState == EGizmoState::E_Rotate;
    if (rotate) ImGui::PushStyleColor(
        ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    if (ImGui::Button("R", buttonSize)) mGizmoState = EGizmoState::E_Rotate;
    ImGui::Dummy(dummyMargin);
    if (rotate) ImGui::PopStyleColor();

    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    bool scale = mGizmoState == EGizmoState::E_Scale;
    if (scale) ImGui::PushStyleColor(
        ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    if (ImGui::Button("S", buttonSize)) mGizmoState = EGizmoState::E_Scale;
    if (scale) ImGui::PopStyleColor();

    ImGui::EndChild();
    ImGui::SameLine();
}

void SceneUI::Scene() {
    ImVec2 avail = ImGui::GetContentRegionAvail();   // 지금 커서 위치에서 남은 영역
    if (avail.x < 1.f) avail.x = 1.f;
    if (avail.y < 1.f) avail.y = 1.f;

    mSceneSize = avail;

    ImGui::ImageWithBg(
        RENDERER->GetSceneMapSrv().ptr, avail, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 1));
        
    // 씬 이미지의 absolute 좌상단(스크린 좌표) 저장
    mSceneMin = ImGui::GetItemRectMin();;
    mSceneMax = ImGui::GetItemRectMax();;

    mbSceneHovered = ImGui::IsItemHovered();

    // 방금 그린 위젯 기준
    if (mbSceneHovered) {
        auto mouseScreen = ImGui::GetMousePos();
        auto itemMin = ImGui::GetItemRectMin();

        ImVec2 localPos;
        localPos.x = mouseScreen.x - itemMin.x;
        localPos.y = mouseScreen.y - itemMin.y;

        //KeyMgr::GetInst()->SetMousePosOnScene({ localPos.x, localPos.y });
    }
}