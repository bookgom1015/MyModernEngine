#include "pch.h"
#include "SkeletalMeshRenderUI.hpp"

#include "ListUI.hpp"

#include "AssetManager.hpp"
#include "EditorManager.hpp"

namespace {
	const ImVec2 ButtonSize = ImVec2(20.f, 20.f);
}

SkeletalMeshRenderUI::SkeletalMeshRenderUI()
	: RenderComponentUI{ EComponent::E_SkeletalMeshRender, "SkeletalMeshRenderUI" } {}

SkeletalMeshRenderUI::~SkeletalMeshRenderUI() {}

void SkeletalMeshRenderUI::DrawUI() {
	OutputTitle("SkeletalMeshRender");

	Ptr<CSkeletalMeshRender> skeletalMeshRender = GetTarget()->SkeletalMeshRender();

	float buttonSize = ButtonSize.x;
	float spacing = ImGui::GetStyle().ItemSpacing.x;

	if (ImGui::BeginTable("SkeletalMeshRenderUITable"
		, 2
		, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Col1", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Col2", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		MeshPanel();

		ImGui::TableNextRow();
		SkeletonPanel();

		ImGui::TableNextRow();
		AnimationPanel();

		ImGui::TableNextRow();
		MaterialSlotPanel();

		ImGui::TableNextRow();
		MaterialPanel();

		ImGui::EndTable();
	}
}

void SkeletalMeshRenderUI::SkeletonPanel() {
	Ptr<CSkeletalMeshRender> skeletalMeshRender = GetTarget()->SkeletalMeshRender();
	Ptr<ASkeleton> skeleton = skeletalMeshRender->GetSkeleton();

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Skeleton");

	ImGui::TableSetColumnIndex(1);
	{
		float avail = ImGui::GetContentRegionAvail().x;

		// 버튼을 위한 공간을 빼고 InputText 폭 계산
		float inputWidth = avail - ButtonSize.x - ImGui::GetStyle().ItemSpacing.x;
		if (inputWidth < 50.f) inputWidth = 50.f; // 최소 폭 보장

		std::string skeletonKey{};
		if (skeleton != nullptr) skeletonKey = WStrToStr(skeleton->GetKey());

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##SkeletonName", skeletonKey.data(), skeletonKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
			if (PayLoad) {
				DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
				Ptr<Asset> pAsset = reinterpret_cast<Asset*>(data);

				if (pAsset->GetType() == EAsset::E_Skeleton)
					skeletalMeshRender->SetSkeleton(dynamic_cast<ASkeleton*>(pAsset.Get()));
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("##SkeletonBtn", ButtonSize)) {
		Ptr<ListUI> listUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
		assert(listUI.Get());

		listUI->SetUIName("Skeleton List");
		std::vector<std::wstring> skeletonNames{};
		ASSET_MANAGER->GetAssetNames(EAsset::E_Skeleton, skeletonNames);

		listUI->AddString(skeletonNames);
		listUI->AddDelegate(this, (DELEGATE_1)&SkeletalMeshRenderUI::SelectSkeleton);
		listUI->SetActive(true);
	}
}

void SkeletalMeshRenderUI::AnimationPanel() {
	Ptr<CSkeletalMeshRender> skeletalMeshRender = GetTarget()->SkeletalMeshRender();
	Ptr<AAnimationClip> animClip = skeletalMeshRender->GetAnimationClip();

	ImGui::TableSetColumnIndex(0);
	ImGui::Text("Animation Clip");

	ImGui::TableSetColumnIndex(1);
	{
		float avail = ImGui::GetContentRegionAvail().x;

		// 버튼을 위한 공간을 빼고 InputText 폭 계산
		float inputWidth = avail - ButtonSize.x - ImGui::GetStyle().ItemSpacing.x;
		if (inputWidth < 50.f) inputWidth = 50.f; // 최소 폭 보장

		std::string animationKey{};
		if (animClip != nullptr) animationKey = WStrToStr(animClip->GetKey());

		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##AnimationName", animationKey.data(), animationKey.length() + 1, ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget()) {
			const ImGuiPayload* PayLoad = ImGui::AcceptDragDropPayload("Content");
			if (PayLoad) {
				DWORD_PTR data = *((DWORD_PTR*)PayLoad->Data);
				Ptr<Asset> pAsset = reinterpret_cast<Asset*>(data);

				if (pAsset->GetType() == EAsset::E_AnimationClip)
					skeletalMeshRender->SetAnimationClip(dynamic_cast<AAnimationClip*>(pAsset.Get()));
			}

			ImGui::EndDragDropTarget();
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("##AnimationBtn", ButtonSize)) {
		Ptr<ListUI> listUI = dynamic_cast<ListUI*>(EDITOR_MANAGER->FindUI("ListUI").Get());
		assert(listUI.Get());

		listUI->SetUIName("Animation List");
		std::vector<std::wstring> animationNames{};
		ASSET_MANAGER->GetAssetNames(EAsset::E_AnimationClip, animationNames);

		listUI->AddString(animationNames);
		listUI->AddDelegate(this, (DELEGATE_1)&SkeletalMeshRenderUI::SelectAnimation);
		listUI->SetActive(true);
	}
}

void SkeletalMeshRenderUI::SelectSkeleton(DWORD_PTR ptr) {
	Ptr<ListUI> listUI = reinterpret_cast<ListUI*>(ptr);

	auto key = StrToWStr(listUI->GetSelectedString());

	Ptr<ASkeleton> skeleton = FIND(ASkeleton, key);

	GetTarget()->SkeletalMeshRender()->SetSkeleton(skeleton);
}

void SkeletalMeshRenderUI::SelectAnimation(DWORD_PTR ptr) {
	Ptr<ListUI> listUI = reinterpret_cast<ListUI*>(ptr);

	auto key = StrToWStr(listUI->GetSelectedString());

	Ptr<AAnimationClip> animClip = FIND(AAnimationClip, key);

	GetTarget()->SkeletalMeshRender()->SetAnimationClip(animClip);
}