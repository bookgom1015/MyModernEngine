#pragma once

#include <array>
#include <chrono>

#include "Core/PerformanceTimer.hpp"
#include "Engine/EditorUI.hpp"

class Profiler : public EditorUI {
public:
	enum class Stage {
		Input,
		Assets,
		Level,
		EditorUpdate,
		RendererUpdate,
		RendererDraw,
		EditorDraw,
		Tasks,
		Count
	};

	Profiler();
	void DrawUI() override;

	static void RecordFrame(float frameMilliseconds);

	template<typename Func>
	static bool Measure(Stage stage, Func&& func) {
		PerformanceTimer timer;
		timer.Start();
		const bool result = func();
		timer.Stop();
		sFrameStages[static_cast<int>(stage)] += static_cast<float>(timer.ElapsedMilliseconds());
		return result;
	}

private:
	static constexpr int HistorySize = 240;
	static constexpr int StageCount = static_cast<int>(Stage::Count);
	using History = std::array<float, HistorySize>;

	static std::array<float, StageCount> sFrameStages;
	static std::array<History, StageCount + 1> sHistory;
	static int sNextSample;
	static int sSampleCount;
	static bool sbPaused;
};
