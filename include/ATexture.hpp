#pragma once

#include "Asset.hpp"

class ATexture : public Asset {
public:
	ATexture();
	virtual ~ATexture();

public:
	virtual bool Load(const std::wstring& filePath) override;
};