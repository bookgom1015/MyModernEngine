#include "Engine/pch.h"
#include "Engine/EditorDrag.hpp"

#include "Engine/InputManager.hpp"

#pragma push_macro("GetTime")
#undef GetTime
#include <imgui_internal.h>
#pragma pop_macro("GetTime")

namespace {
	template <typename DrawWidget>
	bool DrawWithCursorWrap(DrawWidget drawWidget) {
		auto& io = ImGui::GetIO();
		const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left)
			&& io.MouseSource == ImGuiMouseSource_Mouse;
		const float originalDeltaX = io.MouseDelta.x;
		if (mouseDown)
			io.MouseDelta.x = INPUT_MANAGER->GetMouseDir().x;

		const bool changed = drawWidget();
		io.MouseDelta.x = originalDeltaX;

		const ImGuiID activeId = ImGui::GetActiveID();
		if (mouseDown && ImGui::IsItemActive()
			&& ImGui::GetCurrentContext()->ActiveIdSource == ImGuiInputSource_Mouse
			&& !ImGui::TempInputIsActive(activeId)
			&& ImGui::IsMouseDragging(ImGuiMouseButton_Left, io.MouseDragThreshold * 0.5f))
			INPUT_MANAGER->WrapMouseCursor(false);

		return changed;
	}
}

bool EditorDrag::DragFloat(const char* label, float* value, float speed,
	float min, float max, const char* format, ImGuiSliderFlags flags) {
	return DrawWithCursorWrap([&] {
		return ImGui::DragFloat(label, value, speed, min, max, format, flags);
	});
}

bool EditorDrag::DragFloat3(const char* label, float value[3], float speed,
	float min, float max, const char* format, ImGuiSliderFlags flags) {
	return DrawWithCursorWrap([&] {
		return ImGui::DragFloat3(label, value, speed, min, max, format, flags);
	});
}
