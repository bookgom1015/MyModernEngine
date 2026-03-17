#include "pch.h"
#include "SceneUI.hpp"

SceneUI::SceneUI() : EditorUI("Scene") {}

SceneUI::~SceneUI() {}

void SceneUI::DrawUI() {
    //SceneUI();
}

void SceneUI::Scene() {
    ImVec2 avail = ImGui::GetContentRegionAvail();   // 지금 커서 위치에서 남은 영역
    if (avail.x < 1.f) avail.x = 1.f;
    if (avail.y < 1.f) avail.y = 1.f;

    mSceneSize = avail;

    ImGui::ImageWithBg(nullptr, avail, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 1));
        
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