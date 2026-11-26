#include "Engine/pch.h"
#include "Engine/Profiler.hpp"

#include <algorithm>
#include <numeric>
#include <vector>

Profiler::Profiler()
	: EditorUI("Profiler") {}

void Profiler::DrawUI() {
	if (ImGui::IsWindowCollapsed()) return;

	static constexpr ImVec4 SeriesColors[] = {
		{ 0.35f, 0.84f, 0.94f, 1.f },
		{ 0.98f, 0.69f, 0.34f, 1.f },
		{ 0.46f, 0.82f, 0.63f, 1.f },
		{ 0.73f, 0.61f, 0.98f, 1.f },
		{ 0.98f, 0.52f, 0.67f, 1.f },
		{ 0.56f, 0.64f, 0.97f, 1.f },
		{ 0.88f, 0.84f, 0.43f, 1.f }
	};

	auto& timings = FrameTimings::Get();
	bool paused = timings.IsPaused();
	if (ImGui::Checkbox("Pause history", &paused))
		timings.SetPaused(paused);
	ImGui::SameLine();
	constexpr int HistorySize = FrameTimings::HistorySize;
	const int sampleCount = timings.SampleCount();
	ImGui::TextDisabled("%d / %d frames", sampleCount, HistorySize);

	if (sampleCount == 0) {
		ImGui::TextDisabled("Waiting for a completed frame...");
		return;
	}

	// 링 버퍼가 가득 차면 다음에 덮어쓸 슬롯이 가장 오래된 프레임입니다.
	const int next = timings.NextSample();
	const int offset = sampleCount == HistorySize ? next : 0;
	const int latest = (next + HistorySize - 1) % HistorySize;

	const auto drawGraph = [&](const char* title, const std::deque<FrameTimings::Series>& group) {
		struct SeriesStats {
			const FrameTimings::Series& Series;
			ImVec4 Color;
			float Average;
			float Peak;
			float TargetMilliseconds;
			bool Visible;
		};

		std::vector<SeriesStats> stats;
		stats.reserve(group.size());
		float ceiling = 0.01f;
		bool hasVisibleSeries = false;

		for (const auto& series : group) {
			const auto& history = series.Samples();
			const auto end = history.begin() + sampleCount;
			const float average = std::accumulate(history.begin(), end, 0.f) / sampleCount;
			const float peak = *(std::max_element)(history.begin(), end);
			const float targetFPS = series.TargetFramesPerSecond();
			const float targetMilliseconds = targetFPS > 0.f ? 1000.f / targetFPS : 0.f;
			const bool visible = !mHiddenSeries.contains(&series);
			stats.push_back({
				series,
				SeriesColors[stats.size() % std::size(SeriesColors)],
				average,
				peak,
				targetMilliseconds,
				visible
			});

			// 숨긴 항목의 통계는 범례에 남기고 그래프 축 범위에서 제외합니다.
			if (visible) {
				hasVisibleSeries = true;
				ceiling = (std::max)({ ceiling, peak * 1.1f, targetMilliseconds * 1.2f });
			}
		}

		ImGui::PushID(title);
		ImGui::TextUnformatted(title);
		ImGui::SameLine();
		if (hasVisibleSeries)
			ImGui::TextDisabled("0 - %.3f ms", ceiling);
		else
			ImGui::TextDisabled("No stages selected");
		ImGui::InvisibleButton("##history",
			ImVec2((std::max)(1.f, ImGui::GetContentRegionAvail().x), ImGui::GetFrameHeight() * 3.5f));

		if (ImGui::IsItemVisible()) {
			const ImVec2 min = ImGui::GetItemRectMin();
			const ImVec2 max = ImGui::GetItemRectMax();
			auto* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_FrameBg), ImGui::GetStyle().FrameRounding);
			const auto plotY = [&](float value) {
				return max.y - 2.f - (max.y - min.y - 4.f) * (value / ceiling);
			};
			const float step = (max.x - min.x) / (std::max)(1, sampleCount - 1);

			drawList->PushClipRect(min, max, true);
			std::array<ImVec2, HistorySize> points{};
			for (const auto& line : stats) {
				if (!line.Visible) continue;
				if (line.TargetMilliseconds > 0.f) {
					const float y = plotY(line.TargetMilliseconds);
					drawList->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), IM_COL32(255, 185, 85, 180));
				}
				const auto& history = line.Series.Samples();
				const ImU32 color = ImGui::GetColorU32(line.Color);
				for (int sample = 0; sample < sampleCount; ++sample) {
					const float value = history[(offset + sample) % HistorySize];
					points[sample] = ImVec2(min.x + step * sample, plotY(value));
				}
				if (sampleCount == 1)
					drawList->AddCircleFilled(ImVec2((min.x + max.x) * 0.5f, points[0].y), 2.f, color);
				else
					drawList->AddPolyline(points.data(), sampleCount, color, ImDrawFlags_None, 1.5f);
			}
			drawList->PopClipRect();

			if (hasVisibleSeries && ImGui::IsItemHovered()) {
				const int sample = std::clamp(
					static_cast<int>((ImGui::GetIO().MousePos.x - min.x) / step + 0.5f), 0, sampleCount - 1);
				const float x = sampleCount == 1 ? (min.x + max.x) * 0.5f : min.x + step * sample;
				drawList->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), ImGui::GetColorU32(ImGuiCol_TextDisabled));
				ImGui::BeginTooltip();
				for (const auto& line : stats) {
					if (!line.Visible) continue;
					const float value = line.Series.Samples()[(offset + sample) % HistorySize];
					ImGui::TextColored(line.Color, "%s  %.3f ms", line.Series.Name().c_str(), value);
				}
				ImGui::EndTooltip();
			}
		}

		constexpr ImGuiTableFlags legendFlags =
			ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
		if (ImGui::BeginTable("##legend", 4, legendFlags)) {
			ImGui::TableSetupColumn("Stage", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Last (ms)", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();

			for (auto& line : stats) {
				const auto& series = line.Series;
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::PushID(&series);
				if (ImGui::Checkbox("##visible", &line.Visible)) {
					if (line.Visible)
						mHiddenSeries.erase(&series);
					else
						mHiddenSeries.insert(&series);
				}
				ImGui::SetItemTooltip("Show or hide this line");
				ImGui::PopID();
				ImGui::SameLine();
				ImGui::TextColored(line.Color, "%s", series.Name().c_str());
				ImGui::SetItemTooltip("%s\n%s", series.Name().c_str(), series.Description().c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%.3f", series.Samples()[latest]);
				ImGui::TableNextColumn();
				ImGui::Text("%.3f", line.Average);
				ImGui::TableNextColumn();
				ImGui::Text("%.3f", line.Peak);
			}
			ImGui::EndTable();
		}

		for (const auto& line : stats) {
			if (line.Visible && line.TargetMilliseconds > 0.f && line.Average > 0.f)
				ImGui::TextDisabled("%.1f FPS  |  %g FPS target / %.2f ms",
					1000.f / line.Average, line.Series.TargetFramesPerSecond(), line.TargetMilliseconds);
		}

		ImGui::PopID();
		ImGui::Spacing();
	};

	const auto drawGroups = [&](const FrameTimings::Groups& groups) {
		for (const auto& [key, group] : groups) {
			ImGui::PushID(key.first);
			drawGraph(key.second.c_str(), group);
			ImGui::PopID();
		}
	};

	const auto& tabs = timings.GetTabs();
	if (const auto common = tabs.find(""); common != tabs.end()) {
		ImGui::PushID("##commonTimings");
		drawGroups(common->second);
		ImGui::PopID();
	}

	if (!ImGui::BeginTabBar("##profilerViews")) return;
	for (const auto& [tab, groups] : tabs) {
		if (tab.empty() || !ImGui::BeginTabItem(tab.c_str())) continue;
		ImGui::PushID(tab.c_str());
		drawGroups(groups);
		ImGui::PopID();
		ImGui::EndTabItem();
	}
	ImGui::EndTabBar();
}
