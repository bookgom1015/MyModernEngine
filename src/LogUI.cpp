#include "pch.h"
#include "LogUI.hpp"

LogUI::LogUI() 
	: EditorUI("Log")
	, mbAutoScroll{ true }
	, mbScrollToBottom{ false } {}

LogUI::~LogUI() {}

void LogUI::DrawUI() {
    if (ImGui::Checkbox("Auto-scroll", &mbAutoScroll)) mbScrollToBottom = true;
    ImGui::SameLine();
    if (ImGui::Button("At Bottom")) mbScrollToBottom = true;
    ImGui::SameLine();
    if (ImGui::Button("Clear Logs")) mLogs.clear();

    ImGui::Separator();

    {
        ImGui::BeginChild("Log Panel");

        const float scrollY = ImGui::GetScrollY();
        const float scrollMaxY = ImGui::GetScrollMaxY();
        const bool  atBottom = (scrollY >= scrollMaxY - 1.0f);

        for (const auto& log : mLogs) {
            ImVec4 color{};

            switch (log.Level) {
            case LogLevel::E_Info:     color = ImVec4(1, 1, 1, 1); break;
            case LogLevel::E_Warning:  color = ImVec4(1, 1, 0, 1); break;
            case LogLevel::E_Error:    color = ImVec4(1, 0, 0, 1); break;
            case LogLevel::E_Critical: color = ImVec4(1, 0, 1, 1); break;
            }

            ImGui::TextColored(color, log.Message.c_str());
        }

        if (mbScrollToBottom || (mbAutoScroll && atBottom))
            ImGui::SetScrollHereY(1.0f);

        mbScrollToBottom = false;

        // 사용자가 위로 올리면 autoScroll 끄기 (원하면)
        // "맨 아래가 아니면 autoScroll을 꺼버리는" 방식
        if (mbAutoScroll && !atBottom
            && ImGui::IsWindowHovered()
            && ImGui::GetIO().MouseWheel != 0.0f)
            mbAutoScroll = false;

        ImGui::EndChild();
    }
}

void LogUI::AddLog(const LogEntry& entry) { mLogs.push_back(entry); }