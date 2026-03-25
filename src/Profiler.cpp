#include "pch.h"
#include "Profiler.hpp"

Profiler::Profiler() 
	: EditorUI("Profiler") {}

Profiler::~Profiler() {}

void Profiler::DrawUI() {
	CHAR buffer[64];
	snprintf(buffer, sizeof(buffer), "%.1f FPS \n(%.3f ms)",
		ImGui::GetIO().Framerate, 1000.f / ImGui::GetIO().Framerate);

	const float TextWidth = ImGui::CalcTextSize(buffer).x;
	const float RegionWidth = ImGui::GetContentRegionAvail().x;

	mFrameTimes[mFrameOffset] = 1000.f / ImGui::GetIO().Framerate;
	mFrameOffset = (mFrameOffset + 1) % IM_ARRAYSIZE(mFrameTimes);

	auto avail = ImGui::GetContentRegionAvail();
	avail.x -= 120.f;

	ImGui::PlotLines(
		buffer,
		mFrameTimes,
		IM_ARRAYSIZE(mFrameTimes),
		mFrameOffset,
		nullptr,
		0.0f,
		16.0f,
		avail);
}