#pragma once

#include "Entity.hpp"
#include "enum.h"

class Asset : public Entity {
	friend class AssetManager;

public:
	Asset(EAsset::Type type);
	Asset(const Asset& other);
	virtual ~Asset();

public:
	virtual bool Load(const std::wstring& filePath);
	virtual bool Save(const std::wstring& filePath);

public:
	__forceinline constexpr EAsset::Type GetType() const noexcept;

	__forceinline const std::wstring& GetKey() const noexcept;
	__forceinline const std::wstring& GetRelativePath() const noexcept;

private:
	__forceinline void SetKey(const std::wstring& key);
	__forceinline void SetRelativePath(const std::wstring& path);

private:
	std::wstring mKey;
	std::wstring mRelativePath;

	const EAsset::Type mType;
};

#include "Asset.inl"