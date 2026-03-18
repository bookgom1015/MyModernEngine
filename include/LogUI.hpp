#pragma once

#include "EditorUI.hpp"

namespace LogLevel {
    enum Type {
        E_Info = 0,
        E_Warning,
        E_Error,
        E_Critical,
        Count
    };
}

struct LogEntry {
    LogLevel::Type Level;
    std::string Message;
};

class LogUI : public EditorUI {
public:
    LogUI();
    virtual ~LogUI();

public:
    virtual void DrawUI() override;

public:
    void AddLog(const LogEntry& entry);

private:
    std::vector<LogEntry> mLogs{};

    bool mbAutoScroll;
    bool mbScrollToBottom;
};