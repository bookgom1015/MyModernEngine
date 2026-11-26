#pragma once

#include <unordered_set>

#include "Core/FrameTimings.hpp"
#include "Engine/EditorUI.hpp"

class Profiler : public EditorUI {
public:
	Profiler();
	void DrawUI() override;

private:
	std::unordered_set<const FrameTimings::Series*> mHiddenSeries;
};
