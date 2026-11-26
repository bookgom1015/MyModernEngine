#pragma once

#include <imgui.h>

namespace EditorDrag {
	bool DragFloat(
		const char* label, float* value, 
		float speed = 1.f,
		float min = 0.f, 
		float max = 0.f, 
		const char* format = "%.3f", 
		ImGuiSliderFlags flags = 0);

	bool DragFloat3(
		const char* label, 
		float value[3], 
		float speed = 1.f,
		float min = 0.f, 
		float max = 0.f, 
		const char* format = "%.3f", 
		ImGuiSliderFlags flags = 0);
}
