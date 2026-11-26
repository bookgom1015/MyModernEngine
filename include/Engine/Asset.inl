#ifndef __ASSET_INL__
#define __ASSET_INL__

constexpr EAsset::Type Asset::GetType() const noexcept { return mType; }

const std::wstring& Asset::GetKey() const noexcept { return mKey; }

const std::wstring& Asset::GetRelativePath() const noexcept { return mRelativePath; }

void Asset::SetKey(const std::wstring& key) { mKey = key; }

void Asset::SetRelativePath(const std::wstring& path) { mRelativePath = path; }

#endif // __ASSET_INL__