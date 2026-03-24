#include "pch.h"
#include "FrameViewer.hpp"

FrameViewer::FrameViewer() 
	: EditorUI("Frame Viewr") {}

FrameViewer::~FrameViewer() {}

void FrameViewer::DrawUI() {
	std::vector<const char*> keys{};
	keys.reserve(mDisplayTextures.size());

	for (const auto& key : mDisplayTextures | std::views::keys)
		keys.push_back(key.c_str());

	float availWidth = ImGui::GetContentRegionAvail().x;
	float leftWidth = availWidth * 0.8f;
	float rightWidth = availWidth - leftWidth;

	{
		ImGui::BeginChild("Left", ImVec2(leftWidth, 0), true);

		auto iter = std::find(keys.begin(), keys.end(), mSelectedTexture);

		std::string name{};
		if (iter != keys.end()) name = std::string(*iter);
		ImGui::Text(name.c_str());

		ImGui::Dummy(ImVec2(0.f, 2.f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.f, 2.f));

		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (avail.x < 1.f) avail.x = 1.f;
		if (avail.y < 1.f) avail.y = 1.f;

		if (iter != keys.end())
			ImGui::ImageWithBg(mDisplayTextures[*iter], avail, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 1));

		ImGui::EndChild();
	}

	ImGui::SameLine();

	{
		ImGui::BeginChild("Right", ImVec2(0, 0), true);

		ImGui::Text("Select Texture");

		ImGui::Dummy(ImVec2(0.f, 2.f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.f, 2.f));

		for (size_t i = 0, end = keys.size(); i < end; ++i) {
			bool selected = mSelectedTexture == keys[i];

			if (ImGui::Selectable(keys[i], selected))
				mSelectedTexture = keys[i];
		}


		ImGui::EndChild();
	}

	mDisplayTextures.clear();
}

void FrameViewer::AddDisplayTexture(const std::string& name, ImTextureID id) {	
	mDisplayTextures[name] = id;
}