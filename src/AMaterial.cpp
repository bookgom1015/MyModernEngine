#include "pch.h"
#include "AMaterial.hpp"

#include "AssetManager.hpp"

AMaterial::AMaterial()
	: Asset(EAsset::E_Material)
	, mAlbedo{ 1.f }
	, mSpecular{ 0.08f }
	, mRoughness{ 0.5f }
	, mMetalic{} {}


AMaterial::AMaterial(const AMaterial& other) 
	: Asset{ other }
	, mAlbedo{ other.mAlbedo }
	, mSpecular{ other.mSpecular }
	, mRoughness{ other.mRoughness }
	, mMetalic{ other.mMetalic } {
}

AMaterial::~AMaterial() {}

bool AMaterial::Load(const std::wstring& filePath) {
	FILE* pFile{};
	_wfopen_s(&pFile, filePath.c_str(), L"rb");

	mAlbedoMap = LoadAssetRef<ATexture>(pFile);

	fread(&mAlbedo, sizeof(Vec3), 1, pFile);
	fread(&mSpecular, sizeof(Vec3), 1, pFile);
	fread(&mRoughness, sizeof(float), 1, pFile);
	fread(&mMetalic, sizeof(float), 1, pFile);

	fread(&mDomain, sizeof(ERenderDomain::Type), 1, pFile);

	fclose(pFile);

    return true;
}

bool AMaterial::Save(const std::wstring& filePath) {
	FILE* pFile{};
	_wfopen_s(&pFile, filePath.c_str(), L"rb");

	SaveAssetRef(pFile, mAlbedoMap.Get());

	fwrite(&mAlbedo, sizeof(Vec3), 1, pFile);
	fwrite(&mSpecular, sizeof(Vec3), 1, pFile);
	fwrite(&mRoughness, sizeof(float), 1, pFile);
	fwrite(&mMetalic, sizeof(float), 1, pFile);

	fwrite(&mDomain, sizeof(ERenderDomain::Type), 1, pFile);

	fclose(pFile);

    return true;
}