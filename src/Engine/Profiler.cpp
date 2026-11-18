#include "Engine/pch.h"
#include "Engine/Profiler.hpp"

#include <algorithm>
#include <numeric>

std::array<float, Profiler::StageCount> Profiler::sFrameStages{};
std::array<Profiler::History, Profiler::StageCount + 1> Profiler::sHistory{};
int Profiler::sNextSample{};
int Profiler::sSampleCount{};
bool Profiler::sbPaused{};

Profiler::Profiler()
	: EditorUI("Profiler") {}

void Profiler::RecordFrame(float frameMilliseconds) {
	if (!sbPaused) {
		sHistory[0][sNextSample] = frameMilliseconds;
		for (int stage = 0; stage < StageCount; ++stage)
			sHistory[stage + 1][sNextSample] = sFrameStages[stage];

		sNextSample = (sNextSample + 1) % HistorySize;
		sSampleCount = (std::min)(sSampleCount + 1, HistorySize);
	}
	sFrameStages.fill(0.f);
}

void Profiler::DrawUI() {
	if (ImGui::IsWindowCollapsed()) return;

	static constexpr const char* StageNames[StageCount] = {
		"Input", "Assets", "Level / physics", "Editor update",
		"Renderer update", "Draw / present", "Editor UI", "Tasks"
	};
	static constexpr const char* StageDetails[StageCount] = {
		"Input manager update.",
		"Asset manager update, including any work processed on the frame thread.",
		"Level update and all fixed physics steps for the frame.",
		"Editor object and state updates.",
		"Renderer update. On D3D12 this includes frame-resource fence waits.",
		"CPU time in the renderer draw call, including editor UI and any present wait.",
		"CPU time in the editor draw callback. Included in Draw / present.",
		"Task manager update on the frame thread."
	};
	static constexpr ImVec4 StageColors[StageCount] = {
		{ 0.43f, 0.71f, 0.98f, 1.f }, { 0.43f, 0.71f, 0.98f, 1.f },
		{ 0.46f, 0.82f, 0.63f, 1.f }, { 0.46f, 0.82f, 0.63f, 1.f },
		{ 0.98f, 0.69f, 0.34f, 1.f }, { 0.98f, 0.69f, 0.34f, 1.f },
		{ 0.73f, 0.61f, 0.98f, 1.f }, { 0.73f, 0.61f, 0.98f, 1.f }
	};

	ImGui::Checkbox("Pause history", &sbPaused);
	ImGui::SameLine();
	ImGui::TextDisabled("%d / %d frames", sSampleCount, HistorySize);

	if (sSampleCount == 0) {
		ImGui::TextDisabled("Waiting for a completed frame...");
		return;
	}

	const int offset = sSampleCount == HistorySize ? sNextSample : 0;
	const int latest = (sNextSample + HistorySize - 1) % HistorySize;
	constexpr float FrameBudget = 1000.f / 60.f;
	const auto drawGraph = [&](int index, const char* name, const ImVec4& color, float height) {
		const auto& history = sHistory[index];
		const auto end = history.begin() + sSampleCount;
		const float average = std::accumulate(history.begin(), end, 0.f) / sSampleCount;
		const float peak = *(std::max_element)(history.begin(), end);
		const float ceiling = (std::max)(index == 0 ? FrameBudget * 1.2f : 0.01f, peak * 1.1f);

		ImGui::PushID(index);
		ImGui::TextColored(color, "%s", name);
		if (index > 0)
			ImGui::SetItemTooltip("%s", StageDetails[index - 1]);

		ImGui::Text("%.3f  |  avg %.3f  |  max %.3f ms", history[latest], average, peak);
		if (index == 0 && average > 0.f) {
			ImGui::SameLine();
			ImGui::TextDisabled("%.1f FPS", 1000.f / average);
		}

		char scale[32];
		snprintf(scale, sizeof(scale), "0 - %.3f ms", ceiling);
		ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
		ImGui::PlotLines("##history", history.data(), sSampleCount, offset, scale,
			0.f, ceiling, ImVec2(ImGui::GetContentRegionAvail().x, height));
		ImGui::PopStyleColor();
		if (index == 0) {
			const ImVec2 min = ImGui::GetItemRectMin();
			const ImVec2 max = ImGui::GetItemRectMax();
			const ImVec2 padding = ImGui::GetStyle().FramePadding;
			const float y = max.y - padding.y - (max.y - min.y - 2.f * padding.y) * (FrameBudget / ceiling);
			ImGui::GetWindowDrawList()->AddLine(
				ImVec2(min.x + padding.x, y), ImVec2(max.x - padding.x, y), IM_COL32(255, 185, 85, 180));
		}
		ImGui::PopID();
	};

	drawGraph(0, "Frame interval", ImVec4(0.35f, 0.84f, 0.94f, 1.f), ImGui::GetFrameHeight() * 4.f);
	ImGui::TextDisabled("60 FPS target  /  16.67 ms");
	ImGui::Separator();
	ImGui::TextUnformatted("CPU stages (independent scales)");

	const int columns = ImGui::GetContentRegionAvail().x >= ImGui::GetFontSize() * 42.f ? 2 : 1;
	if (ImGui::BeginTable("##cpuStages", columns, ImGuiTableFlags_SizingStretchSame)) {
		for (int stage = 0; stage < StageCount; ++stage) {
			ImGui::TableNextColumn();
			drawGraph(stage + 1, StageNames[stage], StageColors[stage], ImGui::GetFrameHeight() * 2.5f);
		}
		ImGui::EndTable();
	}
}
