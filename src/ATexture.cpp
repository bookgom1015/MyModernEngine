#include "pch.h"
#include "ATexture.hpp"

#include RENDERER_HEADER

ATexture::ATexture()
	: Asset{ EAsset::E_Texture } {}

ATexture::~ATexture() {}

bool ATexture::Load(const std::wstring& filePath) {
	CheckReturn(RENDERER->LoadTexture(filePath, GetKey()));

	return true;
}