#include "pch.h"
#include "Asset.hpp"

Asset::Asset(EAsset::Type type) 
	: Entity{}
	, mType{ type }
	, mKey{}
	, mRelativePath{} {}

Asset::Asset(const Asset& other) 
	: Entity{ other }
	, mKey{ other.mKey }
	, mRelativePath{} 
	, mType(other.mType) {}

Asset::~Asset() {}

bool Asset::OnAdded() { return true; }

bool Asset::Load(const std::wstring& filePath) { return true; };

bool Asset::Save(const std::wstring& filePath) { return true; };